/**
 * @file search/service/storage/storage.h
 * @author Julian Kranz
 * @date 7.8.2012
 *
 * @brief This file defines all exported data structures, functions, constants and variables pertaining to
 * the GNUnet Search service's storage component.
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

#ifndef STORAGE_H_
#define STORAGE_H_

#include <collections/aldictionary/aldictionary.h>
#include <collections/arraylist/arraylist.h>

extern void gnunet_search_storage_init();
extern void gnunet_search_storage_free();
extern void gnunet_search_storage_key_value_add(char const *key, char const *value);
extern array_list_t *gnunet_search_storage_values_get(char const *key);
extern size_t gnunet_search_storage_value_serialize(char **buffer, array_list_t *values, size_t maximal_size);

#endif /* STORAGE_H_ */
