/*
 * client-communication.c
 *
 *  Created on: Aug 4, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "gnunet_protocols_search.h"
#include "../../communication/communication.h"
#include "../dht/dht.h"
#include "../util/service-util.h"
#include "../flooding/flooding.h"
#include "../url-processor/url-processor.h"
#include "../normalization/normalization.h"
#include "client-communication.h"

static struct GNUNET_SERVER_Client *gnunet_search_client_communication_client;

#define GNUNET_SEARCH_CLIENT_COMMUNICATION_MAPPINGS_SIZE 15
struct gnunet_search_client_communication_message_mapping {
	uint16_t request_id;
	uint64_t flow_id;
};
static struct gnunet_search_client_communication_message_mapping *gnunet_search_client_communication_mappings;
static size_t gnunet_search_client_communication_mappings_length;
static size_t gnunet_search_client_communication_mappings_index;

static void gnunet_search_client_communication_flooding_process(char const *keyword, uint64_t flow_id) {
	gnunet_search_flooding_peer_data_flood(keyword, strlen(keyword) + 1, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST,
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

		free(keyword);
	}
	if(cmd->action == GNUNET_SEARCH_ACTION_ADD) {
		char **urls;
		size_t urls_length = gnunet_search_url_processor_cmd_urls_get(&urls, cmd);

		gnunet_search_dht_url_list_put(urls, urls_length, 2);

		gnunet_search_client_communication_send_result(NULL, 0, GNUNET_SEARCH_RESPONSE_TYPE_DONE, cmd->id);

		for(size_t i = 0; i < urls_length; ++i)
			free(urls[i]);
		free(urls);
	}
}

static void gnunet_search_client_communication_request_notify_transmit_ready(size_t size, void *cls,
		size_t (*handler)(void*, size_t, void*), struct GNUNET_TIME_Relative max_delay) {
	GNUNET_SERVER_notify_transmit_ready(gnunet_search_client_communication_client, size, max_delay, handler, cls);
}

void gnunet_search_client_communication_message_handle(void *cls, struct GNUNET_SERVER_Client *client,
		const struct GNUNET_MessageHeader *gnunet_message) {
	GNUNET_SERVER_receive_done(client, GNUNET_OK);
	GNUNET_SERVER_client_keep(client);
	gnunet_search_client_communication_client = client;
	gnunet_search_communication_receive(gnunet_message);
}

void gnunet_search_client_communication_init() {
	gnunet_search_client_communication_mappings = (struct gnunet_search_client_communication_message_mapping*) malloc(
			sizeof(struct gnunet_search_client_communication_message_mapping)
					* GNUNET_SEARCH_CLIENT_COMMUNICATION_MAPPINGS_SIZE);
	gnunet_search_client_communication_mappings_index = 0;
	gnunet_search_client_communication_mappings_length = 0;
	gnunet_search_communication_init(&gnunet_search_client_communication_request_notify_transmit_ready);
	gnunet_search_communication_listener_add(&gnunet_search_client_message_handle);
}

void gnunet_search_client_communication_free() {
	free(gnunet_search_client_communication_mappings);

	gnunet_search_communication_free();
}

void gnunet_search_client_communication_flush() {
	gnunet_search_client_communication_mappings_length = 0;
	gnunet_search_client_communication_mappings_index = 0;

	gnunet_search_communication_flush();
}

void gnunet_search_client_communication_send_result(void const *data, size_t size, char type, uint16_t id) {
	size_t message_size = sizeof(struct search_response) + size;
	void *message_buffer = malloc(message_size);

	struct search_response *response = (struct search_response*) message_buffer;
	response->type = type;
	response->size = message_size;
	response->id = id;

	memcpy(response + 1, data, size);

	gnunet_search_communication_transmit(message_buffer, message_size);

	free(message_buffer);
}

uint16_t gnunet_search_client_communication_by_flow_id_request_id_get(uint64_t flow_id) {
	uint16_t request_id = 0;
	for (size_t i = 0; i < gnunet_search_client_communication_mappings_length; ++i)
		if(gnunet_search_client_communication_mappings[i].flow_id == flow_id) {
			request_id = gnunet_search_client_communication_mappings[i].request_id;
			break;
		}
	return request_id;
}
