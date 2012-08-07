/*
 * flooding.c
 *
 *  Created on: Aug 7, 2012
 *      Author: jucs
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gnunet/platform.h>
#include <gnunet/gnunet_util_lib.h>
#include <gnunet/gnunet_core_service.h>
#include "flooding.h"

static struct gnunet_search_flooding_data_flood_parameters {
	struct GNUNET_PeerIdentity *source;
	void *data;
	size_t size;
};

static struct gnunet_search_flooding_routing_entry {
	uint64_t flow_id;
	struct GNUNET_PeerIdentity *next_hop;
};

static struct gnunet_search_flooding_routing_entry *gnunet_search_flooding_routing_table;
/*
 * Move that somewhere...
 */
static const struct GNUNET_CONFIGURATION_Handle *_cfg;

void gnunet_search_flooding_init(const struct GNUNET_CONFIGURATION_Handle *cfg) {
	_cfg = cfg;
	gnunet_search_flooding_routing_table = (struct gnunet_search_flooding_routing_entry *) malloc(
			sizeof(struct gnunet_search_flooding_routing_entry *) * GNUNET_SEARCH_FLOODING_ROUTING_TABLE_SIZE);
}

void gnunet_search_flooding_free() {
	free(gnunet_search_flooding_routing_table);
}

static void iterate_handler(void *cls, const struct GNUNET_PeerIdentity *peer,
		const struct GNUNET_ATS_Information *atsi, unsigned int atsi_count) {
	if(!peer) {
		printf("Iterating done...\n");
		free(cls);
		return;
	}
	struct gnunet_search_flooding_data_flood_parameters *parameters =
			(struct gnunet_search_flooding_data_flood_parameters*) cls;
	if(parameters->source && !GNUNET_CRYPTO_hash_cmp(&parameters->source->hashPubKey, &peer->hashPubKey)) {
		printf("Skipping sender...\n");
		return;
	}

	struct GNUNET_CRYPTO_HashAsciiEncoded result;
	GNUNET_CRYPTO_hash_to_enc(&peer->hashPubKey, &result);

	printf("Flooding message to peer %.*s...\n", 104, &result);
	printf("Next peer...\n");
}

void gnunet_search_flooding_data_flood(struct GNUNET_PeerIdentity *source, void *data, size_t size) {
	struct gnunet_search_flooding_data_flood_parameters *parameters =
			(struct gnunet_search_flooding_data_flood_parameters*) malloc(
					sizeof(struct gnunet_search_flooding_data_flood_parameters));
	parameters->source = source;
	parameters->data = data;
	parameters->size = size;

	GNUNET_CORE_iterate_peers(_cfg, &iterate_handler, parameters);
	fflush(stdout);

	printf("gnunet_search_flooding_data_flood()\n");
}
