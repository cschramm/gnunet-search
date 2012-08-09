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
 * @file cli-client/gnunet-search.c
 * @brief ...
 * @author Julian Kranz
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_client_lib.h>
#include "gnunet_search_service.h"
#include "gnunet_protocols_search.h"
#include "../client/server-communication/server-communication.h"
#include "../communication/communication.h"
#include "util/client-util.h"

#define GNUNET_SEARCH_ACTION_STRING_SEARCH "search"
#define GNUNET_SEARCH_ACTION_STRING_ADD "add"

static int ret;

static char *action_string;
static char *file_string;
static char *keyword_string;

static void gnunet_search_receive_handler(size_t size, void *buffer, void *cls) {
	GNUNET_assert(size >= sizeof(struct search_response));

	struct search_response *response = (struct search_response*)buffer;

	GNUNET_assert(size == response->size);

	switch(response->type) {
		case GNUNET_SEARCH_RESPONSE_TYPE_DONE: {
			printf("Server: Done\n");
			break;
		}
		case GNUNET_SEARCH_RESPONSE_TYPE_RESULT: {
			size_t result_length = size - sizeof(struct search_response);

			char *result = (char*) malloc(result_length + 1);
			memcpy(result, response + 1, result_length);
			result[result_length] = 0;

			gnunet_search_util_replace(result, result_length, 0, '\n');

			printf("Server result: \n%s-----------\n", result);

			free(result);

			gnunet_search_server_communication_receive(0);
		}
	}
}

static void gnunet_search_transmit_urls(const char *file) {
	char **urls = NULL;
	size_t urls_length = gnunet_search_util_urls_read(&urls, file);

	void *serialized;
	size_t serialized_size = gnunet_search_util_serialize(urls, urls_length, &serialized);

	struct search_command *cmd = (struct search_command*) serialized;

	cmd->size = serialized_size;
	cmd->action = GNUNET_SEARCH_ACTION_ADD;

	for(int i = 0; i < urls_length; ++i)
		free(urls[i]);
	free(urls);

	gnunet_search_communication_transmit(serialized, serialized_size);

	free(serialized);
}

static void gnunet_search_transmit_keyword(const char *keyword) {
	void *serialized;
	size_t serialized_size;
	FILE *memstream = open_memstream((char**) &serialized, &serialized_size);

	fseek(memstream, sizeof(struct search_command), SEEK_CUR);
	fwrite(keyword, 1, strlen(keyword) + 1, memstream);

	fclose(memstream);

	struct search_command *cmd = (struct search_command*) serialized;
	cmd->action = GNUNET_SEARCH_ACTION_SEARCH;
	cmd->size = serialized_size;

	gnunet_search_communication_transmit(serialized, serialized_size);

	free(serialized);
}

/**
 * Main function that will be run by the scheduler.
 *
 * @param cls closure
 * @param args remaining command-line arguments
 * @param cfgfile name of the configuration file used (for saving, can be NULL!)
 * @param cfg configuration
 */
static void gnunet_search_run(void *cls, char * const *args, const char *cfgfile, const struct GNUNET_CONFIGURATION_Handle *cfg) {
	ret = 0;

//	printf("action: %s\n", action_string);
//	printf("file: %s\n", file_string);

	gnunet_search_server_communication_init(cfg);
	gnunet_search_communication_listener_add(&gnunet_search_receive_handler);
	gnunet_search_server_communication_receive(0);

	if(!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_SEARCH))
		gnunet_search_transmit_keyword(keyword_string);
	else if(!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_ADD))
		gnunet_search_transmit_urls(file_string);

}

/**
 * The main function to ext.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int main(int argc, char * const *argv) {
	static const struct GNUNET_GETOPT_CommandLineOption options[] = { { 'a', "action", "search|add",
			gettext_noop("search for keyword or add list of urls"), 1, &GNUNET_GETOPT_set_string, &action_string }, {
			'u', "urls", "path/to/file", gettext_noop("specify the file containing urls"), 1, &GNUNET_GETOPT_set_string,
			&file_string }, { 'k', "keyword", "keyword", gettext_noop("specify the keyword to search for"), 1,
			&GNUNET_GETOPT_set_string, &keyword_string }, GNUNET_GETOPT_OPTION_END };
	return (GNUNET_OK
			== GNUNET_PROGRAM_run(argc, argv, "gnunet-search [options [value]]", gettext_noop("search"), options, &gnunet_search_run,
					NULL)) ? ret : 1;
}

/* end of gnunet-search.c */
