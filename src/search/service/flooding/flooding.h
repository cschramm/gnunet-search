/**
 * @file search/service/flooding/flooding.h
 * @author Julian Kranz
 * @date 7.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search service's flooding component.
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

#ifndef FLOODING_H_
#define FLOODING_H_

#include <stdint.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_core_service.h>

/**
 * @brief This constant defines the size of the routing table.
 */
#define GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE 25
/**
 * @brief This constant defines a numerical code used used in a flooding message to define it as a request message.
 */
#define GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST 0
/**
 * @brief This constant defines a numerical code used used in a flooding message to define it as a response message.
 */
#define GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE 1

/**
 * @brief This constant defines the maximal usable payload size for a flooding message.
 */
#define GNUNET_SEARCH_FLOODING_MESSAGE_MAXIMAL_PAYLOAD_SIZE (GNUNET_SERVER_MAX_MESSAGE_SIZE - sizeof(struct gnunet_search_flooding_message) - sizeof(struct GNUNET_MessageHeader))

/**
 * @brief This data structure defines the header for a message exchanged between peers.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This data structure defines the header for a message exchanged between peers. It is used in a flooded request as well as in a routed response.
 */
struct __attribute__((__packed__)) gnunet_search_flooding_message {
	/**
	 * @brief This member stores the id of the flow the message belongs to.
	 *
	 * \latexonly \\ \\ \endlatexonly
	 * \em Detailed \em description \n
	 * This member stores the id of the flow the message belongs to. It can be chosen randomly at request creating but should be unique in the whole network.
	 * Using this id the response messages can be routed back to the requestor.
	 */
	uint64_t flow_id;
	/**
	 * @brief This member stores the TTL of the message.
	 *
	 * \latexonly \\ \\ \endlatexonly
	 * \em Detailed \em description \n
	 * This member stores the TTL of the message. Whenever the message is forwarded the TTL is decremented. If the TTL reaches zero the message is no longer forwarded.
	 */
	uint8_t ttl;
	/**
	 * @brief This member stores the type of the message - either GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST or GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE.
	 */
	uint8_t type;
};

extern void gnunet_search_flooding_init();
extern void gnunet_search_flooding_free();
extern void gnunet_search_flooding_peer_message_process(struct GNUNET_PeerIdentity const *sender,
		struct GNUNET_MessageHeader const *message);
extern void gnunet_search_flooding_peer_local_message_process(struct GNUNET_MessageHeader const *message);
extern void gnunet_search_flooding_peer_request_flood(void const *data, size_t data_size);
extern void gnunet_search_flooding_peer_response_send(void const *data, size_t data_size, uint64_t flow_id);
extern void gnunet_search_flooding_peer_data_send(void const *data, size_t data_size, uint8_t type, uint64_t flow_id);
//extern void gnunet_search_handlers_set(
//		void (*message_notification_handler)(struct GNUNET_PeerIdentity const *, struct gnunet_search_flooding_message *,
//				size_t size));

#endif /* FLOODING_H_ */
