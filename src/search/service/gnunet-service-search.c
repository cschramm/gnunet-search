/**
 * @file search/service/gnunet-service-search.c
 * @author Julian Kranz
 *
 * @brief This file contains all functions pertaining to the GNUnet Search client's main component.
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

#include "service/util/service-util.h"
#include "service/client-communication/client-communication.h"
#include "dht/dht.h"
#include "flooding/flooding.h"
#include "storage/storage.h"
#include "globals/globals.h"

/**
 * @brief This function handles the shutdown of the application.
 *
 * @param cls the GNUnet closure
 * @param tc the GMUnet scheduler task context
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
 * @brief This function is the main function that will be run by the scheduler.
 *
 * @param cls the GNUnet closure
 * @param server the GNUnet server handle
 * @param cfg the GNUnet configuration
 */
static void gnunet_search_service_run(void *cls, struct GNUNET_SERVER_Handle *server, const struct GNUNET_CONFIGURATION_Handle *cfg) {
	gnunet_search_globals_cfg = cfg;

	gnunet_search_client_communication_init(server);

	gnunet_search_storage_init();
	gnunet_search_dht_init();
	gnunet_search_flooding_init();

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &gnunet_search_shutdown_task, NULL);

//	gnunet_search_storage_key_value_add("fiep", "http://www.fiep.de");
}

/**
 * @brief This function is the main function of the application.
 *
 * @param argc the number of arguments from the command line
 * @param argv the command line arguments
 * @return 0 in case of success, 1 on error
 */
int main(int argc, char * const *argv) {
	return (GNUNET_OK == GNUNET_SERVICE_run(argc, argv, "search", GNUNET_SERVICE_OPTION_NONE, &gnunet_search_service_run, NULL)) ? 0 : 1;
}
