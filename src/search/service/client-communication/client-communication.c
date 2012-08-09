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

#include "gnunet_protocols_search.h"
#include "../../communication/communication.h"
#include "client-communication.h"

static struct GNUNET_SERVER_Client *_client;

void gnunet_search_client_communication_message_handle(void *cls, struct GNUNET_SERVER_Client *client,
		const struct GNUNET_MessageHeader *gnunet_message) {
	GNUNET_SERVER_receive_done(client, GNUNET_OK);
	GNUNET_SERVER_client_keep(client);
	_client = client;
	gnunet_search_communication_receive(gnunet_message);
}

static void gnunet_search_client_communication_request_notify_transmit_ready(size_t size, void *cls,
		size_t (*handler)(void*, size_t, void*), struct GNUNET_TIME_Relative max_delay) {
	GNUNET_SERVER_notify_transmit_ready(_client, size, max_delay, handler, cls);
}

void gnunet_search_client_communication_init() {
	gnunet_search_communication_init(&gnunet_search_client_communication_request_notify_transmit_ready);
}

void gnunet_search_client_communication_free() {
	gnunet_search_communication_free();
}

void gnunet_search_client_communication_send_result(void const *data, size_t size, char type) {

	size_t message_size = sizeof(struct search_response) + size;
	void *message_buffer = malloc(message_size);

	struct search_response *response = (struct search_response*) message_buffer;
	response->type = type;
	response->size = message_size;

	memcpy(response + 1, data, size);

	gnunet_search_communication_transmit(message_buffer, message_size);

	free(message_buffer);
}
