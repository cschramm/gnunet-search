/*
 * server-communication.c
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#ifndef SERVER_COMMUNICATION_C_
#define SERVER_COMMUNICATION_C_

#include <stdint.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

extern void gnunet_search_server_communication_receive();
extern char gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg);
extern void gnunet_search_server_communication_free();

#endif /* SERVER_COMMUNICATION_C_ */
