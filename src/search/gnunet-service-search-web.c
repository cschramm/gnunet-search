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

#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <string.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

#include "ClearSilver.h"
#include "microhttpd.h"

#include "gnunet-service-search-web.h"

struct request_context {
	unsigned int status;
	char * type;
	char * output;
};

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

NEOERR * render_output(void * ctx, char *output) {
	struct request_context * context = (struct request_context *)ctx;
	char * tmp = GNUNET_malloc(strlen(context->output) + strlen(output) + 1);
	sprintf(tmp, "%s%s", context->output, output);
	free(context->output);
	context->output = tmp;
	return STATUS_OK;
}

int uri_handler(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_dat_size, void **con_cls) {
	struct webserver_context * server_context = (struct webserver_context *)cls;
	
	struct request_context * context = malloc(sizeof(struct request_context));
	context->type = "text/plain";
	
	struct MHD_Response * response = 0;
	
	if (strlen(url) && server_context->first_local_file) {
		for (struct local_file * current = server_context->first_local_file; current && !response; current = current->next) {
			if (!strcmp(current->name, url + 1)) {
				char filename[strlen(DATADIR"/") + strlen(current->name) + 1];
				sprintf(filename, DATADIR"/%s", current->name);
				int fd = open(filename, O_RDONLY);
				if (fd != -1) {
					struct stat sbuf;
					if (!fstat(fd, &sbuf)) {
						response = MHD_create_response_from_fd_at_offset(sbuf.st_size, fd, 0);
						context->status = 200;
						context->type = current->type;
					} else {
						close(fd);
					}
				}
			}
		}
	}
	
	if (!response) {
		if (!strcmp(url, "/")) {
			HDF *hdf;
			CSPARSE *parse;
			
			hdf_init(&hdf);
			cs_init(&parse, hdf);
			
			// TODO: handle query
			/*
			GNUNET_SEARCH_RESULT results[] = {
				{"http://www.clearsilver.net/", "ClearSilver"},
				{"http://code.google.com/p/mongoose/", "mongoose"},
				{0, 0}
			};
			
			typedef struct {
				char *url;
				char *name;
			} GNUNET_SEARCH_RESULT;
			int i;
			GNUNET_SEARCH_RESULT result;
			char buf[255];
			for (i = 0; (result = results[i]).url; i++) {
				sprintf(buf, "results.%d.url", i);
				hdf_set_value(hdf, buf, result.url);
				sprintf(buf, "results.%d.name", i);
				hdf_set_value(hdf, buf, result.name);
			}
			*/
			
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

struct webserver_context * start_webserver(int localhost, unsigned short port) {
	struct webserver_context * context = GNUNET_malloc(sizeof(struct webserver_context));
	
	context->first_local_file = 0;
	
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
	
	MHD_stop_daemon(context->daemon);
	
	free(context);
}