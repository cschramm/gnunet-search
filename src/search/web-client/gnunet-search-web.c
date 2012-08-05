/*
 This file is part of GNUnet.
 (C)

 GNUnet is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 3, or (at your
 option) any later version.

 GNUnet is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with GNUnet; see the file COPYING.  If not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 */

/**
 * @file search/gnunet-service-search-web.c
 * @brief search web server
 * @author Christopher Schramm
 */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

#include <ClearSilver.h>
#include <microhttpd.h>
#include <jansson.h>

#include "gnunet_protocols_search.h"

/**
 * The context of the request to be rendered
 */
struct request_context {
	unsigned int status; //!< HTTP status code
	char *type; //!< MIME type
	char *output; //!< Response body
};

/**
 * The context for a query
 */
struct query_context {
	char *query;
	struct webserver_context *server_context;
};

/**
 * Representation of a local file
 */
struct local_file {
	char * name;
	char * type;
	struct local_file * next;
};

/**
 * Result set for a specific query
 */
struct search_results {
	char *query;
	unsigned int num;
	struct search_results * next;
	char **results;
};

/**
 * webserver context
 */
struct webserver_context {
	struct local_file * first_local_file; //!< pointer to a linked list of local files
	struct search_results * first_results; //!< pointer to a linked list of search result sets
	struct search_results * last_results; //!< pointer to the last element of the search result set list
	const struct GNUNET_CONFIGURATION_Handle * gnunet_cfg; //!< GNUnet configuration handle
};

/**
 * Renders a ClearSilver error and updates the given request context
 * 
 * @param context request context to put the error message
 * @param err ClearSilver error
 */
void render_error(struct request_context * context, NEOERR *err) {
	STRING str;
	string_init(&str);
	nerr_error_string(err, &str);
	char *errstr = GNUNET_malloc(strlen(str.buf) + 46);
	sprintf(errstr, "An error occured when parsing template.html: %s", str.buf);
	string_clear(&str);
	context->output = errstr;
	context->status = 500;
}

/**
 * CSOUTFUNC to render the ClearSilver parse tree
 *
 * @param ctx request context to append the data to
 * @param output parsed data to append
 */
NEOERR * render_output(void * ctx, char *output) {
	struct request_context * context = (struct request_context *)ctx;
	char * tmp = GNUNET_malloc(strlen(context->output) + strlen(output) + 1);
	sprintf(tmp, "%s%s", context->output, output);
	free(context->output);
	context->output = tmp;
	return STATUS_OK;
}

/**
 * Serve local file if URL matches
 *
 * @param server_context webserver context containing list of local files
 * @param context request context to update
 * @param filename request file name
 * @return MHD_response or 0 on error
 */
struct MHD_Response * serve_file(const struct webserver_context * server_context, struct request_context * context, const char *filename) {
	if (strlen(filename) && server_context->first_local_file) {
		for (struct local_file * current = server_context->first_local_file; current; current = current->next) {
			if (!strcmp(current->name, filename)) {
				char filename[strlen(DATADIR"/") + strlen(current->name) + 1];
				sprintf(filename, DATADIR"/%s", current->name);
				int fd = open(filename, O_RDONLY);
				if (fd != -1) {
					struct stat sbuf;
					if (!fstat(fd, &sbuf)) {
						context->status = 200;
						context->type = current->type;
						return MHD_create_response_from_fd_at_offset(sbuf.st_size, fd, 0);
					} else {
						close(fd);
					}
				}
			}
		}
	}
	return 0;
}

/**
 * Render search page
 *
 * @param context request context to update
 * @param q search query
 */
void render_page(struct request_context * context, const char *q, int error) {
	HDF *hdf;
	CSPARSE *parse;
	
	hdf_init(&hdf);
	cs_init(&parse, hdf);
	cgi_register_strfuncs(parse);

	hdf_set_value(hdf, "q", q);
	hdf_set_int_value(hdf, "error", error);
	char hostname[256];
	gethostname(hostname, 256);
	hdf_set_value(hdf, "hostname", hostname);

	NEOERR *err = cs_parse_file(parse, DATADIR"/template.html");
	if (err == STATUS_OK) {
		context->output = GNUNET_malloc(1);
		context->output[0] = 0;
		err = cs_render(parse, context, render_output);
		if (err == STATUS_OK) {
			nerr_ignore(&err);
			hdf_destroy(&hdf);
			context->status = MHD_HTTP_OK;
			context->type = "text/html";
		} else {
			render_error(context, err);
		}
	} else {
		render_error(context, err);
	}
}

