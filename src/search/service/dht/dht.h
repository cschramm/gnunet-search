/*
 * dht.h
 *
 *  Created on: Aug 8, 2012
 *      Author: jucs
 */

#ifndef DHT_H_
#define DHT_H_

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>

extern void gnunet_search_dht_url_list_put(char **urls, size_t size, unsigned int parameter);
extern void gnunet_search_dht_init();
extern void gnunet_search_dht_free();

#endif /* DHT_H_ */
