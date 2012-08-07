/*
 * flooding.h
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
 */

#ifndef FLOODING_H_
#define FLOODING_H_

#define GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE 25

#include <stdint.h>

struct gnunet_search_flooding_message {
	uint64_t id;
	uint8_t ttl;
	uint64_t size;
};

extern void gnunet_search_flooding_init();
extern void gnunet_search_flooding_free();
extern void gnunet_search_flooding_data_flood(struct GNUNET_PeerIdentity *source, void *data, size_t size);

#endif /* FLOODING_H_ */
