/**
 * @file search/service/storage/storage.c
 * @author Julian Kranz
 * @date 7.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's storage component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This file contains all functions pertaining to the GNUnet Search service's storage component. This component takes care of storing the
 * local keyword URL combinations. It also provides functions to add additional URLs and keywords and to search the database.
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

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>

#include "storage.h"
#include "../globals/globals.h"

/**
 * @brief This variable stores a reference to a dictionary containing the locally stored data.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This variable stores a reference to a dictionary containing the locally stored data. The keys of the dictionary are the normalized (see the normalization component) keywords;
 * the values are dynamic arrays storing sets of URLs.
 */
static al_dictionary_t *storage;

/**
 * @brief This function compares two strings and is used by the dictionary to compare the keys; this enables the dictionary to sort the keys and thus access them faster.
 *
 * @param a the first string
 * @param b the second string
 *
 * @return a byte value indicating whether a is greater than b (1), equals b (0) or is smaller than b (-1)
 */
static char gnunet_search_storage_string_compare(void const *a, void const *b) {
	char *_a = (char*)a;
	char *_b = (char*)b;
	size_t index = 0;
	while(1) {
		if(!_a[index] && _b[index])
			return -1;
		if(_a[index] && !_b[index])
			return 1;
		if(!_a[index] && !_b[index])
			break;
		if(_a[index] < _b[index])
			return -1;
		else if(_a[index] > _b[index])
			return 1;
		index++;
	}
	return 0;
}

/**
 * @brief This function is used to free a value contained the dictionary; since such a value is a dynamic array (see above) the function has to free such a dynamisch array.
 *
 * @param arraylist the dynamic array to free
 */
static void gnunet_search_storage_string_arraylist_free(void *arraylist) {
	array_list_t *_arraylist = (array_list_t*)arraylist;
	size_t _arraylist_size = array_list_get_length(_arraylist);
	for (long int i = 0; i < _arraylist_size; ++i) {
		void *value;
		array_list_get(_arraylist, (void const **)&value, i);
		GNUNET_free(value);
	}
	array_list_free(_arraylist);
}

/**
 * @brief This function initialises the storage component.
 */
void gnunet_search_storage_init() {
	storage = al_dictionary_construct(&gnunet_search_storage_string_compare);
}

/**
 * @brief This function releases all resources held by the storage component.
 */
void gnunet_search_storage_free() {
	al_dictionary_remove_and_free_all(storage, &free, &gnunet_search_storage_string_arraylist_free);
	al_dictionary_free(storage);
}

/**
 * @brief This function adds a new key value combination to the storage.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function adds a new key value combination to the storage. For this purpose it first checks whether the key is already contained in the dictionary implementing the
 * storage. If it is not a new dynamic array is created containing only the new value and the data is added to the dictionary. If the key is already contained its corresponding
 * value array is searched for the value to insert. If the value is not yet contained in the dynamic array belonging to the key it is inserted.
 *
 * @param key the key to add (the normalized search keyword)
 * @param value the value of the key (the URL of the website the keyword has been found on)
 */
void gnunet_search_storage_key_value_add(char const *key, char const *value) {
	char search_result;
	void const *from_storage = al_dictionary_get(storage, &search_result, key);

	size_t key_length = strlen(key);
	size_t value_length = strlen(value);

	array_list_t *known_values;

	if(search_result) {
		known_values = array_list_construct();

		char *key_copy = (char*) GNUNET_malloc(key_length + 1);
		memcpy(key_copy, key, key_length + 1);

		char *value_copy = (char*) GNUNET_malloc(value_length + 1);
		memcpy(value_copy, value, value_length + 1);

		array_list_insert(known_values, value_copy);

		al_dictionary_insert(storage, key_copy, known_values);
	} else {
		known_values = (array_list_t *) from_storage;

		size_t known_values_length = array_list_get_length(known_values);
		for(long int i = 0; i < known_values_length; ++i) {
			const char *element;
			array_list_get(known_values, (void const **)&element, i);
			if(!strcmp(element, value))
				return;
		}

		char *value_copy = (char*) GNUNET_malloc(value_length + 1);
		memcpy(value_copy, value, value_length + 1);

		array_list_insert(known_values, value_copy);
	}
}

/**
 * @brief This function gets the value array for a specific key.
 *
 * @param key the to get the value array for
 *
 * @return the value array; if the key is not contained in the dictionary NULL is returned.
 */
array_list_t *gnunet_search_storage_values_get(char const *key) {
	char search_result;
	array_list_t *from_storage = (array_list_t*) al_dictionary_get(storage, &search_result, key);

	if(search_result)
		return NULL;
	else
		return from_storage;
}

/**
 * @brief This function serializes a value array.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function serializes a value array in order to be able to send it as part of a (flooding answer) message. Since such a message
 * has a maximal payload size URLs are only added as long as they fully fit into a message's payload.
 *
 * @param buffer a reference to a memory location to store the reference to the serialized buffer in
 * @param value the value array to serialize
 * @param maximal_size the maximal size of serialized data (see above)
 *
 * @return the actual size of the serialized data
 */
size_t gnunet_search_storage_value_serialize(char **buffer, array_list_t *values, size_t maximal_size) {
	size_t buffer_size;

	FILE *memstream = open_memstream(buffer, &buffer_size);

	size_t values_length = array_list_get_length(values);
	for(long int i = 0; i < values_length; ++i) {
		char *next;
		array_list_get(values, (void const **)&next, i);
		size_t next_size = strlen(next) + 1;
		fflush(memstream);
		if(buffer_size + next_size <= maximal_size)
			fwrite(next, 1, next_size, memstream);
		else
			break;
	}

	fclose(memstream);

	return buffer_size;
}
