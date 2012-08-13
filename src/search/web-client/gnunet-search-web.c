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
 * @brief The context of the request to be rendered
 */
struct request_context {
	unsigned int status; //!< HTTP status code
	char *type; //!< MIME type
	char *output; //!< Response body
};

/**
 * @brief Representation of a local file
 */
struct local_file {
	char * name;
	char * type;
	struct local_file * next;
};

/**
 * @brief List of local files
 */
static struct local_file * local_file_list = 0;

/**
 * @brief Result set for a specific query
 */
struct query {
	unsigned int num_res;
	struct query * next;
	char **results;
};

/**
 * @brief A list of queries
 */
struct query_list {
	struct query * first; //!< pointer to first query
	struct query * last; //!< pointer to last query
	unsigned short len; //!< length
};

/**
 * @brief List of queries
 */
static struct query_list * query_list = 0;

/**
 * @brief Renders a ClearSilver error and updates the given request context
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
 * @brief CSOUTFUNC to render the ClearSilver parse tree
 *
 * @param ctx request context to append the data to
 * @param output parsed data to append
 */
NEOERR * render_output(void * ctx, char *output) {
	struct request_context * context = (struct request_context *)ctx;
	char * tmp = GNUNET_malloc(strlen(context->output) + strlen(output) + 1);
	sprintf(tmp, "%s%s", context->output, output);
	GNUNET_free(context->output);
	context->output = tmp;
	return STATUS_OK;
}

/**
 * @brief Serve local file if URL matches
 *
 * @param context request context to update
 * @param filename request file name
 * @return MHD_response or 0 on error
 */
