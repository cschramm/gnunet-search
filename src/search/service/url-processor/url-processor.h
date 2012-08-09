/*
 * gnunet-search-url-processor.h
 *
 *  Created on: Aug 3, 2012
 *      Author: jucs
 */

#ifndef GNUNET_SEARCH_URL_PROCESSOR_H_
#define GNUNET_SEARCH_URL_PROCESSOR_H_

#include "gnunet_protocols_search.h"

extern void gnunet_search_url_processor_incoming_url_process(size_t prefix_length, const void *data, size_t size);
extern size_t gnunet_search_url_processor_cmd_urls_get(char ***urls, struct search_command const *cmd);

#endif /* GNUNET_SEARCH_URL_PROCESSOR_H_ */
