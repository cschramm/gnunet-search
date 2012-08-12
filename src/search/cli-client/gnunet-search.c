/**
 * @file cli-client/gnunet-search.c
 * @brief This file contains all functions pertaining to the GNUnet Search client's main component.
 * @author Julian Kranz
 */
/*
 *  This file is part of GNUnet Search.
 *
 *  GNUnet Search is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  GNUnet Search is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNUnet Search.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_client_lib.h>
#include "gnunet_search_service.h"
#include "gnunet_protocols_search.h"
#include "../client/server-communication/server-communication.h"
#include "../communication/communication.h"
#include "util/client-util.h"

/**
 * @brief This constant defines the string representation for the search action. It is used to match the user's command line options.
 */
#define GNUNET_SEARCH_ACTION_STRING_SEARCH "search"
/**
 * @brief This constant defines the string representation for the URL add action. It is used to match the user's command line options.
 */
#define GNUNET_SEARCH_ACTION_STRING_ADD "add"

static int ret;

/**
 * @brief This variable stores the string given by the user for the action command line parameter.
 */
static char *action_string;
/**
 * @brief This variable stores the string given by the user for the URL file command line parameter.
 */
static char *file_string;
/**
 * @brief This variable stores the string given by the user for the search keyword command line parameter.
 */
static char *keyword_string;

/**
 * @brief This function handles a buffer newly received by the communication component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function handles a buffer newly received by the communication component. The buffer contains all fragments beloging
 * to a specific message. The completeness of the message has to be verified (e.g. by comparing the size of the message with
 * the length of data received).
 *
 * @param size the actual size of the buffer received
 * @param buffer the buffer containing the fragments received
 */
static void gnunet_search_receive_handler(size_t size, void *buffer) {
	GNUNET_assert(size >= sizeof(struct search_response));
	if(size < sizeof(struct search_response))
		return;

	struct search_response *response = (struct search_response*)buffer;

	GNUNET_assert(size == response->size);
	if(size != response->size)
		return;

	switch(response->type) {
		case GNUNET_SEARCH_RESPONSE_TYPE_DONE: {
			printf("Server: Done\n");

			GNUNET_SCHEDULER_shutdown();

			break;
		}
		case GNUNET_SEARCH_RESPONSE_TYPE_RESULT: {
			size_t result_length = size - sizeof(struct search_response);

			char *result = (char*) GNUNET_malloc(result_length + 1);
			memcpy(result, response + 1, result_length);
			result[result_length] = 0;

			gnunet_search_util_replace(result, result_length, 0, '\n');

			printf("Server result: \n%s-----------\n", result);

			GNUNET_free(result);

			gnunet_search_server_communication_receive();
		}
	}
}

/**
 * @brief This function transmits URLs contained in a file to the service.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function transmits URLs contained in a file to the service. The file has to contain a '\n'-seperated list of URLs. The URLs
 * are sent to the service using an ADD action.
 *
 * @param file the path to the file that contains the URLs
 */
static void gnunet_search_transmit_urls(const char *file) {
	char **urls = NULL;
	size_t urls_length = gnunet_search_util_urls_read(&urls, file);

	char *serialized;
	size_t serialized_size = gnunet_search_util_serialize((char const * const *)urls, urls_length, &serialized);

	struct search_command *cmd = (struct search_command*) serialized;

	cmd->size = serialized_size;
	cmd->action = GNUNET_SEARCH_ACTION_ADD;
	cmd->id = 0;

	for(int i = 0; i < urls_length; ++i)
		GNUNET_free(urls[i]);
	GNUNET_free(urls);

	gnunet_search_communication_transmit(serialized, serialized_size);

	GNUNET_free(serialized);
}

/**
 * @brief This function transmit a request for a keyword to the service.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function transmit a request for a keyword to the service. The keyword is sent to the service using an ADD action.
 *
 * @param keyword the keyword to search for
 */
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
	cmd->id = 0;

	gnunet_search_communication_transmit(serialized, serialized_size);

	GNUNET_free(serialized);
}

/**
 * @brief This function handles the shutdon of the application.
 *
 * @param cls the GNUnet closure
 * @param tc the GMUnet scheduler task context
 */
static void shutdown_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	gnunet_search_server_communication_free();

	exit(0);
}

/**
 * @brief This function is the main function that will be run by the scheduler.
 *
 * @param cls the GNUnet closure
 * @param args the remaining command-line arguments
 * @param cfgfile the name of the configuration file used (for saving, can be NULL!)
 * @param cfg the configuration
 */
static void gnunet_search_run(void *cls, char * const *args, const char *cfgfile, const struct GNUNET_CONFIGURATION_Handle *cfg) {
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &shutdown_task, NULL);

	ret = 0;

//	printf("action: %s\n", action_string);
//	printf("file: %s\n", file_string);

	char success = gnunet_search_server_communication_init(cfg);
	if(!success) {
		printf("Unable to connect to service.\n");
		GNUNET_SCHEDULER_shutdown();
		return;
	}
	gnunet_search_communication_listener_add(&gnunet_search_receive_handler);
	gnunet_search_server_communication_receive();

	if(!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_SEARCH))
		gnunet_search_transmit_keyword(keyword_string);
	else if(!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_ADD))
		gnunet_search_transmit_urls(file_string);

}

/**
 * @brief This function is the main function of the application.
 *
 * @param argc the number of arguments from the command line
 * @param argv the command line arguments
 * @return 0 in case of success, 1 on error
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
