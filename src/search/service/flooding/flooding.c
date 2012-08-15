/**
 * @file search/service/flooding/flooding.c
 * @author Julian Kranz
 * @date 7.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's flooding component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This file contains all functions pertaining to the GNUnet Search service's flooding component. This component handles the flooding
 * of data to all connected peers and routing answers to requests back to their original senders. It also takes care of answering incoming
 * requests from other peers querying the storage component for the associated keywords.
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
#include <endian.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

#include "gnunet_protocols_search.h"
#include "../client-communication/client-communication.h"
#include "../storage/storage.h"
#include "../globals/globals.h"
#include "flooding.h"

#include <collections/arraylist/arraylist.h>
#include <collections/queue/queue.h>

/**
 * @brief This variable implements an output queue for messages.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable implements an output queue for messages. Since GNUnet does not allow the user to
 * queue more than one message at a time it is important to handle this situation correctly. New
 * mesage are enqueued in this queue and are sent subequently one after one.
 */
static queue_t *gnunet_search_flooding_message_queue;

/**
 * @brief This data structure is used to combine all parameters needed for a message waiting in the output queue.
 */
struct gnunet_search_flooding_queued_message {
	/**
	 * @brief This member stores a reference to buffer to be sent.
	 */
	void *buffer;
	/**
	 * @brief This member stores the size of the buffer.
	 */
	size_t size;
	struct GNUNET_PeerIdentity *peer;
};

/**
 * @brief This data structure stores all information needed to flood data to a given peer.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This data structure stores all information needed to flood data to a given peer. It is necessary since GNUnet uses an
 * iteration handler to iterate over all connected peers. This handler allows to pass a reference to a closure object; in order
 * to be able to pass more than one parameter to that handler these parameters have to be combined in a structure.
 */
struct gnunet_search_flooding_data_flood_parameters {
	/**
	 * @brief This member stores a reference to sender peer that initiated the flooding.
	 *
	 * \latexonly \\ \\ \endlatexonly
	 * \em Detailed \em description \n
	 * This member stores a reference to sender peer that initiated the flooding. A value of NULL indicates that this flooding
	 * has been been initiated locally. The sender is needed in order to prevent flooding the message back to its orginator; this
	 * reduces the bandwidth needed.
	 */
	struct GNUNET_PeerIdentity *sender;
	/**
	 * @brief This member stores a reference to the data to flood.
	 */
	void *data;
	/**
	 * @brief This member stores the size of the data.
	 */
	size_t size;
};

/**
 * @brief This data structure represents an entry in the routing table.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This data structure represents an entry in the routing table. The routing table is used to be able to forward response messages back
 * to their originator. Whenever a request passes a node it remembers the sending node and the flow id. Since the response will carry
 * the same flow id the node is able to look up the next hop using the flow id of the response message.
 */
struct gnunet_search_flooding_routing_entry {
	/**
	 * @brief This member stores the flow (one request, possibly multiple responses) id associated with the message flow the routing entry is used for.
	 */
	uint64_t flow_id;
	/**
	 * @brief This member stores the next hop to forward response messages to.
	 */
	struct GNUNET_PeerIdentity next_hop;
	/**
	 * @brief This member stores a boolean value indicating whether the request originated locally. In that the data stored in the next_hop attribute is invalid.
	 */
	uint8_t own_request;
};

/**
 * @brief This variable stores a reference to the routing table.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable stores a reference to the routing table. It is implemented as a simple ring buffer using a FIFO replacement strategy. In case a response arrives for
 * some flow id that cannot be found in the routing table (since the entry has been replaced or the message is corrupt) the message is discarded.
 */
static struct gnunet_search_flooding_routing_entry *gnunet_search_flooding_routing_table;
/**
 * @brief This variable stores the current length of routing table; it has fixed maximal length and will never shrink.
 */
static size_t gnunet_search_flooding_routing_table_length;
/**
 * @brief This variable stores the next index of the routing table to be overwritten in case of a newly received request.
 */
static size_t gnunet_search_flooding_routing_table_index;

/**
 * @brief This variable stores a reference to the GNUnet core handle needed to communicate with other peers.
 */
static struct GNUNET_CORE_Handle *gnunet_search_flooding_core_handle;

/**
 * @brief This variable stores a reference to a function that handles a newly received flooded message.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable stores a reference to a function that handles a newly received flooded message. It was originally introduced to allow other components to register a
 * handler for flooded messages; in the end that was not needed since the messages are handled locally by the flooding component. The handler is called whenever a new
 * response arrives for a request that has been initiated locally by the current node or in case a request arrives. The latter is done to be able to generate responses
 * for requests.
 */
