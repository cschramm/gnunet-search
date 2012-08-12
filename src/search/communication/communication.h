/*
 * communication.h
 *
 *  Created on: Aug 6, 2012
 *      Author: jucs
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

extern void gnunet_search_communication_init(
		void (*request_notify_transmit_ready_handler)(size_t size, void *cls, size_t (*)(void*, size_t, void*),
				struct GNUNET_TIME_Relative));
extern char gnunet_search_communication_receive(const struct GNUNET_MessageHeader *gnunet_message);
extern void gnunet_search_communication_transmit(void *data, size_t size);
extern void gnunet_search_communication_listener_add(void (*listener)(size_t, void*));
extern void gnunet_search_communication_free();
extern void gnunet_search_communication_flush();

#endif /* COMMUNICATION_H_ */
