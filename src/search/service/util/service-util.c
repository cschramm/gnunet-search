/*
 * gnunet-search-util.c
 *
 *  Created on: Aug 3, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "service-util.h"
#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>

void gnunet_search_util_key_value_generate_simple(char **key_value, const char *action, const char *data) {
	size_t key_value_length;
	FILE *key_value_stream = open_memstream(key_value, &key_value_length);

	fprintf(key_value_stream, "search:%s:%s", action, data);
	fputc(0, key_value_stream);

	fclose(key_value_stream);
}

/*
 * Security: precond: size > sizeof(struct search_command)
 */
void gnunet_search_util_cmd_keyword_get(char **keyword, struct search_command const *cmd, size_t size) {
	size_t max_length = size - sizeof(struct search_command);
	size_t keyword_length = strnlen((char*) (cmd + 1), max_length);

	*keyword = (char*) GNUNET_malloc(keyword_length + 1);
	memcpy(*keyword, cmd + 1, keyword_length);
	(*keyword)[keyword_length] = 0;
}
