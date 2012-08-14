/**
 * @file search/service/client-communication/client-communication.c
 * @author Julian Kranz
 * @date 4.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's client communication component.
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
#include <string.h>
#include <inttypes.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_protocols_search.h"
#include "../../communication/communication.h"
#include "../dht/dht.h"
#include "../util/service-util.h"
#include "../flooding/flooding.h"
#include "../url-processor/url-processor.h"
#include "../normalization/normalization.h"
#include "client-communication.h"

/**
 * @brief This variable stores a reference to the client the service is communicating with.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable stores a reference to the client the service is communicating with. It is important to note that the service can only communicate with one client
 * at a time.
 */
static struct GNUNET_SERVER_Client *gnunet_search_client_communication_client;

/**
 * @brief This constant defines the amount of time (measured in requests) the server keeps track of mappings between request and flow ids.
 */
#define GNUNET_SEARCH_CLIENT_COMMUNICATION_MAPPINGS_SIZE 15
/**
 * @brief This data structure is used to map a request id to a flow id.
 */
struct gnunet_search_client_communication_message_mapping {
	/**
	 * @brief This member stores the request id to map.
	 */
	uint16_t request_id;
	/**
	 * @brief This member stores the flow id the request id is mapped to.
	 */
	uint64_t flow_id;
};
/**
 * @brief This variable stores a reference to the mapping table used for translating between flow and request id.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable stores a reference to the mapping table used for translating between flow and request id. The request id is used by the client
 * to map a response to a specific request. In case such a request is a search request another id is created for the flooding. That id is called the flow id as it identifies
 * one flow across different routers. Every answer to a flooded request will contain that flow id. On arrival of such a answer the flow id has to be mapped back to the corresponding
 * request id in order to enable the service to create an appropriate answer for the client.
 */
static struct gnunet_search_client_communication_message_mapping *gnunet_search_client_communication_mappings;
/**
 * @brief This variable stores the current length of the mapping table introduced above.
 */
static size_t gnunet_search_client_communication_mappings_length;
/**
 * @brief This variable defines the next index to be overwritten in case a new mapping needs to be added to the table.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable defines the next index to be overwritten in case a new mapping needs to be added to the table. The tables implements a simple FIFO replacement strategy.
 */
static size_t gnunet_search_client_communication_mappings_index;

/**
 * @brief This function hands a keyword over to the flooding component to search for it.
 *
 * @param keyword the keyword to search for
 * @param flow_id the flow id to be used for the flow
 */
static void gnunet_search_client_communication_flooding_process(char const *keyword, uint64_t flow_id) {
	gnunet_search_flooding_peer_data_send(keyword, strlen(keyword) + 1, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST,
			flow_id);
//	gnunet_search_flooding_peer_request_flood(keyword, strlen(keyword) + 1);
}

//static char gnunet_search_client_uint64_t_compare(void *a, void *b) {
//	uint64_t *_a = (uint64_t*) a;
//	uint64_t *_b = (uint64_t*) b;
//	if(*_a < *_b)
//		return -1;
//	else if(*_a > *_b)
//		return 1;
//	else
//		return 0;
//}

/**
 * @brief This function handles a message from the client.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function handles a message from the client. It is important to note that a message may be fragmented and thus consist of more than one GNUnet messages.
 * The function extracts the action id from the header initiates the execution of the corresponding code. It therefor either adds a given set of URLs or floods
 * a search request.
 *
 * @param size the total size of the message; the function has to make sure that this matches the expected size given in the message's header.
 * @param buffer the buffer containing the message
 */
static void gnunet_search_client_message_handle(size_t size, void *buffer) {
	GNUNET_assert(size >= sizeof(struct search_command));
	if(size < sizeof(struct search_command))
		return;

	struct search_command *cmd = (struct search_command*) buffer;

	GNUNET_assert(size == cmd->size);
	if(size != cmd->size)
		return;

	printf("Command: action = %u, size = %" PRIu64 "\n", cmd->action, cmd->size);

	if(cmd->action == GNUNET_SEARCH_ACTION_SEARCH) {
		char *keyword;
		gnunet_search_util_cmd_keyword_get(&keyword, cmd, size);

		gnunet_search_normalization_keyword_normalize(keyword);

		printf("Searching keyword: %s...\n", keyword);

		uint64_t flow_id =  ((uint64_t) rand() << 32) | rand();
		gnunet_search_client_communication_mappings[gnunet_search_client_communication_mappings_index].flow_id = flow_id;
		gnunet_search_client_communication_mappings[gnunet_search_client_communication_mappings_index].request_id = cmd->id;
		gnunet_search_client_communication_mappings_index = (gnunet_search_client_communication_mappings_index + 1)%GNUNET_SEARCH_CLIENT_COMMUNICATION_MAPPINGS_SIZE;
		if(gnunet_search_client_communication_mappings_length < GNUNET_SEARCH_CLIENT_COMMUNICATION_MAPPINGS_SIZE)
			gnunet_search_client_communication_mappings_length++;

//		search_process(keyword);
		gnunet_search_client_communication_flooding_process(keyword, flow_id);

		GNUNET_free(keyword);
	}
	if(cmd->action == GNUNET_SEARCH_ACTION_ADD) {
		char **urls;
		size_t urls_length = gnunet_search_url_processor_cmd_urls_get(&urls, cmd);

		gnunet_search_dht_url_list_put(urls, urls_length, 2);

		gnunet_search_client_communication_send_result(NULL, 0, GNUNET_SEARCH_RESPONSE_TYPE_DONE, cmd->id);

		for(size_t i = 0; i < urls_length; ++i)
			GNUNET_free(urls[i]);
		GNUNET_free(urls);
	}
}

