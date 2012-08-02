/**
 * Representation of a local file
 */
struct local_file {
	char * name;
	char * type;
	struct local_file * next;
};

/**
 * Result set for a specific query
 */
struct search_results {
	char *query;
	char *result;
	struct search_results * next;
};

/**
 * webserver context
 */
struct webserver_context {
	struct local_file * first_local_file; //!< pointer to a linked list of local files
	struct search_results * first_results; //!< pointer to a linked list of search result sets
	struct search_results * last_results; //!< pointer to the last element of the search result set list
	struct GNUNET_CONFIGURATION_Handle * gnunet_cfg; //!< GNUnet configuration handle
	struct MHD_Daemon * daemon; //!< associated libmicrohttpd server
};


/**
 * Gather list of local files and start libmicrohttpd server
 *
 * @param wether or not the webserver shall only be available from localhost
 * @param port TCP port to use
 * @param gnunet_cfg GNUnet configuration handle
 * @return webserver_context containing file list and libmicrohttpd handle
 */
struct webserver_context * start_webserver(int localhost, unsigned short port, struct GNUNET_CONFIGURATION_Handle * gnunet_cfg);

/**
 * Stop libmicrohttpd server and deallocate context
 *
 * @param context webserver_context
 */
void stop_webserver(struct webserver_context * daemon);
