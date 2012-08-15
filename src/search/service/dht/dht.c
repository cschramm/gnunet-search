/**
 * @file search/service/dht/dht.c
 * @author Julian Kranz
 * @date 8.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's dht component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This file contains all functions pertaining to the GNUnet Search service's dht component. This component is responsible for all interaction with the
 * DHT service. It does not only offer functions to insert data but also to monitors the DHT's activity.
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

#include "dht.h"

#include "../globals/globals.h"
#include "../url-processor/url-processor.h"

/**
 * @brief This variable stores a reference to the GNUnet dht monitor handle.
 */
static struct GNUNET_DHT_MonitorHandle *gnunet_search_dht_monitor_handle;
/**
 * @brief This variable stores a reference to the GNUnet dht handle.
 */
static struct GNUNET_DHT_Handle *gnunet_search_dht_handle;

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

/**
 * @brief This function puts a key-value-tuple into the DHT; both the key and the value are strings.
 *
 * @param key the key of the tuple
 * @param value the value of the tuple
 */
static void gnunet_search_dht_string_string_put(const char *key, const char *value) {
	size_t key_length = strlen(key);

	GNUNET_HashCode hash;
	GNUNET_CRYPTO_hash(key, key_length, &hash);

	size_t value_length = strlen(value);

	GNUNET_DHT_put(gnunet_search_dht_handle, &hash, 2, GNUNET_DHT_RO_NONE, GNUNET_BLOCK_TYPE_TEST, value_length + 1,
			value, GNUNET_TIME_absolute_get_forever_(), GNUNET_TIME_relative_get_forever_(), /*&message_sent_cont*/NULL,
			NULL);
}

/**
 * @brief This function combines an action string, an integer parameter and a data string to a string used as a key or value in the DHT.
 *
 * @param key_value a reference to a string used to store the generated string in
 * @param action the action string to use
 * @param the integer parameter
 * @param data the data string
 */
static void gnunet_search_dht_key_value_generate(char **key_value, const char *action, unsigned int parameter,
		const char *data) {
	size_t key_value_length;
	FILE *key_value_stream = open_memstream(key_value, &key_value_length);

	fprintf(key_value_stream, "search:%s:%u:%s", action, parameter, data);
	fputc(0, key_value_stream);

	fclose(key_value_stream);
}

/**
 * @brief This function is the callback function handed to GNUnet to be called on a captured DHT put event.
 *
 * @param cls the GNUnet closure (not used)
 * @param options the GNUnet DHT route options (not used)
 * @param type the GNUnet block type (not used)
 * @param hop_count the hop count (not used)
 * @param desired_replication_level the desired replication level (not used)
 * @param path_length the path length (not used)
 * @param path the path (not used)
 * @param exp the expiration time of the data (not used)
 * @param key the hashed key (not used)
 * @param data the data inserted into the DHT
 * @param size the size of the data
 */
static void gnunet_search_dht_monitor_put(void *cls, enum GNUNET_DHT_RouteOption options, enum GNUNET_BLOCK_Type type,
		uint32_t hop_count, uint32_t desired_replication_level, unsigned int path_length,
		const struct GNUNET_PeerIdentity *path, struct GNUNET_TIME_Absolute exp, const GNUNET_HashCode * key,
		const void *data, size_t size) {
	char const *prefix = "search:url:";
	size_t prefix_length = strlen(prefix);
	if(size < prefix_length + 1)
		return;
	if(!strncmp(prefix, data, prefix_length)) {
		gnunet_search_url_processor_incoming_url_process(prefix_length, data, size);
	}
}

/**
 * @brief This function inserts an array of URLs into the DHT.
 *
 * @param urls the array of URLs
 * @param size the number of URLs contained in the array
 * @parameter the parameter value to include (see URL processor component)
 */
void gnunet_search_dht_url_list_put(char **urls, size_t size, unsigned int parameter) {
	for(int i = 0; i < size; ++i) {
		char *key_value;
		gnunet_search_dht_key_value_generate(&key_value, "url", parameter, urls[i]);

		printf("Putting value %s...\n", key_value);

		gnunet_search_dht_string_string_put(key_value, key_value);

		GNUNET_free(key_value);
	}
}

/**
 * @brief This function initialises the dht component.
 */
void gnunet_search_dht_init() {
	gnunet_search_dht_handle = GNUNET_DHT_connect(gnunet_search_globals_cfg, 3);

	/*
	 * Todo: Own block type
	 */
	gnunet_search_dht_monitor_handle = GNUNET_DHT_monitor_start(gnunet_search_dht_handle, GNUNET_BLOCK_TYPE_TEST, NULL, NULL, NULL,
			&gnunet_search_dht_monitor_put, NULL);
}

/**
 * @brief This function releases all resources held by the dht component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function releases all resources held by the dht component. It stops to monitor the DHT and disconnects from the DHT service.
 */
void gnunet_search_dht_free() {
	GNUNET_DHT_monitor_stop(gnunet_search_dht_monitor_handle);

	GNUNET_DHT_disconnect(gnunet_search_dht_handle);
}

