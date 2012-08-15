/**
 * @file search/service/url-processor/url-processor.h
 * @author Julian Kranz
 * @date 3.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search service's url processor component.
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

#ifndef GNUNET_SEARCH_URL_PROCESSOR_H_
#define GNUNET_SEARCH_URL_PROCESSOR_H_

#include "gnunet_protocols_search.h"

extern void gnunet_search_url_processor_incoming_url_process(size_t prefix_length, const void *data, size_t size);
extern size_t gnunet_search_url_processor_cmd_urls_get(char ***urls, struct search_command const *cmd);

#endif /* GNUNET_SEARCH_URL_PROCESSOR_H_ */
