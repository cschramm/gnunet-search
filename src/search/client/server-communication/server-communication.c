/*
 * server-communication.c
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>
#include "gnunet_protocols_search.h"
#include "server-communication.h"

#include <collections/queue/queue.h>
#include <collections/arraylist/arraylist.h>

static struct GNUNET_CLIENT_Connection *client_connection;

static queue_t *gnunet_search_server_communication_message_queue;
static array_list_t *gnunet_search_server_communication_listeners;

static struct gnunet_search_server_communication_queued_message {
	void *buffer;
	size_t size;
};

static void gnunet_search_server_communication_listeners_notify(size_t size, void *buffer) {
	for(long int i = 0; i < array_list_get_length(gnunet_search_server_communication_listeners); ++i) {
		void (*listener)(size_t, void*);
		array_list_get(gnunet_search_server_communication_listeners, (const void**) &listener, i);
		listener(size, buffer);
	}
}

static void gnunet_search_server_communication_receive_response(void *cls, const struct GNUNET_MessageHeader * gnunet_message) {
	struct message_header *msg_header = (struct message_header*) (gnunet_message + 1);

	GNUNET_assert(ntohs(gnunet_message->size) >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct message_header));

	size_t gnunet_message_size = ntohs(gnunet_message->size);
	size_t payload_size = gnunet_message_size - sizeof(struct GNUNET_MessageHeader) - sizeof(struct message_header);

	static queue_t *fragments = NULL;
	/*
	 * Todo Security - how many fragments?
	 */

	if(msg_header->flags & GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED) {
		if(!fragments)
			fragments = array_list_construct();
		if(msg_header->flags & GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT) {
//			struct gnunet_search_server_communication_header *header =
//					(struct gnunet_search_server_communication_header*) malloc(
//							sizeof(struct gnunet_search_server_communication_header));
//			header->type = response->type;
			size_t total_size;
			char *buffer;
			FILE *memstream = open_memstream(&buffer, &total_size);
			while(queue_get_length(fragments)) {
				struct GNUNET_MessageHeader *fragment = (struct GNUNET_MessageHeader*) queue_dequeue(fragments);
				struct message_header *fragment_msg_header = (struct message_header *) (fragment + 1);

				size_t fragment_message_size = ntohs(fragment->size);
				size_t fragment_payload_size = fragment_message_size - sizeof(struct GNUNET_MessageHeader)
						- sizeof(struct message_header);
				/*
				 * Todo: Security
				 */
				fwrite(fragment_msg_header + 1, 1, fragment_payload_size, memstream);
				free(fragment);
			}
			fwrite(msg_header + 1, 1, payload_size, memstream);
			fclose(memstream);
			gnunet_search_server_communication_listeners_notify(total_size, buffer);
//			free(header);
			free(buffer);
		} else {
			/*
			 * Todo: Security
			 */
			void *buffer = malloc(gnunet_message_size);
			memcpy(buffer, gnunet_message, gnunet_message_size);
			queue_enqueue(fragments, buffer);
			GNUNET_CLIENT_receive(client_connection, &gnunet_search_server_communication_receive_response, NULL, GNUNET_TIME_relative_get_forever_());
		}
	} else {
//		struct gnunet_search_server_communication_header *header =
//				(struct gnunet_search_server_communication_header*) malloc(
//						sizeof(struct gnunet_search_server_communication_header));
//		header->size = response->size;
		gnunet_search_server_communication_listeners_notify(payload_size, msg_header + 1);
//		free(header);
	}
}

static void gnunet_search_server_communication_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc);

static size_t gnunet_search_server_communication_transmit_ready(void *cls, size_t size, void *buffer) {

	size_t msg_size = sizeof(struct GNUNET_MessageHeader);

	struct gnunet_search_server_communication_queued_message *msg =
			(struct gnunet_search_server_communication_queued_message*) cls;
	msg_size += msg->size;

	GNUNET_assert(size >= msg_size);

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) buffer;
	header->type = GNUNET_MESSAGE_TYPE_SEARCH;
	header->size = htons(msg_size);

	memcpy(buffer + sizeof(struct GNUNET_MessageHeader), msg->buffer, msg->size);

	free(msg->buffer);
	free(msg);

	//printf("End of transmit_ready()...\n");

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_server_communication_transmit_next, NULL);

	return msg_size;
}

static void gnunet_search_server_communication_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	if(!queue_get_length(gnunet_search_server_communication_message_queue))
		return;

	struct gnunet_search_server_communication_queued_message *msg =
			(struct gnunet_search_server_communication_queued_message*) queue_dequeue(
					gnunet_search_server_communication_message_queue);

	GNUNET_CLIENT_notify_transmit_ready(client_connection, sizeof(struct GNUNET_MessageHeader) + msg->size,
			GNUNET_TIME_relative_get_forever_(), 1, &gnunet_search_server_communication_transmit_ready, msg);
}

void gnunet_search_server_communication_listener_add(void (*listener)(size_t, void*)) {
	array_list_insert(gnunet_search_server_communication_listeners, listener);
}

void gnunet_search_server_communication_receive() {
	GNUNET_CLIENT_receive(client_connection, &gnunet_search_server_communication_receive_response, NULL, GNUNET_TIME_relative_get_forever_());
}

void gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg) {
	gnunet_search_server_communication_message_queue = queue_construct();
	gnunet_search_server_communication_listeners = array_list_construct();
	client_connection = GNUNET_CLIENT_connect("search", cfg);
}

void gnunet_search_server_communication_free() {
	queue_free(gnunet_search_server_communication_message_queue);
}

void gnunet_search_server_communication_transmit(void *data, size_t size) {
	size_t maximal_payload_size = GNUNET_SERVER_MAX_MESSAGE_SIZE - sizeof(struct message_header)
			- sizeof(struct GNUNET_MessageHeader) /*- 65522 +60*/;

	char fragmented = 0;
	size_t data_left = size;
	while(data_left) {
		size_t offset = size - data_left;
		size_t size;
		uint8_t flags = 0;
		if(data_left > maximal_payload_size) {
			fragmented = 1;
			size = maximal_payload_size;
		} else {
			flags |= fragmented ? GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT : 0;
			size = data_left;
		}

		void *buffer = malloc(sizeof(struct message_header) + size);
		memcpy(buffer + sizeof(struct message_header), data + offset, size);

		struct message_header *msg_header = (struct message_header*) buffer;

		flags |= fragmented ? GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED : 0;

		msg_header->flags = flags;

		struct gnunet_search_server_communication_queued_message *msg =
				(struct gnunet_search_server_communication_queued_message*) malloc(
						sizeof(struct gnunet_search_server_communication_queued_message));
		msg->buffer = buffer;
		msg->size = size;

		queue_enqueue(gnunet_search_server_communication_message_queue, msg);

		data_left -= size;
	}

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_server_communication_transmit_next, NULL);
}
