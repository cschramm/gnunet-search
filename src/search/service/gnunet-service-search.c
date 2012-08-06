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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_dht_service.h>
#include "gnunet_protocols_search.h"

#include "service/url-processor/gnunet-search-url-processor.h"
#include "service/util/gnunet-search-util.h"
#include "service/client-communication/client-communication.h"

/**
 * Our configuration.
 */
static const struct GNUNET_CONFIGURATION_Handle *cfg;

static struct GNUNET_DHT_GetHandle *dht_get_handle;

static void search_key_value_generate_simple(char **key_value, const char *action, const char *data) {
	size_t key_value_length;
	FILE *key_value_stream = open_memstream(key_value, &key_value_length);

	fprintf(key_value_stream, "search:%s:%s", action, data);
	fputc(0, key_value_stream);

	fclose(key_value_stream);
}

/*
 * Security?
 */
static void search_cmd_keyword_get(char **keyword, struct search_command const *cmd) {
	*keyword = (char*) malloc(strlen((char*) (cmd + 1)) + 1);
	strcpy(*keyword, (char*) (cmd + 1));
}

static size_t search_cmd_urls_get(char ***urls, struct search_command const *cmd) {
	char const *urls_source = (char*) (cmd + 1);

	size_t urls_length;
	FILE *url_stream = open_memstream((char**) urls, &urls_length);

	size_t read_length = sizeof(struct search_command);
	size_t urls_number = 0;
	while (read_length < cmd->size) {
		/*
		 * Todo: Security...
		 */
		size_t url_length = strlen(urls_source);
		char *url = (char*) malloc(url_length + 1);
		memcpy(url, urls_source, url_length + 1);

		fwrite(&url, sizeof(url), 1, url_stream);

		read_length += url_length + 1;
		urls_source += url_length + 1;
		urls_number++;

//		printf("url: %s\n", url);
	}

//	printf("read_length: %lu\n", read_length);

	fclose(url_stream);

//	printf("urls_length: %lu\n", urls_length);

	return urls_number;
}

static void search_dht_get_result_iterator_and_send_to_user(void *cls, struct GNUNET_TIME_Absolute exp,
		const GNUNET_HashCode * key, const struct GNUNET_PeerIdentity * get_path, unsigned int get_path_length,
		const struct GNUNET_PeerIdentity * put_path, unsigned int put_path_length, enum GNUNET_BLOCK_Type type,
		size_t size, const void *data) {

//	char *dstr = malloc(size + 1);
//	memcpy(dstr, data, size);
//	dstr[size] = 0;
//	printf("Got data: %s\n", dstr);
//	free(dstr);
//
//	printf("Client: %lu\n", cls);

	GNUNET_DHT_get_stop(dht_get_handle);

	//search_send_result("hallo", 6, (struct GNUNET_SERVER_Client *) cls);

	gnunet_search_client_communication_send_result(data, size, GNUNET_SEARCH_RESPONSE_TYPE_RESULT, (struct GNUNET_SERVER_Client *) cls);
}

static void search_dht_get_and_send_to_user(char const *keyword, struct GNUNET_SERVER_Client *client) {
	char *key_value;
	search_key_value_generate_simple(&key_value, "keyword", keyword);

	printf("Searching for: %s\n", key_value);

	//char *keyword = "search:keyword:hallo";

	GNUNET_HashCode hash;
	GNUNET_CRYPTO_hash(key_value, strlen(key_value), &hash);

	free(key_value);

	dht_get_handle = GNUNET_DHT_get_start(gnunet_search_dht_handle, GNUNET_BLOCK_TYPE_TEST, &hash, 3,
			GNUNET_DHT_RO_NONE, NULL, 0, &search_dht_get_result_iterator_and_send_to_user, client);
}

static void search_process(char const *keyword, struct GNUNET_SERVER_Client *client) {
	search_dht_get_and_send_to_user(keyword, client);
}

/**
 * Handle message from client.
 *
 * @param cls closure
 * @param client identification of the client
 * @param message the actual message
 * @return GNUNET_OK to keep the connection open,
 *         GNUNET_SYSERR to close it (signal serious error)
 */
