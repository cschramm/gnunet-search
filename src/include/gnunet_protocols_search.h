/*
     This file is part of GNUnet.
     (C) 

     GNUnet is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 2, or (at your
     option) any later version.

     GNUnet is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with GNUnet; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

/**
 * @file include/gnunet_protocols_search.h
 * @brief constants for network protocols
 * @author 
 */

#ifndef GNUNET_PROTOCOLS_SEARCH_H
#define GNUNET_PROTOCOLS_SEARCH_H

#ifdef __cplusplus
extern "C"
{
#if 0                           /* keep Emacsens' auto-indent happy */
}
#endif
#endif



/**
 * SEARCH messages
 */
#define GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED (1 << 0)
#define GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT (1 << 2)

struct message_header {
	uint8_t flags;
} __attribute__((packed));

#define GNUNET_MESSAGE_TYPE_SEARCH 0x4242

#define GNUNET_SEARCH_ACTION_SEARCH 0x00
#define GNUNET_SEARCH_ACTION_ADD 0x01

struct search_command {
	uint8_t action;
	uint64_t size;
} __attribute__((packed));

#define GNUNET_SEARCH_RESPONSE_TYPE_RESULT 0x00
#define GNUNET_SEARCH_RESPONSE_TYPE_DONE 0x01

struct search_response {
	uint8_t type;
	uint64_t size;
} __attribute__((packed));

#if 0                           /* keep Emacsens' auto-indent happy */
{
#endif
#ifdef __cplusplus
}
#endif

/* ifndef GNUNET_PROTOCOLS_H */
#endif
/* end of gnunet_protocols.h */
