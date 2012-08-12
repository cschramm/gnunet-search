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

#define GNUNET_SEARCH_FLOODING_MESSAGE_MAXIMAL_PAYLOAD_SIZE (GNUNET_SERVER_MAX_MESSAGE_SIZE - sizeof(struct gnunet_search_flooding_message) - sizeof(struct GNUNET_MessageHeader))

#include <stdint.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_core_service.h>

struct __attribute__((__packed__)) gnunet_search_flooding_message {
	uint64_t flow_id;
	uint8_t ttl;
	uint8_t type;
};

extern void gnunet_search_flooding_init();
extern void gnunet_search_flooding_free();
extern void gnunet_search_flooding_peer_message_process(struct GNUNET_PeerIdentity const *sender,
		struct GNUNET_MessageHeader const *message);
extern void gnunet_search_flooding_peer_request_message_flood(struct GNUNET_MessageHeader const *message);
extern void gnunet_search_flooding_peer_request_flood(void const *data, size_t data_size);
extern void gnunet_search_flooding_peer_response_flood(void const *data, size_t data_size, uint64_t flow_id);
extern void gnunet_search_flooding_peer_data_flood(void const *data, size_t data_size, uint8_t type, uint64_t flow_id);
//extern void gnunet_search_handlers_set(
//		void (*message_notification_handler)(struct GNUNET_PeerIdentity const *, struct gnunet_search_flooding_message *,
//				size_t size));

#endif /* FLOODING_H_ */
