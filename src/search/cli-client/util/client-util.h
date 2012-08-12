/**
 * @file search/cli-client/util/client-util.h
 * @author Julian Kranz
 * @date 5.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search client's utility component.
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

#ifndef UTIL_H_
#define UTIL_H_

extern size_t gnunet_search_util_urls_read(char ***urls, const char *file);
extern size_t gnunet_search_util_serialize(char const * const *urls, size_t urls_length, char **buffer);
extern void gnunet_search_util_replace(char *text, size_t size, char a, char b);

#endif /* UTIL_H_ */
