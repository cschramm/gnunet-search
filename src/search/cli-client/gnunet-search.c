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
 * @file search/gnunet-search.c
 * @brief ext tool
 * @author 
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_client_lib.h>
#include "gnunet_search_service.h"
#include "gnunet_protocols_search.h"
#include "server-communication/server-communication.h"

#define GNUNET_SEARCH_ACTION_STRING_SEARCH "search"
#define GNUNET_SEARCH_ACTION_STRING_ADD "add"

static int ret;

static char *action_string;
static char *file_string;
static char *keyword_string;

static void receive_response(void *cls, const struct GNUNET_MessageHeader * msg) {
	struct search_response *response = (struct search_response*) (msg + 1);

	GNUNET_assert(ntohs(msg->size) >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct search_response));

	switch (response->type) {
		case GNUNET_SEARCH_RESPONSE_TYPE_DONE: {
			printf("Server: Done\n");
			break;
		}
		case GNUNET_SEARCH_RESPONSE_TYPE_RESULT: {
			GNUNET_assert(response->size >= sizeof(struct search_response));
			size_t result_length = response->size - sizeof(struct search_response);

			char *result = (char*) malloc(result_length + 1);
			memcpy(result, response + 1, result_length);
			result[result_length] = 0;

			printf("Server result: %s\n", result);

			free(result);
		}
	}
}

/**
 * Main function that will be run by the scheduler.
 *
 * @param cls closure
 * @param args remaining command-line arguments
 * @param cfgfile name of the configuration file used (for saving, can be NULL!)
 * @param cfg configuration
 */
static void run(void *cls, char * const *args, const char *cfgfile, const struct GNUNET_CONFIGURATION_Handle *cfg) {
	ret = 0;

//	printf("action: %s\n", action_string);
//	printf("file: %s\n", file_string);

	client_connection = GNUNET_CLIENT_connect("search", cfg);
	gnunet_search_server_communication_message_queue_init();

	if (!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_SEARCH))
		transmit_keyword(keyword_string);
	else if (!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_ADD))
		transmit_urls(file_string);

	GNUNET_CLIENT_receive(client_connection, &receive_response, NULL, GNUNET_TIME_relative_get_forever_());
}

//static int cmdline_processor(
//		struct GNUNET_GETOPT_CommandLineProcessorContext * ctx, void *scls,
//		const char *option, const char *value) {
//	printf("option: %s, value: %s", option, value);
//}

/**
 * The main function to ext.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int main(int argc, char * const *argv) {
//	char str[5];
//	for (int i = 0; i < 5; ++i)
//		str[i] = 42;
//
//	sprintf(str, "%s", "tut");
//
//	for (int i = 0; i < 5; ++i)
//		printf("%d\n", str[i]);
//
//	char *str;
//	size_t size;
//
//	FILE *stream = open_memstream(&str, &size);
//
//	fprintf(stream, "%s\\0", "tut");
//
//	fclose(stream);
//
//	for (int i = 0; i < size; ++i)
//		printf("%d\n", str[i]);

	static const struct GNUNET_GETOPT_CommandLineOption options[] = { { 'a', "action", "search|add",
			gettext_noop("search for keyword or add list of urls"), 1, &GNUNET_GETOPT_set_string, &action_string }, {
			'u', "urls", "path/to/file", gettext_noop("specify the file containing urls"), 1, &GNUNET_GETOPT_set_string,
			&file_string }, { 'k', "keyword", "keyword", gettext_noop("specify the keyword to search for"), 1,
			&GNUNET_GETOPT_set_string, &keyword_string }, GNUNET_GETOPT_OPTION_END };
	return (GNUNET_OK
			== GNUNET_PROGRAM_run(argc, argv, "gnunet-search [options [value]]", gettext_noop("search"), options, &run,
					NULL)) ? ret : 1;
}

/* end of gnunet-search.c */
