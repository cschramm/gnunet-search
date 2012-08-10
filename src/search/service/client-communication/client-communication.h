/*
 * client-communication.h
 *
 *  Created on: Aug 4, 2012
 *      Author: jucs
 */

#ifndef CLIENT_COMMUNICATION_H_
#define CLIENT_COMMUNICATION_H_

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

extern void gnunet_search_client_communication_message_handle(void *cls, struct GNUNET_SERVER_Client *client,
		const struct GNUNET_MessageHeader *message);
extern void gnunet_search_client_communication_init();
extern void gnunet_search_client_communication_free();
extern void gnunet_search_client_communication_send_result(void const *data, size_t size, char type, uint16_t id);
extern uint16_t gnunet_search_client_communication_by_flow_id_request_id_get(uint64_t flow_id);

#endif /* CLIENT_COMMUNICATION_H_ */
