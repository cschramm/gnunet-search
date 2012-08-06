/*
 * server-communication.c
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#ifndef SERVER_COMMUNICATION_C_
#define SERVER_COMMUNICATION_C_

#include <stdint.h>

void gnunet_search_server_communication_transmit(void *data, size_t size);
extern void gnunet_search_server_communication_receive();
extern void gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg);
extern void gnunet_search_server_communication_free();

#endif /* SERVER_COMMUNICATION_C_ */
