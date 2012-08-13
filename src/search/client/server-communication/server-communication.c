/**
 * @file search/client/server-communication/server-communication.c
 * @author Julian Kranz
 * @date 5.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search client's server communication component.
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

#include "server-communication.h"
#include "../../communication/communication.h"

/**
 * @brief This variable stores a reference to the GNUnet client connection.
 */
static struct GNUNET_CLIENT_Connection *gnunet_search_server_communication_client_connection;

/**
 * @brief This is function is the handler used by GNUnet to tell the client about new messages.
 *
 * @param cls the GNUnet closure
 * @param gnunet_message the message newly received by GNUnet
 */
static void gnunet_search_server_communication_receive_response(void *cls,
		const struct GNUNET_MessageHeader *gnunet_message) {
	char more_messages = gnunet_search_communication_receive(gnunet_message);
	if(more_messages)
		GNUNET_CLIENT_receive(gnunet_search_server_communication_client_connection,
				&gnunet_search_server_communication_receive_response, NULL, GNUNET_TIME_relative_get_forever_());
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
static void gnunet_search_server_communication_request_notify_transmit_ready(size_t size, void *cls,
		size_t (*handler)(void*, size_t, void*), struct GNUNET_TIME_Relative max_delay) {
	GNUNET_CLIENT_notify_transmit_ready(gnunet_search_server_communication_client_connection, size, max_delay, 1,
			handler, cls);
}

/**
 * @brief This function is used by other components to initiate the recipience of a new message.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function is used by other components to initiate the recipience of a new message. It is important to note that such a message
 * can be fragmented and thus consist of a number of different GNUnet messages.
 */
void gnunet_search_server_communication_receive() {
	GNUNET_CLIENT_receive(gnunet_search_server_communication_client_connection,
			&gnunet_search_server_communication_receive_response, NULL, GNUNET_TIME_relative_get_forever_());
}

/**
 * @brief This function initialises the server communication component.
 *
 * @param cfg the GNUnet configuration handle
 *
 * @return a boolean value indicating whether the initialisation was successful (1) or failed (0)
 */
char gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg) {
	gnunet_search_server_communication_client_connection = GNUNET_CLIENT_connect("search", cfg);
	gnunet_search_communication_init(&gnunet_search_server_communication_request_notify_transmit_ready);
	return gnunet_search_server_communication_client_connection != NULL;
}

/**
 * @brief This function releases all resources held by the server communication component and shuts down the GNUnet client connection.
 */
void gnunet_search_server_communication_free() {
	if(gnunet_search_server_communication_client_connection)
		GNUNET_CLIENT_disconnect(gnunet_search_server_communication_client_connection);

	gnunet_search_communication_free();
}