static void (*_gnunet_search_flooding_message_notification_handler)(struct GNUNET_PeerIdentity const *sender,
		struct gnunet_search_flooding_message *, size_t);

/**
 * @brief This function frees a previously queued message and its buffer.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function frees a previously queued message, its buffer and peer. GNUnet does not call transmit_ready() in case an error occurs.
 * In that case the transmit_ready() function cannot take care of freeing the message data structure and its attributes. In order to solve that problem
 * this handler is scheduled to be called after the transmit_ready() function call. As this is a ordinary scheduled task it is called
 * despite possible communication errors. For this reason the approch prevents memory leaks.
 *
 * @param cls the GNUnet closure containing a reference to message to be freed
 * @tc the GNUnet task context (not used)
 */
static void gnunet_search_flooding_queued_message_free_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	struct gnunet_search_flooding_queued_message *msg = (struct gnunet_search_flooding_queued_message*) cls;
	GNUNET_free(msg->buffer);
	GNUNET_free(msg->peer);
	GNUNET_free(msg);
}

static void gnunet_search_flooding_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc);

/**
 * @brief This function is called by GNUnet is case a new buffer is available for a message to be sent.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function is called by GNUnet is case a new buffer is available for a message to be sent. This function also takes
 * care of initiating the transmission of the next message waiting in the output queue.
 *
 * @param cls the GNUnet closure
 * @param size the amout of buffer space available
 * @param buffer the output buffer the message shall be written to
 *
 * @return the amout of buffer space written
 */
static size_t gnunet_search_flooding_notify_transmit_ready(void *cls, size_t size, void *buffer) {
	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) cls;
	size_t message_size = ntohs(header->size);

	GNUNET_assert(size >= message_size);
	if(size < message_size)
		return 0;

	memcpy(buffer, cls, message_size);
//	free(cls);

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_flooding_transmit_next, NULL);

	return message_size;
}

/**
 * @brief This function initiates the transmission of the next message.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function initiates the transmission of the next message. In order to do that it dequeues the message from the
 * output queue and calls the appropriate GNUnet function for the transmission of the message. The function is implemented as a GNUnet task; this is done in order to decouple it from the
 * transmit_ready() function call (see above).
 *
 * @param cls the GNUnet closure (not used)
 * @param tc the GNUnet task context (not used)
 */
static void gnunet_search_flooding_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	if(!queue_get_length(gnunet_search_flooding_message_queue))
		return;

	struct gnunet_search_flooding_queued_message *msg = (struct gnunet_search_flooding_queued_message*) queue_dequeue(
			gnunet_search_flooding_message_queue);

	struct GNUNET_TIME_Relative max_delay = GNUNET_TIME_relative_get_minute_();
	struct GNUNET_TIME_Relative gct = GNUNET_TIME_relative_add(max_delay, GNUNET_TIME_relative_get_second_());

	GNUNET_CORE_notify_transmit_ready(gnunet_search_flooding_core_handle, 0, 0, max_delay, msg->peer, msg->size,
			&gnunet_search_flooding_notify_transmit_ready, msg->buffer);

	/*
	 * Todo: Save and free...
	 */
	GNUNET_SCHEDULER_add_delayed(gct, &gnunet_search_flooding_queued_message_free_task, msg);
}

/**
 * @brief This function sends data to a peer.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function sends data to a peer. For this purpose it prepends the GNUnet message header and enqueues the message into the
 * output queue. After that it adds a new task to send the message. The caller has to take care of not trying to send a message
 * exceeding the allowed message size.
 *
 * @param peer the peer to send the message to
 * @param data the data to send
 * @param size the size of the data so send
 */
static void gnunet_search_flooding_to_peer_message_send(const struct GNUNET_PeerIdentity *peer, void *data, size_t size) {
	size_t message_size = sizeof(struct GNUNET_MessageHeader) + size;
	void *buffer = GNUNET_malloc(message_size);
	memcpy(buffer + sizeof(struct GNUNET_MessageHeader), data, size);

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) buffer;
	header->size = htons(message_size);
	header->type = htons(GNUNET_MESSAGE_TYPE_SEARCH_FLOODING);

	struct GNUNET_PeerIdentity *_peer = (struct GNUNET_PeerIdentity*) GNUNET_malloc(sizeof(struct GNUNET_PeerIdentity));
	memcpy(_peer, peer, sizeof(struct GNUNET_PeerIdentity));

	struct gnunet_search_flooding_queued_message *msg = (struct gnunet_search_flooding_queued_message*) GNUNET_malloc(
			sizeof(struct gnunet_search_flooding_queued_message));
	msg->buffer = buffer;
	msg->size = message_size;
	msg->peer = _peer;

	queue_enqueue(gnunet_search_flooding_message_queue, msg);

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_flooding_transmit_next, NULL);
}

