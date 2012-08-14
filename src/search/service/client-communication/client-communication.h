/**
 * @file search/service/client-communication/client-communication.h
 * @author Julian Kranz
 * @date 4.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search service's client communication component.
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

#ifndef CLIENT_COMMUNICATION_H_
#define CLIENT_COMMUNICATION_H_

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

extern void gnunet_search_client_communication_message_handle(void *cls, struct GNUNET_SERVER_Client *client,
		const struct GNUNET_MessageHeader *message);
extern void gnunet_search_client_communication_init(struct GNUNET_SERVER_Handle *server);
extern void gnunet_search_client_communication_free();
extern void gnunet_search_client_communication_send_result(void const *data, size_t size, char type, uint16_t id);
extern uint16_t gnunet_search_client_communication_by_flow_id_request_id_get(uint64_t flow_id);
extern void gnunet_search_client_communication_flush();

#endif /* CLIENT_COMMUNICATION_H_ */
