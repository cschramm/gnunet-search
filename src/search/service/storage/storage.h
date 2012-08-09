/*
 * storage.h
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
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
