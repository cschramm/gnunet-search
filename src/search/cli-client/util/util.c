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

size_t urls_read(char ***urls, const char *file) {
	FILE *fh = fopen(file, "r");
	if (fh == NULL) {
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

		if (line_length + 1 + eof > line_size) {
			line_size <<= 1;
			line = realloc(line, line_size);
		}

		if (!eof)
			line[line_length++] = next;

		if ((next == '\n' || eof) && line_length > 0) {
			if (next == '\n')
				line_length--;
			line[line_length++] = 0;

			//printf("%s\n", line);
			if (urls_length + 1 > urls_size) {
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
	} while (!eof);

	free(line);

	fclose(fh);

	return urls_length;
}
