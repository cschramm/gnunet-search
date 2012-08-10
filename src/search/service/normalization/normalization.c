/*
 * normalization.c
 *
 *  Created on: Aug 10, 2012
 *      Author: jucs
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "normalization.h"

void gnunet_search_normalization_keyword_normalize(char *keyword) {
	for (size_t i = 0; keyword[i]; ++i)
		if(keyword[i] >= 'A' && keyword[i] <= 'Z')
			keyword[i] += 'a' - 'A';
}
