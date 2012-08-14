/**
 * @file search/client/server-communication/server-communication.h
 * @author Julian Kranz
 * @date 5.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search client's server communication component.
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

#ifndef SERVER_COMMUNICATION_C_
#define SERVER_COMMUNICATION_C_

#include <stdint.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

extern void gnunet_search_server_communication_receive();
extern char gnunet_search_server_communication_init(const struct GNUNET_CONFIGURATION_Handle *cfg);
extern void gnunet_search_server_communication_free();

#endif /* SERVER_COMMUNICATION_C_ */
