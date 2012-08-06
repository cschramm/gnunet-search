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

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>
#include "gnunet_protocols_search.h"

static size_t gnunet_search_client_communication_send_result_transmit_ready(void *cls, size_t size, void *buffer) {
	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) cls;
	size_t message_size = ntohs(header->size);

//	printf("Message size: %lu, available: %lu\n", message_size, size);
	GNUNET_assert(message_size <= size);

	memcpy(buffer, cls, message_size);

	return message_size;
}

void gnunet_search_client_communication_send_result(void const *data, size_t size, char type,
		struct GNUNET_SERVER_Client *client) {
	size_t message_size = sizeof(struct GNUNET_MessageHeader) + sizeof(struct message_header) + sizeof(struct search_response) + size;
	void *message_buffer = malloc(message_size);

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) message_buffer;
	header->size = htons(message_size);
	header->type = htons(GNUNET_MESSAGE_TYPE_SEARCH);

	struct message_header *msg_header = (struct messsage_header*)(header + 1);
	msg_header->flags = 0;

	struct search_response *response = (struct search_response*) (msg_header + 1);
	response->type = type;
	response->size = sizeof(struct search_response) + size;
//	response->flags = 0;

	memcpy(response + 1, data, size);

//	printf("Client: %lu\n", client);
//
//	printf("Requesting size: %lu\n", message_size);

	GNUNET_SERVER_notify_transmit_ready(client, message_size, GNUNET_TIME_relative_get_forever_(),
			&gnunet_search_client_communication_send_result_transmit_ready, message_buffer);
}