/**
 * @brief This function is the GNUnet iteration handler called while iterating the connected peers.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function is the GNUnet iteration handler called while iterating the connected peers. It is used to flood data to all known peers.
 *
 * @param cls the GNUnet closure; it contains a reference to a data structure (see above) containing a parameters needed to send data to the current peer.
 * @param peer the peer of the current iteration
 * @param atsi a reference to the GNUnet ATS information (not used)
 * @param atsi_count the length of the ATS information (not used)
 */
static void gnunet_search_flooding_peer_iterate_handler(void *cls, const struct GNUNET_PeerIdentity *peer,
		const struct GNUNET_ATS_Information *atsi, unsigned int atsi_count) {
	if(!peer) {
		printf("Iterating done...\n");
		struct gnunet_search_flooding_data_flood_parameters *parameters =
				(struct gnunet_search_flooding_data_flood_parameters*) cls;
		GNUNET_free(parameters->data);
		if(parameters->sender)
			GNUNET_free(parameters->sender);
		GNUNET_free(parameters);
		return;
	}
	struct gnunet_search_flooding_data_flood_parameters *parameters =
			(struct gnunet_search_flooding_data_flood_parameters*) cls;

	if(parameters->sender && !GNUNET_CRYPTO_hash_cmp(&parameters->sender->hashPubKey, &peer->hashPubKey)) {
		printf("Skipping sender...\n");
		return;
	}

	struct GNUNET_CRYPTO_HashAsciiEncoded result;
	GNUNET_CRYPTO_hash_to_enc(&peer->hashPubKey, &result);
	printf("Flooding message to peer %.*s...\n", 104, (char*) &result);

	gnunet_search_flooding_to_peer_message_send(peer, parameters->data, parameters->size);
}

/**
 * @brief This function floods data to all known peers.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function floods data to all known peers. In order to do that it wraps up all parameters necessary in a data structure and initiates iterating
 * all peers using the appropriate GNUnet function.
 *
 * @param sender the peer that initiated the flooding; it may be NULL in case the request originated locally.
 * @param data the data to flood
 * @param size the size of the data to flood
 */
static void gnunet_search_flooding_data_flood(struct GNUNET_PeerIdentity const *sender, void *data, size_t size) {
	struct gnunet_search_flooding_data_flood_parameters *parameters =
			(struct gnunet_search_flooding_data_flood_parameters*) GNUNET_malloc(
					sizeof(struct gnunet_search_flooding_data_flood_parameters));
	struct GNUNET_PeerIdentity *_sender;
	if(sender) {
		_sender = (struct GNUNET_PeerIdentity*) malloc(sizeof(struct GNUNET_PeerIdentity));
		memcpy(_sender, sender, sizeof(struct GNUNET_PeerIdentity));
	} else
		_sender = NULL;

	parameters->sender = _sender;
	parameters->data = data;
	parameters->size = size;

	GNUNET_CORE_iterate_peers(gnunet_search_globals_cfg, &gnunet_search_flooding_peer_iterate_handler, parameters);
}

/**
 * @brief This function retrieves the index of the next hop from the routing table by a given flow id.
 *
 * @param index a reference to a memory location to store the index in; it may be NULL - in that case the index is not stored.
 * @param flow_id the flow id to search for
 *
 * @return a boolean value indicating success (1) or failure (0)
 */
static uint8_t gnunet_search_flooding_routing_table_id_get_next_hop_index(size_t *index, uint64_t flow_id) {
	for(size_t i = 0; i < gnunet_search_flooding_routing_table_length; ++i) {
		if(gnunet_search_flooding_routing_table[i].flow_id == flow_id) {
			if(index)
				*index = i;
			return 1;
		}
	}
	return 0;
}

/**
 * @brief This function tests whether an entry for a given flow id is contained in the routing table.
 *
 * @param flow_id the flow id to search for
 */
static char gnunet_search_flooding_routing_table_id_contains(uint64_t flow_id) {
	return gnunet_search_flooding_routing_table_id_get_next_hop_index(NULL, flow_id);
}

