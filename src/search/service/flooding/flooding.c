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

#include "gnunet_protocols_search.h"
#include "../globals/globals.h"
#include "flooding.h"

struct gnunet_search_flooding_data_flood_parameters {
	struct GNUNET_PeerIdentity const *sender;
	void *data;
	size_t size;
};

struct gnunet_search_flooding_routing_entry {
	uint64_t flow_id;
	struct GNUNET_PeerIdentity const *next_hop;
};

static struct gnunet_search_flooding_routing_entry *gnunet_search_flooding_routing_table;
static size_t gnunet_search_flooding_routing_table_length;
static size_t gnunet_search_flooding_routing_table_index;

static struct GNUNET_CORE_Handle *gnunet_search_dht_core_handle;

static void (*_message_notification_handler)(struct GNUNET_PeerIdentity const *sender, struct gnunet_search_flooding_message *, size_t);

static size_t gnunet_search_flooding_notify_transmit_ready(void *cls, size_t size, void *buffer) {
	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) cls;
	size_t message_size = ntohs(header->size);

	GNUNET_assert(size >= message_size);

	memcpy(buffer, cls, message_size);
//	free(cls);
	return message_size;
}

static void gnunet_search_flooding_buffer_free_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	free(cls);
}

static void gnunet_search_flooding_to_peer_message_send(const struct GNUNET_PeerIdentity *peer, void *data, size_t size) {
	size_t message_size = sizeof(struct GNUNET_MessageHeader) + size;
	void *buffer = malloc(message_size);
	memcpy(buffer + sizeof(struct GNUNET_MessageHeader),data, size);

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) buffer;
	header->size = htons(message_size);
	header->type = htons(GNUNET_MESSAGE_TYPE_SEARCH_FLOODING);

	struct GNUNET_TIME_Relative gct = GNUNET_TIME_relative_add(GNUNET_TIME_relative_get_minute_(), GNUNET_TIME_relative_get_second_());

	GNUNET_CORE_notify_transmit_ready(gnunet_search_dht_core_handle, 0, 0, GNUNET_TIME_relative_get_minute_(),
			peer, message_size, &gnunet_search_flooding_notify_transmit_ready, buffer);
	GNUNET_SCHEDULER_add_delayed(gct, &gnunet_search_flooding_buffer_free_task, buffer);
}

