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
#include "../client/server-communication/server-communication.h"
#include "../communication/communication.h"

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
	char *query_id;
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
struct query {
	char *id;
	unsigned int num_res;
	struct query * next;
	char **results;
};

/**
 * webserver context
 */
struct webserver_context {
	struct local_file * first_local_file; //!< pointer to a linked list of local files
	struct query * first_query; //!< pointer to a linked list of queries
	struct query * last_query; //!< pointer to the last element of the query list
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
 * @param qid query id
 */
void render_page(struct request_context * context, const char *q, const char *qid) {
	HDF *hdf;
	CSPARSE *parse;
	
	hdf_init(&hdf);
	cs_init(&parse, hdf);
	cgi_register_strfuncs(parse);

	hdf_set_value(hdf, "q", q);
	hdf_set_value(hdf, "qid", qid);
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

struct query * lookup_query(struct query * query_list, const char *query_id) {
	for (struct query * current = query_list; current; current = current->next)
		if (!strcmp(current->id, query_id))
			return current;
	return 0;
}

/**
 * Extract results from GNUnet message
 *
 * @param cls webserver context to store results
 * @param msg GNUnet message
 */
void receive_response(size_t size, void *buffer, void *cls) {
	struct query_context * context = cls;

	//GNUNET_assert(size >= sizeof(struct search_response));

	struct search_response * response = (struct search_response*)buffer;

	if (response->type != GNUNET_SEARCH_RESPONSE_TYPE_RESULT)
		return;

	size_t result_length = size - sizeof(struct search_response);

	struct query * query = lookup_query(context->server_context->first_query, context->query_id);
	if (query) {
		// skip if this result is already known
		for (unsigned int i = 0; i < query->num_res; i++)
			if (!strncmp(query->results[0], (const char *)response + 1, result_length))
				return;
		// extend results array otherwise
		GNUNET_realloc(query->results, ++query->num_res * sizeof(char *));
	} else {
		// build new entry
		query = GNUNET_malloc(sizeof(struct query));
		query->next = 0;
		query->id = GNUNET_strdup(context->query_id);
		query->results = GNUNET_malloc(sizeof(char *));
		query->num_res = 1;
		
		if (context->server_context->first_query)
			context->server_context->last_query->next = query;
		else
			context->server_context->first_query = query;
		context->server_context->last_query = query;
	}
	
	// copy result into repective array
	query->results[query->num_res - 1] = (char*)GNUNET_malloc(result_length + 1);
	memcpy(query->results[query->num_res - 1], response + 1, result_length);
	query->results[query->num_res - 1][result_length] = 0;

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
void render_results(struct webserver_context * server_context, struct request_context * context, const char *id, int o) {
	context->status = MHD_HTTP_OK;
	context->type = "text/html";
	
	json_t *arr = json_array();

	struct query * query = lookup_query(server_context->first_query, id);
	if (query)
		for (unsigned int i = o; i < query->num_res; i++)
			json_array_append_new(arr, json_string(query->results[i]));

	context->output = json_dumps(arr, 0);
}

/**
 * Generate unique random 8-byte ID
 *
 * @param query_list list to check for duplicates
 */
char * generate_query_id(const struct query * query_list) {
	char *id = GNUNET_malloc(sizeof(char) * 9);
	for (unsigned int i = 0; i < 8; i++)
		id[i] = (char)((rand() % 78) + 30);
	id[8] = 0;
	return id;
}

/**
 * Query service
 *
 * @param server_context webserver context to store results in
 * @param q query string
 * @return query id
 */
char * start_search(struct webserver_context * server_context, const char *q) {
	char *serialized;
	size_t serialized_size;
	FILE *memstream = open_memstream(&serialized, &serialized_size);

	fseek(memstream, sizeof(struct search_command), SEEK_CUR);
	fwrite(q, 1, strlen(q) + 1, memstream);

	fclose(memstream);

	struct search_command *cmd = (struct search_command*) serialized;
	cmd->action = GNUNET_SEARCH_ACTION_SEARCH;
	cmd->size = serialized_size;

	gnunet_search_communication_transmit(serialized, serialized_size);

	free(serialized);


	struct query_context * context = GNUNET_malloc(sizeof(struct query_context));
	context->query_id = generate_query_id(context->server_context->first_query);
	context->server_context = server_context;
	gnunet_search_server_communication_receive(context);

	return context->query_id;
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
			const char *qid = 0;
			if (q)
				qid = start_search(server_context, q);
			render_page(context, q, qid);
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

int ret;

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
	ret = gnunet_search_server_communication_init(cfg);
	if (ret)
		return;

	gnunet_search_communication_listener_add(&receive_response);
	
	
	srand(time(0));

	struct webserver_context * context = GNUNET_malloc(sizeof(struct webserver_context));
	
	context->first_local_file = 0;
	context->first_query = 0;

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
			"PORT",
			gettext_noop("TCP port to listen on. Defaults to 8080"),
			1,
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

	return (GNUNET_PROGRAM_run(argc, argv, "gnunet-search-web [options]", gettext_noop("GNUnet search webserver"), options, &run, 0) == GNUNET_OK ? 0 : ret);
}