/**
 * @brief This function receives a new message from a peer.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function receives a new message from a peer. It then initiates the processing of the message.
 *
 * @param cls the GNUnet closure (not used)
 * @param other the peer that sent the message
 * @param message the message received
 * @param atsi a reference to the GNUnet ATS information (not used)
 * @param atsi_count the length of the ATS information (not used)
 */
static int gnunet_search_flooding_core_inbound_notify(void *cls, const struct GNUNET_PeerIdentity *other,
		const struct GNUNET_MessageHeader *message, const struct GNUNET_ATS_Information *atsi, unsigned int atsi_count) {
	gnunet_search_flooding_peer_message_process(other, message);
	return GNUNET_OK;
}

/**
 * @brief This function is the handler to be called for a new request or a response destined for this node.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function is the handler to be called for a new request or a response destined for this node. In case a request is received it tries to find URLs for
 * the requested keyword using the storage component. In the event the search is successful the function creates a response message and sends it back to
 * the originator of the request. In case a response is received its data is delivered to the client using the client communication component.
 *
 * @param sender the sender of the message (not used)
 * @param flooding_message the flooding message received
 * @param flooding_message_size the size of the flooding message
 */
static void gnunet_search_flooding_message_notification_handler(struct GNUNET_PeerIdentity const *sender,
		struct gnunet_search_flooding_message *flooding_message, size_t flooding_message_size) {
	switch(flooding_message->type) {
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST: {
			char *key = (char*) (flooding_message + 1);

			/*
			 * Security, data from network
			 */
			char sane = 0;
			char *end = (char*) flooding_message + flooding_message_size;
			for(char *current = key; current < end; ++current)
				if(*current == 0) {
					sane = 1;
					break;
				}
			if(!sane) {
				printf("Fatal error: Invalid data");
				return;
			}

			array_list_t *values = gnunet_search_storage_values_get(key);
			if(values) {
				char *values_serialized;
				size_t values_serialized_size = gnunet_search_storage_value_serialize(&values_serialized, values,
						GNUNET_SEARCH_FLOODING_MESSAGE_MAXIMAL_PAYLOAD_SIZE);

				gnunet_search_flooding_peer_response_send(values_serialized, values_serialized_size,
						be64toh(flooding_message->flow_id));

				GNUNET_free(values_serialized);
			}
			break;
		}
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE: {
			void *data = flooding_message + 1;
			size_t data_size = flooding_message_size - sizeof(struct gnunet_search_flooding_message);

			uint16_t request_id = gnunet_search_client_communication_by_flow_id_request_id_get(
					be64toh(flooding_message->flow_id));

			gnunet_search_client_communication_send_result(data, data_size, GNUNET_SEARCH_RESPONSE_TYPE_RESULT,
					request_id);
			break;
		}
	}
}

/**
 * @brief This function sets the handler called in the event of new requests or locally destined answers.
 *
 * @param message_notification_handler a reference to the handler to set
 */
static void gnunet_search_flooding_handlers_set(
		void (*message_notification_handler)(struct GNUNET_PeerIdentity const *,
				struct gnunet_search_flooding_message *, size_t)) {
	_gnunet_search_flooding_message_notification_handler = message_notification_handler;
}

/**
 * @brief This function initialises the flooding component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function initialises the flooding component. It also connects to the GNUnet core.
 */
void gnunet_search_flooding_init() {
	gnunet_search_flooding_message_queue = queue_construct();
	gnunet_search_flooding_routing_table = (struct gnunet_search_flooding_routing_entry *) GNUNET_malloc(
			sizeof(struct gnunet_search_flooding_routing_entry) * GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE);
	gnunet_search_flooding_routing_table_length = 0;
	gnunet_search_flooding_routing_table_index = 0;
	_gnunet_search_flooding_message_notification_handler = NULL;

	static struct GNUNET_CORE_MessageHandler core_handlers[] = { { &gnunet_search_flooding_core_inbound_notify,
			GNUNET_MESSAGE_TYPE_SEARCH_FLOODING, 0 }, { NULL, 0, 0 } };

	gnunet_search_flooding_core_handle = GNUNET_CORE_connect(gnunet_search_globals_cfg, 42, NULL, NULL, NULL, NULL,
			NULL/*&gnunet_search_flooding_core_inbound_notify*/, 0, NULL, 0, core_handlers);

	gnunet_search_flooding_handlers_set(&gnunet_search_flooding_message_notification_handler);
}

