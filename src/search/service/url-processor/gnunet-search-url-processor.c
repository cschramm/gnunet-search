/*
 * gnunet-search-url-processor.c
 *
 *  Created on: Aug 3, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <crawl.h>

#include "../util/gnunet-search-util.h"
#include "../storage/storage.h"

void gnunet_search_url_processor_incoming_url_process(size_t prefix_length, void const *data, size_t size) {
	size_t position = prefix_length;
	unsigned int parameter = 0;
	for (size_t i = prefix_length; i < size; ++i)
		if (((char*) data)[i] == ':') {
			char parameter_str[i - prefix_length + 1];
			memcpy(parameter_str, data + prefix_length, (i - prefix_length));
			parameter_str[i - prefix_length] = 0;
			sscanf(parameter_str, "%u", &parameter);
			position = i + 1;
			break;
		}

	size_t url_length = size - position;
	char *url = (char*) malloc(url_length + 1);
	memcpy(url, data + position, url_length);
	url[url_length] = 0;

	printf("Parameter: %u; url: %s\n", parameter, url);

	char **urls;
	size_t urls_size;

	char **keywords;
	size_t keywords_size;

	crawl_url_crawl(&keywords_size, &keywords, &urls_size, &urls, url);

	if (parameter > 0)
		gnunet_search_util_dht_url_list_put(urls, urls_size, parameter - 1);

	for (size_t i = 0; i < urls_size; ++i) {
		printf("URL: %s\n", urls[i]);
		free(urls[i]);
	}

	for (size_t i = 0; i < keywords_size; ++i) {
		printf("Keyword: %s\n", keywords[i]);
		gnunet_search_storage_key_value_add(url, keywords[i]);
		free(keywords[i]);
	}

	free(urls);
	free(keywords);

	free(url);
}