struct search_results * lookup_results(struct search_results * results_list, const char *query) {
	for (struct search_results * current = results_list; current; current = current->next)
		if (!strcmp(current->query, query))
			return current;
	return 0;
}

/**
 * Extract results from GNUnet message
 *
 * @param cls webserver context to store results
 * @param msg GNUnet message
 */
void receive_response(void *cls, const struct GNUNET_MessageHeader * msg) {
	struct query_context * context = cls;

	struct search_response *response = (struct search_response*)(msg + 1);

	GNUNET_assert(htons(msg->size) >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct search_response));

	if (response->type != GNUNET_SEARCH_RESPONSE_TYPE_RESULT)
		return;

	GNUNET_assert(response->size >= sizeof(struct search_response));
	size_t result_length = response->size - sizeof(struct search_response);

	// already received results for this query?
	struct search_results * results = lookup_results(context->server_context->first_results, context->query);

	if (results) {
		// skip if this result is already known
		for (unsigned int i = 0; i < results->num; i++)
			if (!strncmp(results->results[0], (const char *)response + 1, result_length))
				return;
		// extend results array otherwise
		GNUNET_realloc(results->results, ++results->num * sizeof(char *));
	} else {
		// build new result set
		results = GNUNET_malloc(sizeof(struct search_results));
		results->next = 0;
		results->query = GNUNET_strdup(context->query);
		results->results = GNUNET_malloc(sizeof(char *));
		results->num = 1;

		if (context->server_context->first_results)
			context->server_context->last_results->next = results;
		else
			context->server_context->first_results = results;
		context->server_context->last_results = results;
	}

	// copy result into repective array
	results->results[results->num - 1] = (char*)GNUNET_malloc(result_length + 1);
	memcpy(results->results[results->num - 1], response + 1, result_length);
	results->results[results->num - 1][result_length] = 0;

	free(context);
}

/**
 * Render received events
 *
 * @param server_context webserver context to get results from
 * @param context request context to update
 * @param q query string to get results for
 * @param o offset to start at
 */
void render_results(struct webserver_context * server_context, struct request_context * context, const char *q, int o) {
	context->status = MHD_HTTP_OK;
	context->type = "text/html";
	
	json_t *arr = json_array();

	struct search_results * results = lookup_results(server_context->first_results, q);
	if (results)
		for (unsigned int i = o; i < results->num; i++)
			json_array_append_new(arr, json_string(results->results[i]));

	context->output = json_dumps(arr, 0);
}

size_t transmit_ready(void *cls, size_t size, void *buffer) {
	size_t msg_size = sizeof(struct GNUNET_MessageHeader);

	struct search_command *cmd = (struct search_command*) cls;
	msg_size += cmd->size;

	GNUNET_assert(size >= msg_size);

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) buffer;
	header->type = GNUNET_MESSAGE_TYPE_SEARCH;
	header->size = htons(msg_size);

	memcpy(buffer + sizeof(struct GNUNET_MessageHeader), cls, cmd->size);

	free(cls);

	return msg_size;
}

/**
 * Query service
 *
 * @param server_context webserver context to store results in
 * @param q query string
 * @return 0 on success, 1 on error
 */
int start_search(struct webserver_context * server_context, const char *q) {
	struct GNUNET_CLIENT_Connection *client_connection = GNUNET_CLIENT_connect("search", server_context->gnunet_cfg);

	if (!client_connection)
		return 1;

	char *serialized;
	size_t serialized_size;
	FILE *memstream = open_memstream(&serialized, &serialized_size);

	fseek(memstream, sizeof(struct search_command), SEEK_CUR);
	fwrite(q, 1, strlen(q) + 1, memstream);

	fclose(memstream);

	struct search_command *cmd = (struct search_command*) serialized;
	cmd->action = GNUNET_SEARCH_ACTION_SEARCH;
	cmd->size = serialized_size;

	GNUNET_CLIENT_notify_transmit_ready(client_connection, sizeof(struct GNUNET_MessageHeader) + serialized_size, GNUNET_TIME_relative_get_forever_(), 1, &transmit_ready, serialized);

	struct query_context * context = GNUNET_malloc(sizeof(struct query_context));
	context->query = GNUNET_strdup(q);
	context->server_context = server_context;
	GNUNET_CLIENT_receive(client_connection, &receive_response, context, GNUNET_TIME_relative_get_forever_());

	return 0;
}

