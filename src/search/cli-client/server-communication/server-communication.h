/*
 * server-communication.c
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#ifndef SERVER_COMMUNICATION_C_
#define SERVER_COMMUNICATION_C_

extern struct GNUNET_CLIENT_Connection *client_connection;

extern void transmit_urls(const char *file);
extern void transmit_keyword(const char *keyword);

#endif /* SERVER_COMMUNICATION_C_ */
