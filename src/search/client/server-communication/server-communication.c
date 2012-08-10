/*
 * server-communication.c
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "server-communication.h"
#include "../../communication/communication.h"

static struct GNUNET_CLIENT_Connection *gnunet_search_server_communication_client_connection;

static void gnunet_search_server_communication_receive_response(void *cls,
		const struct GNUNET_MessageHeader *gnunet_message) {
	char more_messages = gnunet_search_communication_receive(gnunet_message);
	if(more_messages)
		GNUNET_CLIENT_receive(gnunet_search_server_communication_client_connection, &gnunet_search_server_communication_receive_response, NULL,
				GNUNET_TIME_relative_get_forever_());
}

static void gnunet_search_server_communication_request_notify_transmit_ready(size_t size, void *cls,
		size_t (*handler)(void*, size_t, void*), struct GNUNET_TIME_Relative max_delay) {
	GNUNET_CLIENT_notify_transmit_ready(gnunet_search_server_communication_client_connection, size, max_delay, 1, handler, cls);
}

void gnunet_search_server_communication_receive() {
	GNUNET_CLIENT_receive(gnunet_search_server_communication_client_connection, &gnunet_search_server_communication_receive_response, NULL,
			GNUNET_TIME_relative_get_forever_());
}

char gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg) {
	gnunet_search_server_communication_client_connection = GNUNET_CLIENT_connect("search", cfg);
	gnunet_search_communication_init(&gnunet_search_server_communication_request_notify_transmit_ready);
	return gnunet_search_server_communication_client_connection != NULL;
}

void gnunet_search_server_communication_free() {
	gnunet_search_communication_free();
}
