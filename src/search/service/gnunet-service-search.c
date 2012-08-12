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

#include "service/util/service-util.h"
#include "service/client-communication/client-communication.h"
#include "dht/dht.h"
#include "flooding/flooding.h"
#include "storage/storage.h"
#include "globals/globals.h"

/**
 * Task run during shutdown.
 *
 * @param cls unused
 * @param tc unused
 */
static void gnunet_search_shutdown_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	gnunet_search_dht_free();
	gnunet_search_client_communication_free();
	gnunet_search_flooding_free();
	gnunet_search_storage_free();

	//GNUNET_CONFIGURATION_destroy(gnunet_search_globals_cfg);

	/*
	 * Todo: Scheduler?
	 */

	exit(0);
}

/**
 * A client disconnected.  Remove all of its data structure entries.
 *
 * @param cls closure, NULL
 * @param client identification of the client
 */
static void gnunet_search_client_disconnect_handle(void *cls, struct GNUNET_SERVER_Client * client) {
	gnunet_search_client_communication_flush();
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

	static const struct GNUNET_SERVER_MessageHandler handlers[] = { {
			&gnunet_search_client_communication_message_handle, NULL, GNUNET_MESSAGE_TYPE_SEARCH, 0 }, { NULL, NULL, 0,
			0 } };
	gnunet_search_globals_cfg = c;
	GNUNET_SERVER_add_handlers(server, handlers);
	GNUNET_SERVER_disconnect_notify(server, &gnunet_search_client_disconnect_handle, NULL);

	gnunet_search_storage_init();
	gnunet_search_dht_init();
	gnunet_search_flooding_init();

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &gnunet_search_shutdown_task, NULL);

//	gnunet_search_storage_key_value_add("hallo", "http://www.hallo.de");
}

/*
 * Todo: Assert => behandeln!
 */

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
