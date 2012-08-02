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
 * @brief ext tool
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
#include "gnunet-service-search-web.h"

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
void render_page(struct request_context * context, const char *q) {
	HDF *hdf;
	CSPARSE *parse;
	
	hdf_init(&hdf);
	cs_init(&parse, hdf);
	cgi_register_strfuncs(parse);

	hdf_set_value(hdf, "q", q);
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

	struct search_results * results = GNUNET_malloc(sizeof(struct search_results));
	results->next = 0;

	if (context->server_context->first_results)
		context->server_context->last_results->next = results;
	else
		context->server_context->first_results = results;
	context->server_context->last_results = results;

	results->query = context->query;

	results->result = (char*)GNUNET_malloc(result_length + 1);
	memcpy(results->result, response + 1, result_length);
	results->result[result_length] = 0;
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
	context->output = GNUNET_strdup("");
	if (server_context->first_results) {
                for (struct search_results * current = server_context->first_results; current; current = current->next) {
			if (!strcmp(current->query, q)) {
				free(context->output);
				// TODO: use offset
				context->output = GNUNET_strdup(current->result);
				return;
			}
		}
	}
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

void start_search(struct webserver_context * server_context, const char *q) {
	struct GNUNET_CLIENT_Connection *client_connection = GNUNET_CLIENT_connect("search", server_context->gnunet_cfg);

	char *serialized;
	size_t serialized_size;
	FILE *memstream = open_memstream(&serialized, &serialized_size);

	fseek(memstream, sizeof(struct search_command), SEEK_CUR);
	fwrite(q, 1, strlen(q) + 1, memstream);

	fclose(memstream);

	struct search_command *cmd = (struct search_command*) serialized;
	cmd->action = GNUNET_SEARCH_ACTION_SEARCH;
	cmd->size = serialized_size;

	//GNUNET_CLIENT_notify_transmit_ready(client_connection, sizeof(struct GNUNET_MessageHeader) + serialized_size, GNUNET_TIME_relative_get_forever_(), 1, &transmit_ready, serialized);

	struct query_context * context = GNUNET_malloc(sizeof(struct query_context));
	context->query = GNUNET_strdup(q);
	context->server_context = server_context;
	//GNUNET_CLIENT_receive(client_connection, &receive_response, context, GNUNET_TIME_relative_get_forever_());
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
			start_search(server_context, q);
			render_page(context, q);
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

struct webserver_context * start_webserver(int localhost, unsigned short port, struct GNUNET_CONFIGURATION_Handle * gnunet_cfg) {
	struct webserver_context * context = GNUNET_malloc(sizeof(struct webserver_context));
	
	context->first_local_file = 0;
	context->first_results = 0;

	context->gnunet_cfg = gnunet_cfg;
	
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
			}
			current->next = 0;
		}
		closedir(dir);
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = localhost ? inet_addr("127.0.0.1") : INADDR_ANY;
	
	context->daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, port, 0, 0, uri_handler, context, MHD_OPTION_SOCK_ADDR, &addr, MHD_OPTION_END);
	
	return context;
}

void stop_webserver(struct webserver_context * context) {
	if (context->first_local_file) {
		struct local_file * current = context->first_local_file;
		struct local_file * next = context->first_local_file->next;
		while (next) {
			next = current->next;
			free(current);
			current = next;
		}
	}

	if (context->first_results) {
		struct search_results * current = context->first_results;
		struct search_results * next = context->first_results->next;
		while (next) {
			next = current->next;
			free(current);
			current = next;
		}
	}
	
	MHD_stop_daemon(context->daemon);
	
	free(context);
}
