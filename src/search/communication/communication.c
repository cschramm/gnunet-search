/**
 * @file search/communication/communication.c
 * @author Julian Kranz
 * @date 6.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search communication component.
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

#include <gnunet/platform.h>
#include <gnunet/gnunet_common.h>
#include <gnunet/gnunet_util_lib.h>
#include "gnunet_protocols_search.h"
#include "communication.h"

#include <collections/queue/queue.h>
#include <collections/arraylist/arraylist.h>

/**
 * @brief This variable stores references to all subscribed listeners; see below for more details.
 */
static array_list_t *gnunet_search_communication_listeners;
/**
 * @brief This variable implements an output queue for messages.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable implements an output queue for messages. Since GNUnet does not allow the user to
 * queue more than one message at a time it is important to handle this situation correctly. New
 * mesage are enqueued in this queue and are subequently sent one after one.
 */
static queue_t *gnunet_search_communication_message_queue;
/**
 * @brief This variable stores a reference to a function which calls the respective notify transmit ready
 * function of GNUnet.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable stores a reference to a function which calls the respective notify transmit ready function
 * of GNUnet. GNUnet offers different functions
 * for message transmission on the service and client side; while the functionality does not differ and the process steps are the same, different
 * API functions and data structures are offered. In order to nevertheless implement a generic communication component both the client and the
 * service have to implement their own generic handlers. These generic handlers then call the specific GNUnet API functions for transmission.
 */
static void (*gnunet_search_communication_request_notify_transmit_ready)(size_t size, void *cls,
		size_t (*)(void*, size_t, void*), struct GNUNET_TIME_Relative);

/**
 * @brief This data structure is used to combine all parameters needed for a message waiting in the output queue.
 */
struct gnunet_search_communication_queued_message {
	/**
	 * @brief This member stores a reference to buffer to be sent.
	 */
	void *buffer;
	/**
	 * @brief This member stores the size of the buffer.
	 */
	size_t size;
};

/**
 * @brief This function notifies all listeners about a newly arrived message.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function notifies all listeners about a newly arrived message. It is important to note that a message may be fragmented and thus
 * consist of multiple GNUnet messages.
 *
 * @param size the size of the new message
 * @param buffer the buffer containing the new message
 */
static void gnunet_search_communication_listeners_notify(size_t size, void *buffer) {
	for(long int i = 0; i < array_list_get_length(gnunet_search_communication_listeners); ++i) {
		void (*listener)(size_t, void*);
		array_list_get(gnunet_search_communication_listeners, (const void**) &listener, i);
		listener(size, buffer);
	}
}

static void gnunet_search_communication_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc);

/**
 * @brief This function is called by GNUnet is case a new buffer is available for a message to be sent.
 *
 * @param cls the GNUnet closure
 * @param size the amout of buffer space available
 * @param buffer the output buffer the message shall be written to
 *
 * @return the amout of buffer space written
 */
static size_t gnunet_search_communication_transmit_ready(void *cls, size_t size, void *buffer) {
	size_t msg_size = sizeof(struct GNUNET_MessageHeader);

	struct gnunet_search_communication_queued_message *msg = (struct gnunet_search_communication_queued_message*) cls;
	msg_size += msg->size;

	GNUNET_assert(size >= msg_size);
	if(size < msg_size)
		return 0;

	struct GNUNET_MessageHeader *header = (struct GNUNET_MessageHeader*) buffer;
	header->type = GNUNET_MESSAGE_TYPE_SEARCH;
	header->size = htons(msg_size);

	memcpy(buffer + sizeof(struct GNUNET_MessageHeader), msg->buffer, msg->size);

//	free(msg->buffer);
//	free(msg);

//printf("End of transmit_ready()...\n");

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_communication_transmit_next, NULL);

	return msg_size;
}

