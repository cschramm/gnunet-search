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

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>
#include "gnunet_protocols_search.h"
#include "server-communication.h"
#include "../../communication/communication.h"

#include <collections/queue/queue.h>

static struct GNUNET_CLIENT_Connection *client_connection;

static void gnunet_search_server_communication_receive_response(void *cls, const struct GNUNET_MessageHeader *gnunet_message) {
	char more_messages = gnunet_search_communication_receive(gnunet_message, cls);
	if(more_messages)
		GNUNET_CLIENT_receive(client_connection, &gnunet_search_server_communication_receive_response, cls, GNUNET_TIME_relative_get_forever_());
}

static void gnunet_search_server_communication_request_notify_transmit_ready(size_t size, void *cls,
		size_t (*handler)(void*, size_t, void*)) {
	GNUNET_CLIENT_notify_transmit_ready(client_connection, size, GNUNET_TIME_relative_get_forever_(), 1, handler, cls);
}

void gnunet_search_server_communication_receive(void *cls) {
	GNUNET_CLIENT_receive(client_connection, &gnunet_search_server_communication_receive_response, cls,
			GNUNET_TIME_relative_get_forever_());
}

int gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg) {
	client_connection = GNUNET_CLIENT_connect("search", cfg);
	gnunet_search_communication_init(&gnunet_search_server_communication_request_notify_transmit_ready);
	return !client_connection;
}

void gnunet_search_server_communication_free() {
}