struct MHD_Response * serve_file(struct request_context * context, const char *filename) {
	if (strlen(filename) && local_file_list) {
		for (struct local_file * current = local_file_list; current; current = current->next) {
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
 * @brief Render search page
 *
 * @param context request context to update
 * @param q search query
 * @param qid query id
 */
void render_page(struct request_context * context, const char *q, unsigned short qid) {
	HDF *hdf;
	CSPARSE *parse;
	
	hdf_init(&hdf);
	cs_init(&parse, hdf);
	cgi_register_strfuncs(parse);

	hdf_set_value(hdf, "q", q);
	hdf_set_int_value(hdf, "qid", qid);
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

/**
 * @brief Lookup a query and its results
 *
 * @param id The query's ID
 */
struct query * lookup_query(unsigned short id) {
	if (!query_list || !id || query_list->len < id)
		return 0;
	
	struct query * result = query_list->first;
	
	for (id--; id; id--)
		result = result->next;
	
	return result;
}

/**
 * @brief Extract results from GNUnet message
 *
 * @param size message size
 * @param buffer message
 */
void receive_response(size_t size, void *buffer) {
	GNUNET_assert(size >= sizeof(struct search_response));
	if (size < sizeof(struct search_response))
		return;

	struct search_response * response = (struct search_response*)buffer;

	GNUNET_assert(size == response->size);
	if (size != response->size)
		return;

	if (response->type != GNUNET_SEARCH_RESPONSE_TYPE_RESULT)
		return;

	size_t result_length = size - sizeof(struct search_response);

	// do we actually know the query?
	struct query * query = lookup_query(response->id);
	if (!query)
		return;
	
	// skip if this result is already known
	for (unsigned int i = 0; i < query->num_res; i++)
		if (!strncmp(query->results[0], (const char *)response + 1, result_length))
			return;
	// extend results array otherwise
	GNUNET_realloc(query->results, ++query->num_res * sizeof(char *));
	
	// copy result into repective array
	query->results[query->num_res - 1] = (char*)GNUNET_malloc(result_length + 1);
	memcpy(query->results[query->num_res - 1], response + 1, result_length);
	query->results[query->num_res - 1][result_length] = 0;
}

/**
 * @brief Render received events
 *
 * @param context request context to update
 * @param query_id query ID to get results for
 * @param offset offset to start at
 */
void render_results(struct request_context * context, unsigned short query_id, int offset) {
	context->status = MHD_HTTP_OK;
	context->type = "text/html";
	
	json_t *arr = json_array();

	struct query * query = lookup_query(query_id);
	if (query)
		for (unsigned int i = offset; i < query->num_res; i++)
			json_array_append_new(arr, json_string(query->results[i]));

	context->output = json_dumps(arr, 0);
}

/**
 * @brief Query service
 *
 * @param q query string
 * @return query id
 */
unsigned short start_search(const char *q) {
	if (!query_list) {
		query_list = GNUNET_malloc(sizeof(struct query_list));
		query_list->first = 0;
		query_list->last = 0;
		query_list->len = 0;
	}
	
	struct query * query = GNUNET_malloc(sizeof(struct query));
	query->next = 0;
	query->results = 0;
	query->num_res = 0;
	
	if (query_list->first && query_list->last)
		query_list->last->next = query;
	else
		query_list->first = query;
	query_list->last = query;
	
	unsigned short id = ++(query_list->len);
	
	char *serialized;
	size_t serialized_size;
	FILE *memstream = open_memstream(&serialized, &serialized_size);

	fseek(memstream, sizeof(struct search_command), SEEK_CUR);
	fwrite(q, 1, strlen(q) + 1, memstream);

	fclose(memstream);

	struct search_command *cmd = (struct search_command*) serialized;
	cmd->action = GNUNET_SEARCH_ACTION_SEARCH;
	cmd->size = serialized_size;
	cmd->id = id;

	gnunet_search_communication_transmit(serialized, serialized_size);

	GNUNET_free(serialized);

	return id;
}

/**
 * @brief MHD_AccessHandlerCallback for the webserver
 *
 * @param cls unused
 * @param connection libmicrohttpd connection
 * @param url requested URL
 * @param method HTTP request method
 * @param version HTTP request version
 * @param upload_data HTTP request body
 * @param upload_data_size HTTP request body length
 * @param con_cls unused
 * @return MHD_YES on success, MHD_NO on error
 */
int uri_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_dat_size, void **con_cls) {
	struct request_context * context = malloc(sizeof(struct request_context));
	context->type = "text/plain";
	
	struct MHD_Response * response = serve_file(context, url + 1);

	if (!response) {
		if (!strcmp(url, "/")) {
			const char *q = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q");
			unsigned short qid = 0;
			if (q)
				qid = start_search(q);
			render_page(context, q, qid);
		} else if (!strcmp(url, "/results")) {
			render_results(context, atoi(MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "q")), atoi(MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "o")));
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
 * @brief Task run during shutdown.
 *
 *
 * @param cls the libmicrohttpd handler
 * @param tc unused
 */
void shutdown_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	gnunet_search_server_communication_free();

	MHD_stop_daemon(cls);

	exit(0);
}

/**
 * @brief "Loop body" for libmicrohttpd request processing
 *
 * @param cls MHD_Daemon to run
 * @param tc unused
 */
static void gnunet_search_web_process_requests(void * cls, const struct GNUNET_SCHEDULER_TaskContext * tc) {
	fd_set rs, ws, es;
	FD_ZERO(&rs);
	FD_ZERO(&ws);
	FD_ZERO(&es);
	struct timeval tv;
	struct timeval * tvp;
	unsigned MHD_LONG_LONG mhd_timeout;
	int max = 0;
	if (MHD_get_fdset(cls, &rs, &ws, &es, &max) != MHD_YES)
		exit(1);

	if (MHD_get_timeout(cls, &mhd_timeout) == MHD_YES) {
		tv.tv_sec = mhd_timeout / 1000;
		tv.tv_usec = (mhd_timeout - (tv.tv_sec * 1000)) * 1000;
		tvp = &tv;
	} else
		tvp = 0;
	select(max + 1, &rs, &ws, &es, tvp);
	MHD_run(cls);
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_web_process_requests, cls);
}

unsigned int port = 8080;
int local = 0;

int ret;

/**
 * @brief Main function that will be run by the scheduler
 *
 * Gather list of local files and start libmicrohttpd server
 *
 * @param cls libmicrohttpd handle
 * @param args remaining command-line arguments
 * @param cfgfile name of the configuration file used (for saving, can be NULL!)
 * @param cfg configuration
 */
void run(void *cls, char * const *args, const char *cfgfile, const struct GNUNET_CONFIGURATION_Handle *cfg) {
	ret = gnunet_search_server_communication_init(cfg);
	if (!ret)
		return;

	gnunet_search_communication_listener_add(&receive_response);
	gnunet_search_server_communication_receive();
	
	srand(time(0));

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
				current = local_file_list = GNUNET_malloc(sizeof(struct local_file));
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
	
	struct MHD_Daemon * daemon = MHD_start_daemon(0, port, 0, 0, uri_handler, 0, MHD_OPTION_SOCK_ADDR, &addr, MHD_OPTION_END);
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &shutdown_task, daemon);
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_web_process_requests, daemon);
}

/**
 * @brief The main function
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