static void gnunet_service_search_client_message_handle(void *cls, struct GNUNET_SERVER_Client *client,
		const struct GNUNET_MessageHeader *message) {
	GNUNET_SERVER_receive_done(client, GNUNET_OK);
	GNUNET_SERVER_client_keep(client);

	struct message_header *msg_header = (struct message_header*) (message + 1);

	struct search_command *cmd = (struct search_command*) (msg_header + 1);

	//printf("Message: size = %lu\n", htons(message->size));
	printf("Command: action = %u, size = %zu, flags = 0x%x\n", cmd->action, cmd->size, msg_header->flags);

	if (cmd->action == GNUNET_SEARCH_ACTION_SEARCH) {
		char *keyword;
		search_cmd_keyword_get(&keyword, cmd);

		printf("Searching keyword: %s...\n", keyword);

		search_process(keyword, client);

		free(keyword);
	}
	if (cmd->action == GNUNET_SEARCH_ACTION_ADD) {
		char **urls;
		size_t urls_length = search_cmd_urls_get(&urls, cmd);

		gnunet_search_util_dht_url_list_put(urls, urls_length, 2);

		gnunet_search_client_communication_send_result(NULL, 0, GNUNET_SEARCH_RESPONSE_TYPE_DONE, client);

		for (size_t i = 0; i < urls_length; ++i)
			free(urls[i]);
		free(urls);
	}

//	printf("blah :-)\n");
}

/**
 * Task run during shutdown.
 *
 * @param cls unused
 * @param tc unused
 */
static void shutdown_task(void *cls, const struct GNUNET_SCHEDULER_TaskContext *tc) {
}

/**
 * A client disconnected.  Remove all of its data structure entries.
 *
 * @param cls closure, NULL
 * @param client identification of the client
 */
static void handle_client_disconnect(void *cls, struct GNUNET_SERVER_Client * client) {
}

static void search_dht_monitor_put(void *cls, enum GNUNET_DHT_RouteOption options, enum GNUNET_BLOCK_Type type,
		uint32_t hop_count, uint32_t desired_replication_level, unsigned int path_length,
		const struct GNUNET_PeerIdentity *path, struct GNUNET_TIME_Absolute exp, const GNUNET_HashCode * key,
		const void *data, size_t size) {
	char const *prefix = "search:url:";
	size_t prefix_length = strlen(prefix);
	if (size < prefix_length + 1)
		return;
	if (!strncmp(prefix, data, prefix_length)) {
		gnunet_search_url_processor_incoming_url_process(prefix_length, data, size);
	}
}

/**
 * Process statistics requests.
 *
 * @param cls closure
 * @param server the initialized server
 * @param c configuration to use
 */
static void run(void *cls, struct GNUNET_SERVER_Handle *server, const struct GNUNET_CONFIGURATION_Handle *c) {
	static const struct GNUNET_SERVER_MessageHandler handlers[] = { { &gnunet_service_search_client_message_handle,
			NULL, GNUNET_MESSAGE_TYPE_SEARCH, 0 }, { NULL, NULL, 0, 0 } };
	cfg = c;
	GNUNET_SERVER_add_handlers(server, handlers);
	GNUNET_SERVER_disconnect_notify(server, &handle_client_disconnect, NULL);
	GNUNET_SCHEDULER_add_delayed(GNUNET_TIME_UNIT_FOREVER_REL, &shutdown_task, NULL);

	gnunet_search_dht_handle = GNUNET_DHT_connect(cfg, 3);

	GNUNET_DHT_monitor_start(gnunet_search_dht_handle, GNUNET_BLOCK_TYPE_TEST, NULL, NULL, NULL,
			&search_dht_monitor_put, NULL);

//	char **urls;
//	size_t urls_length;// = search_cmd_urls_get(&urls, cmd);
//
//	char *hugo = "http://www.heise.de/";
////	char *hugoM = malloc(strlen(hugo) + 1);
////	memcpy(hugoM, hugo, strlen(hugo) + 1);
////
//	char *inge = "http://www.google.de/";
////	char *ingeM = malloc(strlen(inge) + 1);
////	memcpy(ingeM, inge, strlen(inge) + 1);
////
//	char *egon = "http://www.golem.de/";
////	char *egonM = malloc(strlen(egon) + 1);
////	memcpy(egonM, egon, strlen(egon) + 1);
//
//	urls = (char**)malloc(sizeof(char*)*3);
//	urls[0] = hugo;
//	urls[1] = inge;
//	urls[2] = egon;
//	urls_length = 3;
////
//	search_dht_url_list_put(urls, urls_length);

//	search_dht_string_string_put(hugoM, "lala");
//	free(hugoM);
//	search_dht_string_string_put(ingeM, "lala1");
//	free(ingeM);
//	search_dht_string_string_put(egonM, "lala2");
//	free(egonM);

}

/**
 * The main function for the ext service.
 *
 * @param argc number of arguments from the command line
 * @param argv command line arguments
 * @return 0 ok, 1 on error
 */
int main(int argc, char * const *argv) {
	return (GNUNET_OK == GNUNET_SERVICE_run(argc, argv, "search", GNUNET_SERVICE_OPTION_NONE, &run, NULL)) ? 0 : 1;
}

/* end of gnunet-service-ext.c */
