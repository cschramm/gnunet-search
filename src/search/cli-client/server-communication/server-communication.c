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
#include "../util/util.h"

#include <collections/queue/queue.h>
#include <collections/arraylist/arraylist.h>

static struct GNUNET_CLIENT_Connection *client_connection;

static queue_t *message_queue;
static array_list_t *listeners;

static void notify_listeners(struct gnunet_search_server_communication_header *header, void *buffer) {
	for(long int i = 0; i < array_list_get_length(listeners); ++i) {
		void (*handler)(struct gnunet_search_server_communication_header *, void*);
		array_list_get(listeners, (const void**) &handler, i);
		handler(header, buffer);
	}
}

void add_listener(void (*handler)(struct gnunet_search_server_communication_header *, void*)) {
	array_list_insert(listeners, handler);
}

static void receive_response(void *cls, const struct GNUNET_MessageHeader * msg) {
	struct search_response *response = (struct search_response*) (msg + 1);

	GNUNET_assert(ntohs(msg->size) >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct search_response));

	static queue_t *fragments = NULL;
	/*
	 * Todo Security - how many fragments?
	 */

	if(response->flags & GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED) {
		if(!fragments)
			fragments = array_list_construct();
		if(response->flags & GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT) {
			struct gnunet_search_server_communication_header *header =
					(struct gnunet_search_server_communication_header*) malloc(
							sizeof(struct gnunet_search_server_communication_header));
			header->type = response->type;
			char *buffer;
			FILE *memstream = open_memstream(&buffer, &header->size);
			while(queue_get_length(fragments)) {
				struct search_response *fragment = (struct search_response*) queue_dequeue(fragments);
				/*
				 * Todo: Security
				 */
				fwrite(fragment + 1, 1, fragments->size, memstream);
				free(fragment);
			}
			fwrite(response + 1, 1, response->size, memstream);
			fclose(memstream);
			notify_listeners(header, buffer);
			free(header);
			free(buffer);
		} else {
			size_t response_size = sizeof(struct search_response) + response->size;
			/*
			 * Todo: Security
			 */
			void *buffer = malloc(response_size);
			memcpy(buffer, response, response_size);
			queue_enqueue(fragments, response);
			GNUNET_CLIENT_receive(client_connection, &receive_response, NULL, GNUNET_TIME_relative_get_forever_());
		}
	} else {
		struct gnunet_search_server_communication_header *header =
				(struct gnunet_search_server_communication_header*) malloc(
						sizeof(struct gnunet_search_server_communication_header));
		header->type = response->type;
		header->size = response->size;
		notify_listeners(header, response + 1);
		free(header);
	}
}

void gnunet_search_server_communication_receive() {
	GNUNET_CLIENT_receive(client_connection, &receive_response, NULL, GNUNET_TIME_relative_get_forever_());
}

void gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg) {
	message_queue = queue_construct();
	listeners = array_list_construct();
	client_connection = GNUNET_CLIENT_connect("search", cfg);
}

void gnunet_search_server_communication_free() {
	queue_free(message_queue);
}

static void transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc);

static size_t transmit_ready(void *cls, size_t size, void *buffer) {

	size_t msg_size = sizeof(struct GNUNET_MessageHeader);

	struct search_command *cmd = (struct search_command*) cls;
	msg_size += cmd->size;

	GNUNET_assert(size >= msg_size);

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) buffer;
	header->type = GNUNET_MESSAGE_TYPE_SEARCH;
	header->size = htons(msg_size);

	memcpy(buffer + sizeof(struct GNUNET_MessageHeader), cls, cmd->size);

	free(cls);

	//printf("End of transmit_ready()...\n");

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &transmit_next, NULL);

	return msg_size;
}

static void transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	if(!queue_get_length(message_queue))
		return;

	struct search_command *cmd = (struct search_command*) queue_dequeue(message_queue);

	GNUNET_CLIENT_notify_transmit_ready(client_connection, sizeof(struct GNUNET_MessageHeader) + cmd->size,
			GNUNET_TIME_relative_get_forever_(), 1, &transmit_ready, cmd);
}

void transmit_urls(const char *file) {
//	printf("transmit_urls()\n");

	char **urls = NULL;
	size_t urls_length = urls_read(&urls, file);

	size_t maximal_payload_size = GNUNET_SERVER_MAX_MESSAGE_SIZE - sizeof(struct search_command)
			- sizeof(struct GNUNET_MessageHeader) /*- 65522 +l 60*/;
//	printf("mps: %lu\n", maximal_payload_size);

	char more_urls = 1;
	char fragmented = 0;
	size_t url_index = 0;
	while(more_urls) {
		void *serialized;
		size_t serialized_size;
		FILE *memstream = open_memstream((char**) &serialized, &serialized_size);

		fseek(memstream, sizeof(struct search_command), SEEK_CUR);

//	int64_t urls_length64 = urls_length;
//	fwrite(&urls_length64, sizeof(urls_length64), 1, memstream);

		uint8_t flags = fragmented ? GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT : 0;

		more_urls = 0;
		size_t written_size = 0;
		for(; url_index < urls_length; ++url_index) {
			size_t url_size = strlen(urls[url_index]) + 1;
//			printf("++ url_index: %lu, %lu, %lu\n", url_index, written_size, url_size);
			if(written_size + url_size > maximal_payload_size) {
				flags &= ~GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT;
				fragmented = 1;
				more_urls = 1;
//				printf("url_index: %lu, %lu, %lu\n", url_index, written_size, url_size);
				break;
			}
			fwrite(urls[url_index], 1, url_size, memstream);
			written_size += url_size;
		}

		fclose(memstream);

		flags |= fragmented ? GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED : 0;

//		printf("flags: 0x%x\n", flags);

		struct search_command *cmd = (struct search_command*) serialized;

		cmd->size = serialized_size;
		cmd->action = GNUNET_SEARCH_ACTION_ADD;
		cmd->flags = flags;

//	printf("%s - %lu\n", (char*)(serialized + sizeof(struct search_command)), serialized_size);

//	for (int i = 0; i < urls_length; ++i) {
//		printf("%s\n", urls[i]);
//	}

		queue_enqueue(message_queue, serialized);
//		GNUNET_CLIENT_notify_transmit_ready(client_connection, sizeof(struct GNUNET_MessageHeader) + cmd->size,
//				GNUNET_TIME_relative_get_forever_(), 1, &transmit_ready, serialized);

//		if (more_urls)
//			printf("WARNING --- SKIPPING OTHER FRAGMENTS ---");
//		break;
	}

	for(int i = 0; i < urls_length; ++i)
		free(urls[i]);

	free(urls);

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &transmit_next, NULL);
}

void transmit_keyword(const char *keyword) {
	void *serialized;
	size_t serialized_size;
	FILE *memstream = open_memstream((char**) &serialized, &serialized_size);

	fseek(memstream, sizeof(struct search_command), SEEK_CUR);
	fwrite(keyword, 1, strlen(keyword) + 1, memstream);

	fclose(memstream);

	struct search_command *cmd = (struct search_command*) serialized;
	cmd->action = GNUNET_SEARCH_ACTION_SEARCH;
	cmd->size = serialized_size;

	queue_enqueue(message_queue, serialized);
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &transmit_next, NULL);
//	GNUNET_CLIENT_notify_transmit_ready(client_connection, sizeof(struct GNUNET_MessageHeader) + serialized_size,
//			GNUNET_TIME_relative_get_forever_(), 1, &transmit_ready, serialized);
}