/**
 * @brief This function initiates the transmission of the next message.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function initiates the transmission of the next message. In order to do that it dequeues the next message from the
 * output queue and requests the service or client communication component to call the appropriate GNUnet function for the
 * transmission of the message. The function is implemented as a GNUnet task; this is done in order to decouple it from the
 * transmit_ready() function call (see above).
 *
 * @param cls the GNUnet closure (not used)
 * @param tc the GNUnet task context (not used)
 */
static void gnunet_search_communication_transmit_next(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
	if(!queue_get_length(gnunet_search_communication_message_queue))
		return;

	struct gnunet_search_communication_queued_message *msg =
			(struct gnunet_search_communication_queued_message*) queue_dequeue(
					gnunet_search_communication_message_queue);

	struct GNUNET_TIME_Relative max_delay = GNUNET_TIME_relative_get_minute_();
	struct GNUNET_TIME_Relative gct = GNUNET_TIME_relative_add(max_delay, GNUNET_TIME_relative_get_second_());

	gnunet_search_communication_request_notify_transmit_ready(sizeof(struct GNUNET_MessageHeader) + msg->size, msg,
			&gnunet_search_communication_transmit_ready, max_delay);

	/*
	 * Todo: Save and free...
	 */
	GNUNET_SCHEDULER_add_delayed(gct, &gnunet_search_commuinication_queued_message_free_task, msg);
}

/**
 * @brief This function frees a previously queued message and its buffer.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function frees a previously queued message and its buffer. GNUnet does not call transmit_ready() in case an error occurs.
 * In that case this function cannot take care of freeing the message data structure and the buffer. In order to solve that problem
 * this handler is scheduled to be called after the transmit_ready() function call. As this is a ordinary scheduled task it is called
 * despite possible communication errors. For this reason the approch prevents memory leaks.
 *
 * @param cls the GNUnet closure containing a reference to message to be freed
 * @tc the GNUnet task context (not used)
 */
static void gnunet_search_commuinication_queued_message_free_task(void *cls,
		const struct GNUNET_SCHEDULER_TaskContext *tc) {
	struct gnunet_search_communication_queued_message *msg = (struct gnunet_search_communication_queued_message*) cls;
	GNUNET_free(msg->buffer);
	GNUNET_free(msg);
}

/**
 * @brief This function initialises the communication component.
 *
 * @param request_notify_transmit_ready_handler a handler given by the client's over service's communication component that calls the appropriate GNUnet
 * functions for message transmission
 */
void gnunet_search_communication_init(
		void (*request_notify_transmit_ready_handler)(size_t size, void *cls, size_t (*)(void*, size_t, void*),
				struct GNUNET_TIME_Relative)) {
	gnunet_search_communication_listeners = array_list_construct();
	gnunet_search_communication_message_queue = queue_construct();
	gnunet_search_communication_request_notify_transmit_ready = request_notify_transmit_ready_handler;
}

/**
 * @brief This function releases all resources held by the communication component.
 */
void gnunet_search_communication_free() {
	while(queue_get_length(gnunet_search_communication_message_queue)) {
		struct gnunet_search_communication_queued_message *msg =
				(struct gnunet_search_communication_queued_message *) queue_dequeue(
						gnunet_search_communication_message_queue);
		GNUNET_free(msg->buffer);
		GNUNET_free(msg);
	}
	queue_free(gnunet_search_communication_message_queue);
	array_list_free(gnunet_search_communication_listeners);
}

/**
 * @brief This function resets all active communication sessions.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function resets all active communication sessions. It is used to cope with a client or service disconnect; in that case
 * already received message fragments are no longer valid and all outgoing messages not yet sent have to be discarded.
 */
void gnunet_search_communication_flush() {
	while(queue_get_length(gnunet_search_communication_message_queue)) {
		struct gnunet_search_communication_queued_message *msg =
				(struct gnunet_search_communication_queued_message *) queue_dequeue(
						gnunet_search_communication_message_queue);
		GNUNET_free(msg->buffer);
		GNUNET_free(msg);
	}
	gnunet_search_communication_receive(NULL);
}

