/**
 * @file search/service/normalization/normalization.c
 * @author Julian Kranz
 * @date 10.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's normalization component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This file contains all functions pertaining to the GNUnet Search service's normalization component. This component normalizes keywords in order
 * to be able to associate similar keywords with each other.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "normalization.h"

/**
 * @brief This function normalizes a keyword.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This function normalizes a keyword. Currently that only means changing all uppercase letters to lowercase letters.
 *
 * @param keyword the keyword to normalize; the function works on the parameter itself.
 */
void gnunet_search_normalization_keyword_normalize(char *keyword) {
	for (size_t i = 0; keyword[i]; ++i)
		if(keyword[i] >= 'A' && keyword[i] <= 'Z')
			keyword[i] += 'a' - 'A';
}
