/*
 * server-communication.c
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#ifndef SERVER_COMMUNICATION_C_
#define SERVER_COMMUNICATION_C_

#include <stdint.h>

struct gnunet_search_server_communication_header {
	uint8_t type;
	size_t size;
};

void add_listener(void (*handler)(struct gnunet_search_server_communication_header *, void*));
extern void gnunet_search_server_communication_receive();
extern void gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg);
extern void gnunet_search_server_communication_free();
extern void transmit_urls(const char *file);
extern void transmit_keyword(const char *keyword);

#endif /* SERVER_COMMUNICATION_C_ */
