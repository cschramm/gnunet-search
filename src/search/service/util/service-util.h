/**
 * @file search/service/util/service-util.h
 * @author Julian Kranz
 * @date 3.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search service's utility component.
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

#ifndef GNUNET_SEARCH_UTIL_H_
#define GNUNET_SEARCH_UTIL_H_

#include "gnunet_protocols_search.h"

extern void gnunet_search_util_key_value_generate_simple(char **key_value, const char *action, const char *data);
extern void gnunet_search_util_cmd_keyword_get(char **keyword, struct search_command const *cmd, size_t size);

#endif /* GNUNET_SEARCH_UTIL_H_ */
