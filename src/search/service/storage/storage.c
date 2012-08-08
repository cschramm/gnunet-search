/*
 * storage.c
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "storage.h"
#include "../globals/globals.h"

static al_dictionary_t *storage;

static char string_compare(void const *a, void const *b) {
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

void gnunet_search_storage_init() {
	storage = al_dictionary_construct(&string_compare);
}

void gnunet_search_storage_free() {
	/*
	 * Todo: ;-)
	 */
}

void gnunet_search_storage_key_value_add(char const *key, char const *value) {
	char search_result;
	void const *from_storage = al_dictionary_get(storage, &search_result, key);

	size_t key_length = strlen(key);
	size_t value_length = strlen(value);

	array_list_t *known_values;

	if(search_result) {
		known_values = array_list_construct();

		char *key_copy = (char*) malloc(key_length + 1);
		memcpy(key_copy, key, key_length + 1);

		char *value_copy = (char*) malloc(value_length + 1);
		memcpy(value_copy, value, value_length + 1);

		array_list_insert(known_values, value_copy);

		al_dictionary_insert(storage, key_copy, known_values);
	} else {
		known_values = (array_list_t const*) from_storage;

		size_t known_values_length = array_list_get_length(known_values);
		for(long int i = 0; i < known_values_length; ++i) {
			const char *element;
			array_list_get(known_values, &element, i);
			if(!strcmp(element, value))
				return;
		}

		char *value_copy = (char*) malloc(value_length + 1);
		memcpy(value_copy, value, value_length + 1);

		array_list_insert(known_values, value_copy);
	}
}

array_list_t *gnunet_search_storage_values_get(char const *key) {
	char search_result;
	array_list_t *from_storage = (array_list_t*) al_dictionary_get(storage, &search_result, key);

	if(search_result)
		return NULL;
	else
		return from_storage;
}

size_t gnunet_search_storage_value_serialize(char **buffer, array_list_t *values, size_t maximal_size) {
	size_t buffer_size;

	FILE *memstream = open_memstream(buffer, &buffer_size);

	size_t values_length = array_list_get_length(values);
	for(long int i = 0; i < values_length; ++i) {
		char *next;
		array_list_get(values, &next, i);
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
