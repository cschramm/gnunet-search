/**
 * @file search/service/dht/dht.c
 * @author Julian Kranz
 * @date 8.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search service's dht component.
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

#ifndef DHT_H_
#define DHT_H_

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>

extern void gnunet_search_dht_url_list_put(char **urls, size_t size, unsigned int parameter);
extern void gnunet_search_dht_init();
extern void gnunet_search_dht_free();

#endif /* DHT_H_ */
