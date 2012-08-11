/**
 * @file include/gnunet_protocols_search.h
 * @brief This file defines all exported data structures and constants pertaining to
 * the GNUnet Search communication protocol.
 * @author Julian Kranz
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

#ifndef GNUNET_PROTOCOLS_SEARCH_H
#define GNUNET_PROTOCOLS_SEARCH_H

#ifdef __cplusplus
extern "C"
{
#if 0                           /* keep Emacsens' auto-indent happy */
}
#endif
#endif

#include <stdint.h>

/**
 * @brief This constant defines a flag used to indicate message fragmentation.
 */
#define GNUNET_MESSAGE_SEARCH_FLAG_FRAGMENTED (1 << 0)

/**
 * @brief This constant defines a flag used to indicate that the current part of the message
 * is the last fragment of the message.
 */
#define GNUNET_MESSAGE_SEARCH_FLAG_LAST_FRAGMENT (1 << 1)

/**
 * @brief This data structure is used as a header for every message exchanged between the
 * client and the service. This additional header is used to generically implement message
 * fragmentation independent of the data transmitted.
 */
struct __attribute__((__packed__)) message_header {
	/**
	 * @brief This member saves the flags associated with the message.
	 */
	uint8_t flags;
};

/**
 * @brief This constant defines the GNUnet message type for messages exchanged between
 * the search client and the service.
 */
#define GNUNET_MESSAGE_TYPE_SEARCH 0x4242

/**
 * @brief This constant defines the GNUnet message type for messages used to flood search
 * requests through the network or deliver answers back to the requestor.
 */
#define GNUNET_MESSAGE_TYPE_SEARCH_FLOODING 0x2424

/**
 * @brief This constant defines a numerical code used by the client to tell the service that
 * the request sent is a search request.
 */
#define GNUNET_SEARCH_ACTION_SEARCH 0x00

/**
 * @brief This constant defines a numerical code used by the client to tell the service that
 * the request sent is a request to scan new URLs for keywords and add them to the distributed
 * database.
 */
#define GNUNET_SEARCH_ACTION_ADD 0x01

/**
 * @brief This data structure is sent to the service; it contains attributes needed to process
 * a client's request. As the client and service are thought to run on the same machine the
 * host's byte order is used for every attribute.
 */
struct __attribute__((__packed__)) search_command {
	/**
	 * This member defines the action requested to be performed by the service. For more details
	 * see the constant definitions above.
	 */
	uint8_t action;
	/**
	 * This member saves a message id chosen by the client. The service will use the same id for
	 * every answer to that request. If answers cannot be associated with a request the id of the
	 * response message is set to 0.
	 */
	uint16_t id;
	/**
	 * This member saves the total size of the request. The service has to use that value to check
	 * whether all fragments have been received correctly. The service has to verify that the buffer
	 * received has exactly got that size (i.e. the size field contains the aggregated size of all
	 * GNUnet messages received for that request).
	 */
	uint64_t size;
};

/**
 * @brief This constant defines a numerical code used by the service to tell the client about
 * the type of the response received by it. This constant is used for a response containing result
 * data.
 */
#define GNUNET_SEARCH_RESPONSE_TYPE_RESULT 0x00

/**
 * @brief This constant defines a numerical code used by the service to tell the client about
 * the type of the response received by it. This constant is used for a response used only as
 * an acknowledgement.
 */
#define GNUNET_SEARCH_RESPONSE_TYPE_DONE 0x01

/**
 * @brief This data structure is sent to the client; it contains attributes needed to process
 * a service's answer. As the client and service are thought to run on the same machine the
 * host's byte order is used for every attribute.
 */
struct __attribute__((__packed__)) search_response {
	/**
	 * This member defines the type of the answer message. For more details see the constant
	 * definitions above.
	 */
	uint8_t type;
	/**
	 * This member saves the message id. It has to match the id chosen by the client for the
	 * corresponding request (see above).
	 */
	uint16_t id;
	/**
	 * This member saves the total size of the response. The client has to use that value to check
	 * whether all fragments have been received correctly. The client has to verify that the buffer
	 * received has exactly got that size (i.e. the size field contains the aggregated size of all
	 * GNUnet messages received for that request).
	 */
	uint64_t size;
};

#if 0                           /* keep Emacsens' auto-indent happy */
{
#endif
#ifdef __cplusplus
}
#endif

/* ifndef GNUNET_PROTOCOLS_H */
#endif
/* end of gnunet_protocols.h */