/**
 * @brief This function releases all resources held by the flooding component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function releases all resources held by the flooding component. It also disconnects from the GNUnet core and flushes the output queue.
 */
void gnunet_search_flooding_free() {
	GNUNET_CORE_disconnect(gnunet_search_flooding_core_handle);

	GNUNET_free(gnunet_search_flooding_routing_table);

	while(queue_get_length(gnunet_search_flooding_message_queue)) {
		struct gnunet_search_flooding_queued_message *msg =
				(struct gnunet_search_flooding_queued_message *) queue_dequeue(gnunet_search_flooding_message_queue);
		GNUNET_free(msg->buffer);
		GNUNET_free(msg);
	}
}

/**
 * @brief This function processes a message.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function processes a message. In case the message is a request it first checks whether the routing table already contains the flow id - in that case the
 * same request has already been seen before and thus has been flooded in a cycle; such a message is discarded. Otherwise the new flow is entered into the routing
 * table. Afterwards the request is passed to the handler that processes the search for the keyword included in the request (see above). Independent of the result
 * of the search the request's TTL is decremented and it is - in case the resulting TTL is greater than zero - flooded to all neighbouring peers; thus a request
 * always either reaches all peers or is discarded because of an exceeded TTL - it is not stopped because of a peer knowing an answer. The reason for that behaviour is that there might
 * be multiple peers with different answers all of which the sender is interested in. In case the message is a response the corresponding entry in the routing table is
 * searched. If no entry is found the message is discarded; otherwise the message is either passed to the message notification handler (in case of an answer to a request
 * originating at local node) or forwarded to the next hop according to the entry of the routing table.
 *
 * @param sender the sender peer of the message; this parameter has to be NULL in case the message is a request originating locally
 * @param message the message to process
 */
void gnunet_search_flooding_peer_message_process(struct GNUNET_PeerIdentity const *sender,
		struct GNUNET_MessageHeader const *message) {
	size_t message_size = ntohs(message->size);

	GNUNET_assert(message_size >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct gnunet_search_flooding_message));
	if(message_size < sizeof(struct GNUNET_MessageHeader) + sizeof(struct gnunet_search_flooding_message))
		return;

	struct gnunet_search_flooding_message *flooding_message = (struct gnunet_search_flooding_message*) (message + 1);

//	printf("data^2: %s\n", flooding_message + 1);

	size_t flooding_message_size = message_size - sizeof(struct GNUNET_MessageHeader);
	uint64_t flooding_message_flow_id_host = be64toh(flooding_message->flow_id);

	switch(flooding_message->type) {
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST: {
			if(gnunet_search_flooding_routing_table_id_contains(flooding_message_flow_id_host)) {
				printf("Message cycle; discarding...\n");
				break;
			}

			gnunet_search_flooding_routing_table[gnunet_search_flooding_routing_table_index].flow_id =
					flooding_message_flow_id_host;
			gnunet_search_flooding_routing_table[gnunet_search_flooding_routing_table_index].own_request = sender
					== NULL;
			if(!gnunet_search_flooding_routing_table[gnunet_search_flooding_routing_table_index].own_request)
				memcpy(&gnunet_search_flooding_routing_table[gnunet_search_flooding_routing_table_index].next_hop,
						sender, sizeof(struct GNUNET_PeerIdentity));
			gnunet_search_flooding_routing_table_index = (gnunet_search_flooding_routing_table_index + 1)
					% GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE;
			if(gnunet_search_flooding_routing_table_length < GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE)
				gnunet_search_flooding_routing_table_length++;

			if(_gnunet_search_flooding_message_notification_handler)
				_gnunet_search_flooding_message_notification_handler(sender, flooding_message, flooding_message_size);

			struct gnunet_search_flooding_message *output_flooding_message =
					(struct gnunet_search_flooding_message *) GNUNET_malloc(flooding_message_size);
			memcpy(output_flooding_message, flooding_message, flooding_message_size);
			output_flooding_message->ttl--;
			if(output_flooding_message->ttl > 0)
				gnunet_search_flooding_data_flood(sender, output_flooding_message, flooding_message_size);
			break;
		}
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE: {
			size_t next_hop_index;
			uint8_t found = gnunet_search_flooding_routing_table_id_get_next_hop_index(&next_hop_index,
					flooding_message_flow_id_host);
			if(!found) {
				printf("Unknown flow; aborting...\n");
				break;
			}
			if(gnunet_search_flooding_routing_table[next_hop_index].own_request) {
				printf("Yippie, this is response to my request :-).\n");
				if(_gnunet_search_flooding_message_notification_handler)
					_gnunet_search_flooding_message_notification_handler(sender, flooding_message,
							flooding_message_size);
			} else {
				struct GNUNET_PeerIdentity const *next_hop =
						&gnunet_search_flooding_routing_table[next_hop_index].next_hop;
				struct gnunet_search_flooding_message *output_flooding_message =
						(struct gnunet_search_flooding_message *) GNUNET_malloc(flooding_message_size);
				memcpy(output_flooding_message, flooding_message, flooding_message_size);

				printf("Relaying answer to original sender of request...\n");
				struct GNUNET_CRYPTO_HashAsciiEncoded result;
				GNUNET_CRYPTO_hash_to_enc(&sender->hashPubKey, &result);
				printf("Received from peer: %.*s...\n", 104, (char*) &result);
				GNUNET_CRYPTO_hash_to_enc(&next_hop->hashPubKey, &result);
				printf("Relaying to peer: %.*s...\n", 104, (char*) &result);

				output_flooding_message->ttl--;
				if(output_flooding_message->ttl > 0)
					gnunet_search_flooding_to_peer_message_send(next_hop, output_flooding_message,
							flooding_message_size);
				GNUNET_free(output_flooding_message);
			}
		}
	}
}

