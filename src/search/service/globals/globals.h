/*
 * globals.h
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <gnunet/platform.h>

extern struct GNUNET_CONFIGURATION_Handle const *gnunet_search_globals_cfg;
extern struct GNUNET_CORE_Handle *gnunet_search_globals_core_handle;
extern struct GNUNET_DHT_Handle *gnunet_search_dht_handle;

#endif /* GLOBALS_H_ */
