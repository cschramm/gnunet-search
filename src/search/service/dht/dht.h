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

extern void gnunet_search_util_dht_url_list_put(char **urls, size_t size, unsigned int parameter);
extern void gnunet_search_dht_monitor_put(void *cls, enum GNUNET_DHT_RouteOption options, enum GNUNET_BLOCK_Type type,
		uint32_t hop_count, uint32_t desired_replication_level, unsigned int path_length,
		const struct GNUNET_PeerIdentity *path, struct GNUNET_TIME_Absolute exp, const GNUNET_HashCode * key,
		const void *data, size_t size);

#endif /* DHT_H_ */
