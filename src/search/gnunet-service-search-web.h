struct local_file {
	char * name;
	char * type;
	struct local_file * next;
};

struct webserver_context {
	struct local_file * first_local_file;
	struct MHD_Daemon * daemon;
};

struct webserver_context * start_webserver(int localhost, unsigned short port);
void stop_webserver(struct webserver_context * daemon);