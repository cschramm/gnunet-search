/*
 * communication.c
 *
 *  Created on: Aug 6, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
//#include <gnunet/gnunet_dht_service.h>
#include "gnunet_protocols_search.h"
#include "communication.h"

#include <collections/queue/queue.h>
#include <collections/arraylist/arraylist.h>

static array_list_t *gnunet_search_server_communication_listeners;

static void gnunet_search_server_communication_listeners_notify(size_t size, void *buffer) {
	for(long int i = 0; i < array_list_get_length(gnunet_search_server_communication_listeners); ++i) {
		void (*listener)(size_t, void*);
		array_list_get(gnunet_search_server_communication_listeners, (const void**) &listener, i);
		listener(size, buffer);
	}
}

void gnunet_search_communication_init() {
	gnunet_search_server_communication_listeners = array_list_construct();
}

char gnunet_search_communication_receive(const struct GNUNET_MessageHeader *gnunet_message) {
	struct message_header *msg_header = (struct message_header*) (gnunet_message + 1);

	size_t gnunet_message_size = ntohs(gnunet_message->size);

	GNUNET_assert(gnunet_message_size >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct message_header));

	size_t payload_size = gnunet_message_size - sizeof(struct GNUNET_MessageHeader) - sizeof(struct message_header);

	static queue_t *fragments = NULL;
	/*
	 * Todo Security - how many fragments?
	 */

	if(msg_header->flags & GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED) {
		if(!fragments)
			fragments = queue_construct();
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
			return 0;
		} else {
			/*
			 * Todo: Security
			 */
			void *buffer = malloc(gnunet_message_size);
			memcpy(buffer, gnunet_message, gnunet_message_size);
			queue_enqueue(fragments, buffer);
			return 1;
		}
	} else {
//		struct gnunet_search_server_communication_header *header =
//				(struct gnunet_search_server_communication_header*) malloc(
//						sizeof(struct gnunet_search_server_communication_header));
//		header->size = response->size;
		gnunet_search_server_communication_listeners_notify(payload_size, msg_header + 1);
		return 0;
//		free(header);
	}
}

void gnunet_search_communication_listener_add(void (*listener)(size_t, void*)) {
	array_list_insert(gnunet_search_server_communication_listeners, listener);
}
