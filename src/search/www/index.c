#include "ClearSilver.h"
#include "string.h"

typedef struct {
	char *url;
	char *name;
} GNUNET_SEARCH_RESULT;

int main(void) {
	CGI *cgi;
	int i;
	GNUNET_SEARCH_RESULT result;
	char buf[255];
	HDF *hdf;
	
	hdf_init(&hdf);
	cgi_init(&cgi, hdf);
	
	cgi_parse(cgi);
	
	GNUNET_SEARCH_RESULT results[] = {
		{"http://www.clearsilver.net/", "ClearSilver"},
		{"http://code.google.com/p/mongoose/", "mongoose"},
		{0, 0}
	};
	
	if (!strlen(hdf_get_value(hdf, "Query.q", "")))
		results [0].url = 0;
	
	for (i = 0; (result = results[i]).url; i++) {
		sprintf(buf, "results.%d.url", i);
		hdf_set_value(hdf, buf, result.url);
		sprintf(buf, "results.%d.name", i);
		hdf_set_value(hdf, buf, result.name);
	}
	
	NEOERR *err = cgi_display(cgi, "template.html");
	
	if (err != STATUS_OK) {
		STRING str;
		nerr_error_string(err, &str);
		cgi_error(cgi, "An error occured when parsing template.html: %s", str.buf);
		string_clear(&str);
		return -1;
	}
	
	nerr_ignore(&err);
	hdf_destroy(&hdf);
	
	return 0;
}
