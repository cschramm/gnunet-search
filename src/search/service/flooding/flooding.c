/*
 * flooding.c
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
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
 * mesage are enqueued in this queue and are subequently sent one after one.
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

struct gnunet_search_flooding_data_flood_parameters {
	struct GNUNET_PeerIdentity *sender;
	void *data;
	size_t size;
};

struct gnunet_search_flooding_routing_entry {
	uint64_t flow_id;
	struct GNUNET_PeerIdentity next_hop;
	uint8_t own_request;
};

static struct gnunet_search_flooding_routing_entry *gnunet_search_flooding_routing_table;
static size_t gnunet_search_flooding_routing_table_length;
static size_t gnunet_search_flooding_routing_table_index;

static struct GNUNET_CORE_Handle *gnunet_search_flooding_core_handle;

static void (*_gnunet_search_flooding_message_notification_handler)(struct GNUNET_PeerIdentity const *sender,
		struct gnunet_search_flooding_message *, size_t);

static void gnunet_search_flooding_queued_message_free_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	struct gnunet_search_flooding_queued_message *msg = (struct gnunet_search_flooding_queued_message*) cls;
	GNUNET_free(msg->buffer);
	GNUNET_free(msg->peer);
	GNUNET_free(msg);
}

static void gnunet_search_flooding_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc);

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

static void gnunet_search_flooding_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	if(!queue_get_length(gnunet_search_flooding_message_queue))
		return;

	struct gnunet_search_flooding_queued_message *msg = (struct gnunet_search_flooding_queued_message*) queue_dequeue(
			gnunet_search_flooding_message_queue);

	struct GNUNET_TIME_Relative max_delay = GNUNET_TIME_relative_get_minute_();
	struct GNUNET_TIME_Relative gct = GNUNET_TIME_relative_add(max_delay, GNUNET_TIME_relative_get_second_());

//	gnunet_search_communication_request_notify_transmit_ready(sizeof(struct GNUNET_MessageHeader) + msg->size, msg,
//			&gnunet_search_communication_transmit_ready, max_delay);
	GNUNET_CORE_notify_transmit_ready(gnunet_search_flooding_core_handle, 0, 0, max_delay, msg->peer, msg->size,
			&gnunet_search_flooding_notify_transmit_ready, msg->buffer);

	/*
	 * Todo: Save and free...
	 */
	GNUNET_SCHEDULER_add_delayed(gct, &gnunet_search_flooding_queued_message_free_task, msg);
}

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

static char gnunet_search_flooding_routing_table_id_contains(uint64_t flow_id) {
	return gnunet_search_flooding_routing_table_id_get_next_hop_index(NULL, flow_id);
}

static int gnunet_search_flooding_core_inbound_notify(void *cls, const struct GNUNET_PeerIdentity *other,
		const struct GNUNET_MessageHeader *message, const struct GNUNET_ATS_Information *atsi, unsigned int atsi_count) {
	gnunet_search_flooding_peer_message_process(other, message);
	return GNUNET_OK;
}

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

				gnunet_search_flooding_peer_response_flood(values_serialized, values_serialized_size,
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

static void gnunet_search_flooding_handlers_set(
		void (*message_notification_handler)(struct GNUNET_PeerIdentity const *,
				struct gnunet_search_flooding_message *, size_t)) {
	_gnunet_search_flooding_message_notification_handler = message_notification_handler;
}

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

void gnunet_search_flooding_peer_data_flood(void const *data, size_t data_size, uint8_t type, uint64_t flow_id) {
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

	gnunet_search_flooding_peer_request_message_flood(message);

	GNUNET_free(buffer);
}

void gnunet_search_flooding_peer_request_message_flood(struct GNUNET_MessageHeader const *message) {
	gnunet_search_flooding_peer_message_process(NULL, message);
}

void gnunet_search_flooding_peer_request_flood(void const *data, size_t data_size) {
	gnunet_search_flooding_peer_data_flood(data, data_size, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST,
			((uint64_t) rand() << 32) | rand());
}

void gnunet_search_flooding_peer_response_flood(void const *data, size_t data_size, uint64_t flow_id) {
	gnunet_search_flooding_peer_data_flood(data, data_size, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE, flow_id);
}
