/*
 * communication.h
 *
 *  Created on: Aug 6, 2012
 *      Author: jucs
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

extern void gnunet_search_communication_init();
extern char gnunet_search_communication_receive(const struct GNUNET_MessageHeader *gnunet_message);
extern void gnunet_search_communication_transmit(void *data, size_t size);
extern void gnunet_search_communication_listener_add(void (*listener)(size_t, void*));
extern void gnunet_search_communication_free();

#endif /* COMMUNICATION_H_ */
