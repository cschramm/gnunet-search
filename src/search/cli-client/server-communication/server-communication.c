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
#include "../util/util.h"

#include <collections/queue/queue.h>

struct GNUNET_CLIENT_Connection *client_connection;

static queue_t *message_queue;

void gnunet_search_server_communication_message_queue_init() {
	message_queue = queue_construct();
}

void gnunet_search_server_communication_message_queue_free() {
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

	struct search_command *cmd = (struct search_command*)queue_dequeue(message_queue);

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
	while (more_urls) {
		void *serialized;
		size_t serialized_size;
		FILE *memstream = open_memstream((char**) &serialized, &serialized_size);

		fseek(memstream, sizeof(struct search_command), SEEK_CUR);

//	int64_t urls_length64 = urls_length;
//	fwrite(&urls_length64, sizeof(urls_length64), 1, memstream);

		uint8_t flags = fragmented ? GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT : 0;

		more_urls = 0;
		size_t written_size = 0;
		for (; url_index < urls_length; ++url_index) {
			size_t url_size = strlen(urls[url_index]) + 1;
//			printf("++ url_index: %lu, %lu, %lu\n", url_index, written_size, url_size);
			if (written_size + url_size > maximal_payload_size) {
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

	for (int i = 0; i < urls_length; ++i)
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
