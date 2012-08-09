/*
 * util.h
 *
 *  Created on: Aug 5, 2012
 *      Author: jucs
 */

#ifndef UTIL_H_
#define UTIL_H_

extern size_t gnunet_search_util_urls_read(char ***urls, const char *file);
extern size_t gnunet_search_util_serialize(char const * const *urls, size_t urls_length, char **buffer);
extern void gnunet_search_util_replace(char *text, size_t size, char a, char b);

#endif /* UTIL_H_ */
