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

#include <stdlib.h>
#include <stdio.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_client_lib.h>
#include "gnunet_search_service.h"
#include "gnunet_protocols_search.h"

#define GNUNET_SEARCH_ACTION_STRING_SEARCH "search"
#define GNUNET_SEARCH_ACTION_STRING_ADD "add"

static int ret;
static struct GNUNET_CLIENT_Connection *client_connection;

static char *action_string;
static char *file_string;
static char *keyword_string;

static size_t transmit_ready(void *cls, size_t size, void *buf) {

	size_t msg_size = sizeof(struct GNUNET_MessageHeader)
			+ sizeof(struct search_command);

	struct search_command *cmd = (struct search_command*) cls;
	msg_size += cmd->size;

	GNUNET_assert(size >= msg_size);

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) buf;
	header->type = GNUNET_MESSAGE_TYPE_SEARCH_URLS;
	header->size = htons(msg_size);

	memcpy(buf + sizeof(struct GNUNET_MessageHeader), cls,
			sizeof(struct search_command) + cmd->size);

	//printf("End of transmit_ready()...\n");

	return msg_size;
}

static void transmit_urls(const char *file) {

}

static void transmit_keyword(const char *keyword) {
	struct search_command cmd;

	size_t keyword_size = strlen(keyword) + 1;

	cmd.action = GNUNET_SEARCH_ACTION_SEARCH;
	cmd.size = keyword_size;

	void *data = malloc(sizeof(struct search_command) + keyword_size);

	memcpy(data, &cmd, sizeof(struct search_command));
	memcpy(data + sizeof(struct search_command), keyword, keyword_size);

	GNUNET_CLIENT_notify_transmit_ready(client_connection,
			sizeof(struct GNUNET_MessageHeader),
			GNUNET_TIME_relative_get_forever(), 1, &transmit_ready, data);
}

/**
 * Main function that will be run by the scheduler.
 *
 * @param cls closure
 * @param args remaining command-line arguments
 * @param cfgfile name of the configuration file used (for saving, can be NULL!)
 * @param cfg configuration
 */
static void run(void *cls, char * const *args, const char *cfgfile,
		const struct GNUNET_CONFIGURATION_Handle *cfg) {
	ret = 0;

//	printf("action: %s\n", action_string);
//	printf("file: %s\n", file_string);

	client_connection = GNUNET_CLIENT_connect("search", cfg);

	if (!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_ADD))
		transmit_keyword(keyword_string);
	else if (!strcmp(action_string, GNUNET_SEARCH_ACTION_STRING_SEARCH))
		transmit_urls(file_string);
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
	static const struct GNUNET_GETOPT_CommandLineOption options[] = { { 'a',
			"action", "search|add",
			gettext_noop("search for keyword or add list of urls"), 1,
			&GNUNET_GETOPT_set_string, &action_string }, { 'u', "urls",
			"path/to/file", gettext_noop("specify the file containing urls"), 1,
			&GNUNET_GETOPT_set_string, &file_string }, { 'k', "keyword",
			"keyword", gettext_noop("specify the keyword to search for"), 1,
			&GNUNET_GETOPT_set_string, &keyword_string },
			GNUNET_GETOPT_OPTION_END };
	return (GNUNET_OK
			== GNUNET_PROGRAM_run(argc, argv, "gnunet-search [options [value]]",
					gettext_noop("search"), options, &run, NULL)) ? ret : 1;
}

/* end of gnunet-search.c */
