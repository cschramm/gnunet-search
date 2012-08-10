/**
 * @file search/cli-client/util/client-util.c
 * @author Julian Kranz
 * @date 5.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search client's utility component.
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

#include "gnunet_protocols_search.h"

/**
 * @brief This function reads a file containing URLs and constructs an array from its data.
 *
 * @param urls a pointer to an address used for the array
 * @param file the file to read the data from
 *
 * @return the size of the array
 */
size_t gnunet_search_util_urls_read(char ***urls, const char *file) {
	FILE *fh = fopen(file, "r");
	if(fh == NULL) {
		printf("Error opening file...");
		return 0;
		/**
		 * Todo: Handle error
		 */
	}

	size_t urls_size = 32;
	size_t urls_length = 0;
	*urls = (char**) GNUNET_malloc(sizeof(char*) * urls_size);

	size_t line_size = 64;
	size_t line_length = 0;
	char *line = (char*) GNUNET_malloc(line_size);

	int eof;
	do {
		char next;
		size_t read = fread(&next, 1, 1, fh);
		eof = feof(fh) && !read;

		if(line_length + 1 + eof > line_size) {
			line_size <<= 1;
			line = realloc(line, line_size);
		}

		if(!eof)
			line[line_length++] = next;

		if((next == '\n' || eof) && line_length > 0) {
			if(next == '\n')
				line_length--;
			line[line_length++] = 0;

			//printf("%s\n", line);
			if(urls_length + 1 > urls_size) {
				urls_size <<= 1;
				*urls = (char**) realloc(*urls, urls_size);
			}
			(*urls)[urls_length] = (char*) GNUNET_malloc(line_length);
			memcpy(*(*urls + urls_length), line, line_length);
			urls_length++;

			line_length = 0;
		}

		//		char *line;
//		fscanf(fh, "%as", &line);
//		printf("Read: %s", line);
//		free(line);
	} while(!eof);

	GNUNET_free(line);

	fclose(fh);

	return urls_length;
}

/**
 * @brief This function serializes and an array of URLs
 *
 * @param urls the array of urls
 * @param urls_length the length of the array
 * @param buffer a pointer to an address used to return the serialized data
 *
 * @return the size of the serialized data
 */
size_t gnunet_search_util_serialize(char const * const *urls, size_t urls_length, void **buffer) {
	size_t serialized_size;
	FILE *memstream = open_memstream((char**) buffer, &serialized_size);

	fseek(memstream, sizeof(struct search_command), SEEK_CUR);

	for(size_t url_index = 0; url_index < urls_length; ++url_index) {
		size_t url_length = strlen(urls[url_index]);
		fwrite(urls[url_index], 1, url_length, memstream);
		fputc(0, memstream);
	}

	fclose(memstream);

	return serialized_size;
}

/**
 * @brief This function replaces a character inside a string by another character
 *
 * @param text the text to replace characters in
 * @param size the amount of memory to process
 * @param a the character which it is searched for
 * @param b the replacement character
 */
void gnunet_search_util_replace(char *text, size_t size, char a, char b) {
	for(size_t index = 0; index < size; ++index)
		if(text[index] == a)
			text[index] = b;
}
