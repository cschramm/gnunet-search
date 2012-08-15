/**
 * @file search/service/url-processor/url-processor.c
 * @author Julian Kranz
 * @date 3.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's url processor component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This file contains all functions pertaining to the GNUnet Search service's url processor component. This component processes URLs received via the DHT. It also
 * helps deserializing the URL list received from the client.
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
#include <inttypes.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <crawl.h>

#include "../util/service-util.h"
#include "../storage/storage.h"
#include "../dht/dht.h"
#include "../normalization/normalization.h"

/**
 * @brief This function extracts a parameter and an URL from a value (structured by the GNUnet search) received while monitoring the DHT.
 *
 * @param url a reference to a memory location to store a reference to the extracted URL in
 * @param parameter a reference to a memory location to store the extracted parameter in
 * @param prefix_length an offset into the data from which to search start the parsing
 * @param data the data containing the DHT value
 * @param the size of the data containing the DHT value
 *
 * @return the length of the url
 */
static size_t gnunet_search_url_processor_url_extract(char **url, unsigned int *parameter, size_t prefix_length, void const *data, size_t size) {
	size_t position = prefix_length;
	*parameter = 0;
	for (size_t i = prefix_length; i < size; ++i)
		if (((char*) data)[i] == ':') {
			char parameter_str[i - prefix_length + 1];
			memcpy(parameter_str, data + prefix_length, (i - prefix_length));
			parameter_str[i - prefix_length] = 0;
			sscanf(parameter_str, "%u", parameter);
			position = i + 1;
			break;
		}

	size_t url_length = size - position;
	*url = (char*) GNUNET_malloc(url_length + 1);
	memcpy(*url, data + position, url_length);
	(*url)[url_length] = 0;

	return url_length;
}

//It therefor extracts the URL and its parameter (used for the crawling depth) from the raw DHT value and comm

/**
 * @brief This function processes an incoming URL value received while monitoring the DHT.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function processes an incoming URL value received while monitoring the DHT. It therefor extracts the URL and its parameter (used for the crawling depth)
 * from the raw DHT value; then it passes the URL to the crawling library. The resulting keywords found by the crawler are stored using the storage component.
 * In case the parameter value is greater than zero the URLs found by the crawler are again inserted into the DHT (with a lowered parameter).
 *
 * @param url a reference to a memory location to store a reference to the extracted URL in
 * @param parameter a reference to a memory location to store the extracted parameter in
 * @param prefix_length an offset into the data from which to search start the parsing
 * @param data the data containing the DHT value
 * @param the size of the data containing the DHT value
 */
void gnunet_search_url_processor_incoming_url_process(size_t prefix_length, void const *data, size_t size) {
//	printf("data: %s\n", (char*)data);

	char *url;
	unsigned int parameter;
	/*size_t url_length = */gnunet_search_url_processor_url_extract(&url, &parameter, prefix_length, data, size);

//	printf("Parameter: %u; url: %s\n", parameter, url);

	char **urls;
	size_t urls_size;

	char **keywords;
	size_t keywords_size;

	crawl_url_crawl(&keywords_size, &keywords, &urls_size, &urls, url);

	if (parameter > 0)
		gnunet_search_dht_url_list_put(urls, urls_size, parameter - 1);

	for (size_t i = 0; i < urls_size; ++i) {
//		printf("URL: %s\n", urls[i]);
		GNUNET_free(urls[i]);
	}

	for (size_t i = 0; i < keywords_size; ++i) {
//		printf("Keyword: %s\n", keywords[i]);
		gnunet_search_normalization_keyword_normalize(keywords[i]);
		gnunet_search_storage_key_value_add(keywords[i], url);
		GNUNET_free(keywords[i]);
	}

	if(urls)
		GNUNET_free(urls);
	if(keywords)
		GNUNET_free(keywords);

	GNUNET_free(url);
}

/**
 * @brief This function deserializes an URL array using a search command structure.
 *
 * @param urls a reference to a memory location to store the URL array in
 * @param cmd the search command structure
 *
 * @return the number of URLs in the array
 */
size_t gnunet_search_url_processor_cmd_urls_get(char ***urls, struct search_command const *cmd) {
	char const *urls_source = (char*) (cmd + 1);

	size_t urls_length;
	FILE *url_stream = open_memstream((char**) urls, &urls_length);

	size_t read_length = sizeof(struct search_command);
	size_t urls_number = 0;
	while(read_length < cmd->size) {
		size_t url_length = strnlen(urls_source, cmd->size - read_length);
		char *url = (char*) GNUNET_malloc(url_length + 1);
		memcpy(url, urls_source, url_length);
		url[url_length] = 0;

		fwrite(&url, sizeof(url), 1, url_stream);

		read_length += url_length + 1;
		urls_source += url_length + 1;
		urls_number++;
	}

	fclose(url_stream);
	return urls_number;
}
