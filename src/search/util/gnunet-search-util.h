/*
 * gnunet-search-util.h
 *
 *  Created on: Aug 3, 2012
 *      Author: jucs
 */

#ifndef GNUNET_SEARCH_UTIL_H_
#define GNUNET_SEARCH_UTIL_H_

extern struct GNUNET_DHT_Handle *gnunet_search_dht_handle;

extern void gnunet_search_util_dht_url_list_put(char **urls, size_t size, unsigned int parameter);


#endif /* GNUNET_SEARCH_UTIL_H_ */
