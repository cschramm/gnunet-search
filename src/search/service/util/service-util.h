/*
 * gnunet-search-util.h
 *
 *  Created on: Aug 3, 2012
 *      Author: jucs
 */

#ifndef GNUNET_SEARCH_UTIL_H_
#define GNUNET_SEARCH_UTIL_H_

#include "gnunet_protocols_search.h"

extern void gnunet_search_util_key_value_generate_simple(char **key_value, const char *action, const char *data);
extern void gnunet_search_util_cmd_keyword_get(char **keyword, struct search_command const *cmd);

#endif /* GNUNET_SEARCH_UTIL_H_ */
