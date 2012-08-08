/*
 * dht.c
 *
 *  Created on: Aug 8, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>
#include "../globals/globals.h"

//static void search_dht_get_result_iterator_and_send_to_user(void *cls, struct GNUNET_TIME_Absolute exp,
//		const GNUNET_HashCode * key, const struct GNUNET_PeerIdentity * get_path, unsigned int get_path_length,
//		const struct GNUNET_PeerIdentity * put_path, unsigned int put_path_length, enum GNUNET_BLOCK_Type type,
//		size_t size, const void *data) {
//
//	GNUNET_DHT_get_stop(dht_get_handle);
//
//	gnunet_search_client_communication_send_result(data, size, GNUNET_SEARCH_RESPONSE_TYPE_RESULT);
//}
//
//static void search_dht_get_and_send_to_user(char const *keyword) {
//	char *key_value;
//	search_key_value_generate_simple(&key_value, "keyword", keyword);
//
//	printf("Searching for: %s\n", key_value);
//
//	GNUNET_HashCode hash;
//	GNUNET_CRYPTO_hash(key_value, strlen(key_value), &hash);
//
//	free(key_value);
//
//	dht_get_handle = GNUNET_DHT_get_start(gnunet_search_dht_handle, GNUNET_BLOCK_TYPE_TEST, &hash, 3,
//			GNUNET_DHT_RO_NONE, NULL, 0, &search_dht_get_result_iterator_and_send_to_user, NULL);
//}
//
//static void search_process(char const *keyword) {
//	search_dht_get_and_send_to_user(keyword);
//}

static void gnunet_search_util_dht_string_string_put(const char *key, const char *value) {
	size_t key_length = strlen(key);

	GNUNET_HashCode hash;
	GNUNET_CRYPTO_hash(key, key_length, &hash);

	size_t value_length = strlen(value);

	GNUNET_DHT_put(gnunet_search_dht_handle, &hash, 2, GNUNET_DHT_RO_NONE, GNUNET_BLOCK_TYPE_TEST, value_length + 1, value,
			GNUNET_TIME_absolute_get_forever_(), GNUNET_TIME_relative_get_forever_(), /*&message_sent_cont*/NULL, NULL);
}

static void gnunet_search_util_key_value_generate(char **key_value, const char *action, unsigned int parameter, const char *data) {
	size_t key_value_length;
	FILE *key_value_stream = open_memstream(key_value, &key_value_length);

	fprintf(key_value_stream, "search:%s:%u:%s", action, parameter, data);
	fputc(0, key_value_stream);

	fclose(key_value_stream);
}

void gnunet_search_util_dht_url_list_put(char **urls, size_t size, unsigned int parameter) {
	for (int i = 0; i < size; ++i) {
		char *key_value;
		gnunet_search_util_key_value_generate(&key_value, "url", parameter, urls[i]);

		printf("Putting value %s...\n", key_value);

		gnunet_search_util_dht_string_string_put(key_value, key_value);

		free(key_value);
	}
}
