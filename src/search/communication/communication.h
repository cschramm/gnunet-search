/**
 * @file search/communication/communication.c
 * @author Julian Kranz
 * @date 6.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search communication component.
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
