/*
 * globals.c
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
 */
#include "globals.h"

/**
 * Our configuration.
 */
struct GNUNET_CONFIGURATION_Handle const *gnunet_search_globals_cfg;
struct GNUNET_CORE_Handle const *gnunet_search_globals_core_handle;
struct GNUNET_DHT_Handle const *gnunet_search_dht_handle;