/**
 * MHD_AccessHandlerCallback for the webserver
 *
 * @param cls webserver context
 * @param connection libmicrohttpd connection
 * @param url requested URL
 * @param method HTTP request method
 * @param version HTTP request version
 * @param upload_data HTTP request body
 * @param upload_data_size HTTP request body length
 * @param con_cls persistent data (unused)
 * @return MHD_YES on success, MHD_NO on error
 */
int uri_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_dat_size, void **con_cls) {
	struct webserver_context * server_context = (struct webserver_context *)cls;
	
	struct request_context * context = malloc(sizeof(struct request_context));
	context->type = "text/plain";
	
	struct MHD_Response * response = serve_file(server_context, context, url + 1);

	if (!response) {
		if (!strcmp(url, "/")) {
			const char *q = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");
			int error = q && start_search(server_context, q);
			render_page(context, q, error);
		} else if (!strcmp(url, "/results")) {
			render_results(server_context, context, MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q"), atoi(MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "o")));
		} else {
			context->output = GNUNET_malloc(16);
			strcpy(context->output, "404 - Not Found");
		}
		
		response = MHD_create_response_from_buffer(strlen(context->output), (void *)context->output, MHD_RESPMEM_MUST_FREE);
	}

	MHD_add_response_header(response, "Content-Type", context->type);
	int ret = MHD_queue_response(connection, context->status, response);
	MHD_destroy_response(response);
	return ret;
}

/**
 * Task run during shutdown.
 *
 *
 * @param cls the webserver context
 * @param tc unused
 */
void shutdown_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	MHD_stop_daemon(cls);
}

unsigned int port = 8080;
int local = 0;

/**
 * Main function that will be run by the scheduler
 *
 * Gather list of local files and start libmicrohttpd server
 *
 * @param cls closure
 * @param args remaining command-line arguments
 * @param cfgfile name of the configuration file used (for saving, can be NULL!)
 * @param cfg configuration
 */
void run(void *cls, char * const *args, const char *cfgfile, const struct GNUNET_CONFIGURATION_Handle *cfg) {
	struct webserver_context * context = GNUNET_malloc(sizeof(struct webserver_context));
	
	context->first_local_file = 0;
	context->first_results = 0;

	context->gnunet_cfg = cfg;
	
	struct local_file * current = 0;
	struct dirent * dirent;
	char *ext;
	DIR *dir = opendir(DATADIR);
	if (dir) {
		while ((dirent = readdir(dir))) {
			if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, "..") || !strcmp(dirent->d_name, "template.html"))
				continue;
				
			if (current) {
				current->next = GNUNET_malloc(sizeof(struct local_file));
				current = current->next;
			} else {
				current = context->first_local_file = GNUNET_malloc(sizeof(struct local_file));
			}
			current->name = dirent->d_name;
			current->type = "text/plain";
			if ((ext = strrchr(dirent->d_name, '.'))) {
				if (!strcmp(ext, ".png"))
					current->type = "image/png";
				else if (!strcmp(ext, ".css"))
					current->type = "text/css";
				else if (!strcmp(ext, ".js"))
					current->type = "text/javascript";
			}
			current->next = 0;
		}
		closedir(dir);
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = local ? inet_addr("127.0.0.1") : INADDR_ANY;
	
	struct MHD_Daemon * daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, port, 0, 0, uri_handler, context, MHD_OPTION_SOCK_ADDR, &addr, MHD_OPTION_END);
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &shutdown_task, daemon);
}

/**
 * The main function
 *
 * @param argc number of arguments from command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int main(int argc, char * const *argv) {
	static const struct GNUNET_GETOPT_CommandLineOption options[] = {
		{
			'p',
			"port",
			"port",
			gettext_noop("TCP port to listen on. Defaults to 8080"),
			0,
			&GNUNET_GETOPT_set_uint,
			&port
		},
		{
			's',
			"local",
			0,
			gettext_noop("only listen on localhost"),
			0,
			&GNUNET_GETOPT_set_one,
			&local
		},
		GNUNET_GETOPT_OPTION_END
	};

	return GNUNET_PROGRAM_run(argc, argv, "gnunet-search-web [options]", gettext_noop("GNUnet search webserver"), options, &run, 0);
}