/**
 * @brief This function is used to call the appropriate GNUnet function in order to initiate message transmission.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function is used to call the appropriate GNUnet function in order to initiate message transmission. GNUnet offers different functions
 * for message transmission on the service and client side; while the functionality does not differ and the process steps are the same, different
 * API functions and data structures are offered. In order to nevertheless implement a generic communication component both the client and the
 * service have to implement their own generic handlers. These generic handlers then call the specific GNUnet API functions for transmission.
 *
 * @size the size of the message
 * @cls the GNUnet closure for the GNUnet API call
 * @handler the function within the generic communication component that handles the message transmission
 */
static void gnunet_search_client_communication_request_notify_transmit_ready(size_t size, void *cls,
		size_t (*handler)(void*, size_t, void*), struct GNUNET_TIME_Relative max_delay) {
	GNUNET_SERVER_notify_transmit_ready(gnunet_search_client_communication_client, size, max_delay, handler, cls);
}

/**
 * @brief This function handles the disconnection of a client.
 *
 * @param cls the GNUnet closure (NULL)
 * @param client identification of the disconnected client (not used since only one client is allowed to connect to the service at once here)
 */
static void gnunet_search_client_communication_disconnect_handle(void *cls, struct GNUNET_SERVER_Client *client) {
	gnunet_search_client_communication_flush();
}

/**
 * @brief This function handles the reception of a new GNUnet message from a client.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function handles the reception of a new GNUnet message from a client. It call a necessary GNUnet functions to acknowledge the reception
 * and then passes the message to the communication component.
 */
void gnunet_search_client_communication_message_handle(void *cls, struct GNUNET_SERVER_Client *client,
		const struct GNUNET_MessageHeader *gnunet_message) {
	GNUNET_SERVER_receive_done(client, GNUNET_OK);
	GNUNET_SERVER_client_keep(client);
	gnunet_search_client_communication_client = client;
	gnunet_search_communication_receive(gnunet_message);
}

/**
 * @brief This function initialises the client communication component.
 *
 * @param server a reference to the GNUnet server object needed to add handlers
 */
void gnunet_search_client_communication_init(struct GNUNET_SERVER_Handle *server) {
	gnunet_search_client_communication_mappings = (struct gnunet_search_client_communication_message_mapping*) GNUNET_malloc(
			sizeof(struct gnunet_search_client_communication_message_mapping)
					* GNUNET_SEARCH_CLIENT_COMMUNICATION_MAPPINGS_SIZE);
	gnunet_search_client_communication_mappings_index = 0;
	gnunet_search_client_communication_mappings_length = 0;
	gnunet_search_communication_init(&gnunet_search_client_communication_request_notify_transmit_ready);
	gnunet_search_communication_listener_add(&gnunet_search_client_message_handle);

	static const struct GNUNET_SERVER_MessageHandler handlers[] = { {
			&gnunet_search_client_communication_message_handle, NULL, GNUNET_MESSAGE_TYPE_SEARCH, 0 }, { NULL, NULL, 0,
			0 } };
	GNUNET_SERVER_add_handlers(server, handlers);
	GNUNET_SERVER_disconnect_notify(server, &gnunet_search_client_communication_disconnect_handle, NULL);
}

/**
 * @brief This function releases all resources held by the client communication component.
 */
void gnunet_search_client_communication_free() {
	GNUNET_free(gnunet_search_client_communication_mappings);

	gnunet_search_communication_free();
}

/**
 * @brief This function resets all active communication sessions.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function resets all active communication sessions. For this purpose it resets the id to flow id mappings table and flushes the
 * communication component.
 */
void gnunet_search_client_communication_flush() {
	gnunet_search_client_communication_mappings_length = 0;
	gnunet_search_client_communication_mappings_index = 0;

	gnunet_search_communication_flush();
}

/**
 * @brief This function sends a result generated by the service back to the client.
 *
 * @param data the data to send to the client
 * @param size the size of the data
 * @param the request id to use for the response
 */
void gnunet_search_client_communication_send_result(void const *data, size_t size, char type, uint16_t id) {
	size_t message_size = sizeof(struct search_response) + size;
	void *message_buffer = GNUNET_malloc(message_size);

	struct search_response *response = (struct search_response*) message_buffer;
	response->type = type;
	response->size = message_size;
	response->id = id;

	memcpy(response + 1, data, size);

	gnunet_search_communication_transmit(message_buffer, message_size);

	GNUNET_free(message_buffer);
}

/**
 * @brief This function looks up a request id by a given flow id using the mappings table.
 *
 * @param flow_id the flow id to map
 *
 * @return the corresponding request id or zero in case it is not found
 */
uint16_t gnunet_search_client_communication_by_flow_id_request_id_get(uint64_t flow_id) {
	uint16_t request_id = 0;
	for (size_t i = 0; i < gnunet_search_client_communication_mappings_length; ++i)
		if(gnunet_search_client_communication_mappings[i].flow_id == flow_id) {
			request_id = gnunet_search_client_communication_mappings[i].request_id;
			break;
		}
	return request_id;
}