/**
 * @brief This function is a wrapper function to process a message originating locally.
 *
 * @param message the message to process
 */
void gnunet_search_flooding_peer_local_message_process(struct GNUNET_MessageHeader const *message) {
	gnunet_search_flooding_peer_message_process(NULL, message);
}

/**
 * @brief This function sends data originating locally either by flooding (in case of a request) or by forwarding (in case of a response).
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function sends data originating locally either by flooding (in case of a request) or by forwarding (in case of a response). It therefore
 * prepends the GNUnet message header and the flooding message header.
 *
 * @param data the data so send
 * @param data_size the size of the data; the caller should care about the maximal possible size using the GNUNET_SEARCH_FLOODING_MESSAGE_MAXIMAL_PAYLOAD_SIZE constant.
 * @param type the type of the message - either GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST or GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE
 * @param flow_id the flow id to use
 */
void gnunet_search_flooding_peer_data_send(void const *data, size_t data_size, uint8_t type, uint64_t flow_id) {
	size_t message_total_size = sizeof(struct GNUNET_MessageHeader) + sizeof(struct gnunet_search_flooding_message)
			+ data_size;

	GNUNET_assert(message_total_size <= GNUNET_SERVER_MAX_MESSAGE_SIZE);
	if(message_total_size > GNUNET_SERVER_MAX_MESSAGE_SIZE)
		return;

	void *buffer = GNUNET_malloc(message_total_size);

	struct GNUNET_MessageHeader *message = (struct GNUNET_MessageHeader *) buffer;
	message->size = htons((uint16_t) message_total_size);
	message->type = htons(GNUNET_MESSAGE_TYPE_SEARCH_FLOODING);

	struct gnunet_search_flooding_message *flooding_message = (struct gnunet_search_flooding_message*) (message + 1);
	flooding_message->flow_id = htobe64(flow_id);
	flooding_message->ttl = 16;
	flooding_message->type = type;

	memcpy(flooding_message + 1, data, data_size);

	gnunet_search_flooding_peer_local_message_process(message);

	GNUNET_free(buffer);
}

/**
 * @brief This function sends data using a request message and a random flow id.
 *
 * @param data the data to send
 * @param data_size the size of the data; the caller should care about the maximal possible size using the GNUNET_SEARCH_FLOODING_MESSAGE_MAXIMAL_PAYLOAD_SIZE constant.
 */
void gnunet_search_flooding_peer_request_flood(void const *data, size_t data_size) {
	gnunet_search_flooding_peer_data_send(data, data_size, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST,
			((uint64_t) rand() << 32) | rand());
}

/**
 * @brief This function sends data using a response message.
 *
 * @param data the data to send
 * @param data_size the size of the data; the caller should care about the maximal possible size using the GNUNET_SEARCH_FLOODING_MESSAGE_MAXIMAL_PAYLOAD_SIZE constant.
 * @param flow_id the flow id to use
 */
void gnunet_search_flooding_peer_response_send(void const *data, size_t data_size, uint64_t flow_id) {
	gnunet_search_flooding_peer_data_send(data, data_size, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE, flow_id);
}