static void gnunet_search_flooding_peer_iterate_handler(void *cls, const struct GNUNET_PeerIdentity *peer,
		const struct GNUNET_ATS_Information *atsi, unsigned int atsi_count) {
	if(!peer) {
		printf("Iterating done...\n");
		struct gnunet_search_flooding_data_flood_parameters *parameters =
				(struct gnunet_search_flooding_data_flood_parameters*) cls;
		free(parameters->data);
		free(parameters);
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
	printf("Flooding message to peer %.*s...\n", 104, (char*)&result);

	gnunet_search_flooding_to_peer_message_send(peer, parameters->data, parameters->size);
}

static void gnunet_search_flooding_data_flood(struct GNUNET_PeerIdentity const *sender, void *data, size_t size) {
	struct gnunet_search_flooding_data_flood_parameters *parameters =
			(struct gnunet_search_flooding_data_flood_parameters*) malloc(
					sizeof(struct gnunet_search_flooding_data_flood_parameters));
	parameters->sender = sender;
	parameters->data = data;
	parameters->size = size;

	GNUNET_CORE_iterate_peers(gnunet_search_globals_cfg, &gnunet_search_flooding_peer_iterate_handler, parameters);
}

static uint8_t gnunet_search_flooding_routing_table_id_get_next_hop_index(size_t *index, uint64_t flow_id) {
	for (size_t i = 0; i < gnunet_search_flooding_routing_table_length; ++i) {
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

static int gnunet_search_flooding_core_inbound_notify(void *cls, const struct GNUNET_PeerIdentity *other, const struct GNUNET_MessageHeader *message,
		const struct GNUNET_ATS_Information *atsi, unsigned int atsi_count) {
	gnunet_search_flooding_peer_message_process(other, message);
	return GNUNET_OK;
}

void gnunet_search_flooding_init() {
	gnunet_search_flooding_routing_table = (struct gnunet_search_flooding_routing_entry *) malloc(
			sizeof(struct gnunet_search_flooding_routing_entry *) * GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE);
	gnunet_search_flooding_routing_table_length = 0;
	gnunet_search_flooding_routing_table_index = 0;
	_message_notification_handler = NULL;

	static struct GNUNET_CORE_MessageHandler core_handlers[] = { { &gnunet_search_flooding_core_inbound_notify,
			GNUNET_MESSAGE_TYPE_SEARCH_FLOODING, 0 },
			{ NULL, 0, 0 } };

	gnunet_search_dht_core_handle = GNUNET_CORE_connect(gnunet_search_globals_cfg, 42, NULL, NULL, NULL, NULL,
			NULL/*&gnunet_search_flooding_core_inbound_notify*/, 0, NULL, 0, core_handlers);
}

void gnunet_search_flooding_free() {
	GNUNET_CORE_disconnect(gnunet_search_dht_core_handle);

	free(gnunet_search_flooding_routing_table);
}

void gnunet_search_flooding_peer_message_process(struct GNUNET_PeerIdentity const *sender, struct GNUNET_MessageHeader const *message) {
	size_t message_size = ntohs(message->size);

	/*
	 * Todo: Security?
	 * Todo: Byte order for IDs
	 */
	GNUNET_assert(message_size >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct gnunet_search_flooding_message));

	struct gnunet_search_flooding_message *flooding_message = (struct gnunet_search_flooding_message*)(message + 1);

//	printf("data^2: %s\n", flooding_message + 1);

	size_t flooding_message_size = message_size - sizeof(struct GNUNET_MessageHeader);

	switch(flooding_message->type) {
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST: {
			if(gnunet_search_flooding_routing_table_id_contains(flooding_message->flow_id)) {
				printf("Message cycle; discarding...\n");
				break;
			}

			gnunet_search_flooding_routing_table[gnunet_search_flooding_routing_table_index].flow_id = flooding_message->flow_id;
			gnunet_search_flooding_routing_table[gnunet_search_flooding_routing_table_index].next_hop = sender;
			gnunet_search_flooding_routing_table_index = (gnunet_search_flooding_routing_table_index + 1)%GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE;
			if(gnunet_search_flooding_routing_table_length < GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE)
				gnunet_search_flooding_routing_table_length++;

			if(_message_notification_handler)
				_message_notification_handler(sender, flooding_message, flooding_message_size);

			struct gnunet_search_flooding_message *output_flooding_message = (struct gnunet_search_flooding_message *)malloc(flooding_message_size);
			memcpy(output_flooding_message, flooding_message, flooding_message_size);
			output_flooding_message->ttl--;
			if(output_flooding_message->ttl > 0)
				gnunet_search_flooding_data_flood(sender, output_flooding_message, flooding_message_size);
			break;
		}
		case GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE: {
			size_t next_hop_index;
			uint8_t found = gnunet_search_flooding_routing_table_id_get_next_hop_index(&next_hop_index, flooding_message->flow_id);
			if(!found) {
				printf("Unknown flow; aborting...\n");
				break;
			}
			struct GNUNET_PeerIdentity const *next_hop = gnunet_search_flooding_routing_table[next_hop_index].next_hop;
			if(!next_hop) {
				printf("Yippie, this is response to my request :-).\n");
				if(_message_notification_handler)
					_message_notification_handler(sender, flooding_message, flooding_message_size);
			}
			else {
				struct gnunet_search_flooding_message *output_flooding_message = (struct gnunet_search_flooding_message *)malloc(flooding_message_size);
				memcpy(output_flooding_message, flooding_message, flooding_message_size);

				output_flooding_message->ttl--;
				if(output_flooding_message->ttl > 0)
					gnunet_search_flooding_to_peer_message_send(next_hop, output_flooding_message, flooding_message_size);
				free(output_flooding_message);
			}
		}
	}
}

static void gnunet_search_flooding_peer_data_flood(void const *data, size_t data_size, uint8_t type, uint64_t flow_id) {
	size_t message_total_size = sizeof(struct GNUNET_MessageHeader) + sizeof(struct gnunet_search_flooding_message) + data_size;

	GNUNET_assert(message_total_size <= GNUNET_SERVER_MAX_MESSAGE_SIZE);

	void *buffer = malloc(message_total_size);

	struct GNUNET_MessageHeader *message = (struct GNUNET_MessageHeader *)buffer;
	message->size = htons((uint16_t)message_total_size);
	message->type = htons(GNUNET_MESSAGE_TYPE_SEARCH_FLOODING);

	struct gnunet_search_flooding_message *flooding_message = (struct gnunet_search_flooding_message*)(message + 1);
	flooding_message->flow_id = flow_id;
	flooding_message->ttl = 16;
	flooding_message->type = type;

	memcpy(flooding_message + 1, data, data_size);

	gnunet_search_flooding_peer_request_message_flood(message);

	free(buffer);
}

void gnunet_search_flooding_peer_request_message_flood(struct GNUNET_MessageHeader const *message) {
	gnunet_search_flooding_peer_message_process(NULL, message);
}

void gnunet_search_flooding_peer_request_flood(void const *data, size_t data_size) {
	gnunet_search_flooding_peer_data_flood(data, data_size, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_REQUEST, ((uint64_t)rand() << 32) | rand());
}

void gnunet_search_flooding_peer_response_flood(void const *data, size_t data_size, uint64_t flow_id) {
	gnunet_search_flooding_peer_data_flood(data, data_size, GNUNET_SEARCH_FLOODING_MESSAGE_TYPE_RESPONSE, flow_id);
}

void gnunet_search_handlers_set(void (*message_notification_handler)(struct GNUNET_PeerIdentity const *, struct gnunet_search_flooding_message *, size_t)) {
	_message_notification_handler = message_notification_handler;
}
