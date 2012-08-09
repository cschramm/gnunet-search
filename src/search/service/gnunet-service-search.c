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
 * @file ext/gnunet-service-ext.c
 * @brief ext service implementation
 * @author Christian Grothoff
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>
#include <gnunet/gnunet_core_service.h>
#include "gnunet_protocols_search.h"

#include "service/url-processor/url-processor.h"
#include "service/util/service-util.h"
#include "service/client-communication/client-communication.h"
#include "../communication/communication.h"
#include "dht/dht.h"
#include "flooding/flooding.h"
#include "storage/storage.h"
#include "globals/globals.h"

#include <collections/arraylist/arraylist.h>


static void gnunet_search_process_flooding(char const *keyword) {
	gnunet_search_flooding_peer_request_flood(keyword, strlen(keyword) + 1);
}

/**
 * Handle message from client.
 *
 * @param cls closure
 * @param client identification of the client
 * @param message the actual message
 * @return GNUNET_OK to keep the connection open,
 *         GNUNET_SYSERR to close it (signal serious error)
 */
static void gnunet_search_client_message_handle(size_t size, void *buffer) {
	GNUNET_assert(size >= sizeof(struct search_command));

	struct search_command *cmd = (struct search_command*) buffer;

	GNUNET_assert(size == cmd->size);

	printf("Command: action = %u, size = %" PRIu64 "\n", cmd->action, cmd->size);

	if(cmd->action == GNUNET_SEARCH_ACTION_SEARCH) {
		char *keyword;
		gnunet_search_util_cmd_keyword_get(&keyword, cmd);

		printf("Searching keyword: %s...\n", keyword);

//		search_process(keyword);
		gnunet_search_process_flooding(keyword);

		free(keyword);
	}
	if(cmd->action == GNUNET_SEARCH_ACTION_ADD) {
		char **urls;
		size_t urls_length = gnunet_search_url_processor_cmd_urls_get(&urls, cmd);

		gnunet_search_dht_url_list_put(urls, urls_length, 2);

		gnunet_search_client_communication_send_result(NULL, 0, GNUNET_SEARCH_RESPONSE_TYPE_DONE);

		for(size_t i = 0; i < urls_length; ++i)
			free(urls[i]);
		free(urls);
	}

//	printf("blah :-)\n");
}

/**
 * Task run during shutdown.
 *
 * @param cls unused
 * @param tc unused
 */
static void shutdown_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	gnunet_search_dht_free();
	gnunet_search_client_communication_free();
	gnunet_search_flooding_free();
	gnunet_search_storage_free();

	GNUNET_CONFIGURATION_destroy(gnunet_search_globals_cfg);

	/*
	 * Todo: Scheduler?
	 */

//	exit(0);
}

/**
 * A client disconnected.  Remove all of its data structure entries.
 *
 * @param cls closure, NULL
 * @param client identification of the client
 */
static void handle_client_disconnect(void *cls, struct GNUNET_SERVER_Client * client) {
}

void gnunet_search_message_notification_handler(struct GNUNET_PeerIdentity const *sender,
		struct gnunet_search_flooding_message *flooding_message, size_t flooding_message_size) {
	switch(flooding_message->type) {
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST: {
			/*
			 * Todo: Security!!! (Länge muss gepüft werden)
			 */
			char *key = (char*)(flooding_message + 1);
//			printf("a: %s...\n", key);
			array_list_t *values = gnunet_search_storage_values_get(key);
			if(values) {
				char *values_serialized;
				size_t values_serialized_size = gnunet_search_storage_value_serialize(&values_serialized, values,
						GNUNET_SEARCH_FLOODING_MESSAGE_MAXIMAL_PAYLOAD_SIZE);

//				printf("b...\n");

				gnunet_search_flooding_peer_response_flood(values_serialized, values_serialized_size,
						flooding_message->flow_id);

				free(values_serialized);
			}
			break;
		}
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE: {
			void *data = flooding_message + 1;
			size_t data_size = flooding_message_size - sizeof(struct gnunet_search_flooding_message);
			gnunet_search_client_communication_send_result(data, data_size, GNUNET_SEARCH_RESPONSE_TYPE_RESULT);
			break;
		}
	}
}

/**
 * Process statistics requests.
 *
 * @param cls closure
 * @param server the initialized server
 * @param c configuration to use
 */
static void gnunet_search_service_run(void *cls, struct GNUNET_SERVER_Handle *server, const struct GNUNET_CONFIGURATION_Handle *c) {
	gnunet_search_client_communication_init();
	gnunet_search_communication_listener_add(&gnunet_search_client_message_handle);

	static const struct GNUNET_SERVER_MessageHandler handlers[] = { {
			&gnunet_search_client_communication_message_handle, NULL, GNUNET_MESSAGE_TYPE_SEARCH, 0 }, { NULL, NULL, 0,
			0 } };
	gnunet_search_globals_cfg = c;
	GNUNET_SERVER_add_handlers(server, handlers);
	GNUNET_SERVER_disconnect_notify(server, &handle_client_disconnect, NULL);

	gnunet_search_dht_init();

	gnunet_search_flooding_init();
	gnunet_search_handlers_set(&gnunet_search_message_notification_handler);

	gnunet_search_storage_init();

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &shutdown_task, NULL);
}

/**
 * The main function for the ext service.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int main(int argc, char * const *argv) {
	return (GNUNET_OK == GNUNET_SERVICE_run(argc, argv, "search", GNUNET_SERVICE_OPTION_NONE, &gnunet_search_service_run, NULL)) ? 0 : 1;
}

/* end of gnunet-service-ext.c */