/**
 * @brief This function handles the reception of a new GNUnet message that may be a fragment of a communication message.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function handles the reception of a new GNUnet message that may be a fragment of a communication message. For this purpose
 * the function stores a fragment queue using a static variable. A new message contains a fragmentation header that indicates whether
 * the message is fragmented and whether the current fragment is the last fragment. In case the current fragment is the last fragment
 * or the message is not fragmented it is assembled and passed to registered message handlers.
 *
 * @param gnunet_message the GNUnet messag received; a NULL reference resets the fragment queue.
 *
 * @return a boolean value indicating whether more fragments are needed (1) for the communication message or not (0)
 */
char gnunet_search_communication_receive(const struct GNUNET_MessageHeader *gnunet_message) {
	static queue_t *fragments = NULL;
	/*
	 * Todo Security - how many fragments?
	 */
	if(!gnunet_message) {
		if(fragments)
			while(queue_get_length(fragments)) {
				void *fragment = (void*)queue_dequeue(fragments);
				free(fragment);
			}
		return 0;
	}

	struct message_header *msg_header = (struct message_header*) (gnunet_message + 1);

	size_t gnunet_message_size = ntohs(gnunet_message->size);

	GNUNET_assert(gnunet_message_size >= sizeof(struct GNUNET_MessageHeader) + sizeof(struct message_header));
	if(gnunet_message_size < sizeof(struct GNUNET_MessageHeader) + sizeof(struct message_header)) {
		return gnunet_search_communication_receive(NULL);
	}

	size_t payload_size = gnunet_message_size - sizeof(struct GNUNET_MessageHeader) - sizeof(struct message_header);

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
				fwrite(fragment_msg_header + 1, 1, fragment_payload_size, memstream);
				GNUNET_free(fragment);
			}
			fwrite(msg_header + 1, 1, payload_size, memstream);
			fclose(memstream);
			gnunet_search_communication_listeners_notify(total_size, buffer);
//			free(header);
			GNUNET_free(buffer);
			return 0;
		} else {
			void *buffer = GNUNET_malloc(gnunet_message_size);
			memcpy(buffer, gnunet_message, gnunet_message_size);
			queue_enqueue(fragments, buffer);
			return 1;
		}
	} else {
//		struct gnunet_search_server_communication_header *header =
//				(struct gnunet_search_server_communication_header*) malloc(
//						sizeof(struct gnunet_search_server_communication_header));
//		header->size = response->size;
		gnunet_search_communication_listeners_notify(payload_size, msg_header + 1);
		return 0;
//		free(header);
	}
}

void gnunet_search_communication_listener_add(void (*listener)(size_t, void*)) {
	array_list_insert(gnunet_search_communication_listeners, listener);
}

void gnunet_search_communication_transmit(void *data, size_t size) {
	size_t maximal_payload_size = GNUNET_SERVER_MAX_MESSAGE_SIZE - sizeof(struct message_header)
			- sizeof(struct GNUNET_MessageHeader);
//	maximal_payload_size = 5;

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

		size_t msg_with_header_size = sizeof(struct message_header) + size;

		void *buffer = GNUNET_malloc(msg_with_header_size);
		memcpy(buffer + sizeof(struct message_header), data + offset, size);

		struct message_header *msg_header = (struct message_header*) buffer;

		flags |= fragmented ? GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED : 0;

		msg_header->flags = flags;

		struct gnunet_search_communication_queued_message *msg =
				(struct gnunet_search_communication_queued_message*) GNUNET_malloc(
						sizeof(struct gnunet_search_communication_queued_message));
		msg->buffer = buffer;
		msg->size = msg_with_header_size;

		queue_enqueue(gnunet_search_communication_message_queue, msg);

		data_left -= size;
	}

	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_ZERO, &gnunet_search_communication_transmit_next, NULL);
}
