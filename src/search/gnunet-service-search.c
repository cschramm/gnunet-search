/*
 This file is part of GNUnet.
 (C)

 GNUnet is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 3, or (at your
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
 * @file ext/gnunet-service-ext.c
 * @brief ext service implementation
 * @author Christian Grothoff
 */

#include <string.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>
#include "gnunet_protocols_search.h"

/**
 * Our configuration.
 */
static const struct GNUNET_CONFIGURATION_Handle *cfg;
static struct GNUNET_DHT_Handle *dht_handle;

static void search_dht_string_string_put(const char *key, const char *value) {
	size_t key_length = strlen(key);

	GNUNET_HashCode hash;
	GNUNET_CRYPTO_hash(key, key_length, &hash);

	size_t value_length = strlen(value);

	GNUNET_DHT_put(dht_handle, &hash, 2, GNUNET_DHT_RO_NONE,
			GNUNET_BLOCK_TYPE_TEST, value_length + 1, value,
			GNUNET_TIME_absolute_get_forever(),
			GNUNET_TIME_relative_get_forever(), /*&message_sent_cont*/NULL, NULL);
}

static void search_dht_url_list_put(char **urls, size_t size) {
	for (int i = 0; i < size; ++i) {
		const char *key_prefix = "search:url:";
		size_t key_prefix_length = strlen(key_prefix);
		size_t key_url_length = strlen(urls[i]);

		char *key = (char*)malloc(key_prefix_length + key_url_length + 1);
		memcpy(key, key_prefix, key_prefix_length);
		memcpy(key + key_prefix_length, urls[i], key_url_length);
		key[key_prefix_length + key_url_length] = 0;

		search_dht_string_string_put(key, urls[i]);

		free(key);
	}
}

/**
 * Handle EXT-message.
 *
 * @param cls closure
 * @param client identification of the client
 * @param message the actual message
 * @return GNUNET_OK to keep the connection open,
 *         GNUNET_SYSERR to close it (signal serious error)
 */
static void handle_search(void *cls, struct GNUNET_SERVER_Client *client,
		const struct GNUNET_MessageHeader *message) {
	GNUNET_SERVER_receive_done(client, GNUNET_OK);

	printf("blah :-)\n");
}

/**
 * Task run during shutdown.
 *
 * @param cls unused
 * @param tc unused
 */
static void shutdown_task(void *cls,
		const struct GNUNET_SCHEDULER_TaskContext *tc) {
}

/**
 * A client disconnected.  Remove all of its data structure entries.
 *
 * @param cls closure, NULL
 * @param client identification of the client
 */
static void handle_client_disconnect(void *cls,
		struct GNUNET_SERVER_Client * client) {
}

/**
 * Process statistics requests.
 *
 * @param cls closure
 * @param server the initialized server
 * @param c configuration to use
 */
static void run(void *cls, struct GNUNET_SERVER_Handle *server,
		const struct GNUNET_CONFIGURATION_Handle *c) {
	static const struct GNUNET_SERVER_MessageHandler handlers[] = { {
			&handle_search, NULL, GNUNET_MESSAGE_TYPE_SEARCH_URLS, 0 },
			{ NULL, NULL, 0, 0 } };
	cfg = c;
	GNUNET_SERVER_add_handlers(server, handlers);
	GNUNET_SERVER_disconnect_notify(server, &handle_client_disconnect, NULL);
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &shutdown_task,
			NULL);

	dht_handle = GNUNET_DHT_connect(cfg, 3);
}

/**
 * The main function for the ext service.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int main(int argc, char * const *argv) {
	return (GNUNET_OK
			== GNUNET_SERVICE_run(argc, argv, "search",
					GNUNET_SERVICE_OPTION_NONE, &run, NULL)) ? 0 : 1;
}

/* end of gnunet-service-ext.c */
