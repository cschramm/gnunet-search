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
#include <inttypes.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <crawl.h>

#include "../util/service-util.h"
#include "../storage/storage.h"
#include "../dht/dht.h"
#include "../normalization/normalization.h"

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
	char *url = (char*) GNUNET_malloc(url_length + 1);
	memcpy(url, data + position, url_length);
	url[url_length] = 0;

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
