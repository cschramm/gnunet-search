/*
 * flooding.h
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
 */

#ifndef FLOODING_H_
#define FLOODING_H_

#define GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE 25
#define GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST 0
#define GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE 1

#include <stdint.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_core_service.h>

struct gnunet_search_flooding_message {
	uint64_t id;
	uint8_t ttl;
	uint8_t type;
};

extern void gnunet_search_flooding_init();
extern void gnunet_search_flooding_free();
extern void gnunet_search_flooding_peer_message_process(struct GNUNET_PeerIdentity *sender, const struct GNUNET_MessageHeader *message);
extern void gnunet_search_flooding_peer_request_message_flood(const struct GNUNET_MessageHeader *message);

#endif /* FLOODING_H_ */
