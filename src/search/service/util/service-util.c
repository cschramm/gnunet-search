/**
 * @file search/service/util/service-util.c
 * @author Julian Kranz
 * @date 3.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's utility component.
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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "service-util.h"
#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>

/**
 * @brief This function generates a structured DHT key or value.
 *
 * @param key_value a reference to a memory location used to store the resulting DHT key or value
 * @param action an action associated with key or the value (not relevant anymore)
 * @param data the data to be included in the key or value
 */
void gnunet_search_util_key_value_generate_simple(char **key_value, const char *action, const char *data) {
	size_t key_value_length;
	FILE *key_value_stream = open_memstream(key_value, &key_value_length);

	fprintf(key_value_stream, "search:%s:%s", action, data);
	fputc(0, key_value_stream);

	fclose(key_value_stream);
}

/**
 * @brief This function extracts a keyword using a search command structure.
 *
 * @param keyword a reference to a memory location used to store the keyword
 * @param cmd a reference to the search command
 * @param size the amount of memory available including the search command; the caller has to make that this size is at least sizeof(struct search_command).
 */
void gnunet_search_util_cmd_keyword_get(char **keyword, struct search_command const *cmd, size_t size) {
	size_t max_length = size - sizeof(struct search_command);
	size_t keyword_length = strnlen((char*) (cmd + 1), max_length);

	*keyword = (char*) GNUNET_malloc(keyword_length + 1);
	memcpy(*keyword, cmd + 1, keyword_length);
	(*keyword)[keyword_length] = 0;
}
