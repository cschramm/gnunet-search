/*
 * util.c
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gnunet_protocols_search.h"

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
	*urls = (char**) malloc(sizeof(char*) * urls_size);

	size_t line_size = 64;
	size_t line_length = 0;
	char *line = (char*) malloc(line_size);

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
			(*urls)[urls_length] = (char*) malloc(line_length);
			memcpy(*(*urls + urls_length), line, line_length);
			urls_length++;

			line_length = 0;
		}

		//		char *line;
//		fscanf(fh, "%as", &line);
//		printf("Read: %s", line);
//		free(line);
	} while(!eof);

	free(line);

	fclose(fh);

	return urls_length;
}

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

void gnunet_search_util_replace(char *text, size_t size, char a, char b) {
	for(size_t index = 0; index < size; ++index)
		if(text[index] == a)
			text[index] = b;
}
