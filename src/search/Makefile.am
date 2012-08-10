SUBDIRS = .

INCLUDES = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir) \
  @GNUNET_CFLAGS@ 

AM_CFLAGS = \
  -DDATADIR=\""$(pkgdatadir)"\"

lib_LTLIBRARIES = libgnunetsearch.la

libgnunetsearch_la_SOURCES = \
  search_api.c 
libgnunetsearch_la_LIBADD = \
  -lgnunetutil
libgnunetsearch_la_LDFLAGS = \
  $(GNUNET_LIBS)  $(WINFLAGS) \
  -version-info 0:0:0

bin_PROGRAMS = gnunet-service-search gnunet-search gnunet-search-web

gnunet_service_search_SOURCES = \
  service/gnunet-service-search.c \
  service/url-processor/url-processor.c \
  service/util/service-util.c \
  service/client-communication/client-communication.c \
  communication/communication.c \
  service/dht/dht.c \
  service/flooding/flooding.c \
  service/storage/storage.c \
  service/normalization/normalization.c \
  service/globals/globals.c
gnunet_service_search_LDADD = \
  -lgnunetutil -lgnunetcore -lgnunetdht\
  -lcrawl -lcurl -lcollections \
  $(INTLLIBS) 
gnunet_service_search_LDFLAGS = \
  $(GNUNET_LIBS)  $(WINFLAGS) -export-dynamic 

gnunet_search_web_SOURCES = \
  web-client/gnunet-search-web.c
gnunet_search_web_LDADD = \
  -lmicrohttpd \
  $(INTLLIBS)
gnunet_search_web_LDFLAGS = \
  $(GNUNET_LIBS) $(WINFLAGS) -export-dynamic

gnunet_search_SOURCES = \
  cli-client/gnunet-search.c \
  cli-client/util/client-util.c \
  client/server-communication/server-communication.c \
  communication/communication.c
gnunet_search_LDADD = \
  -lgnunetutil \
  -lcollections \
  $(INTLLIBS) 
gnunet_search_LDFLAGS = \
 $(GNUNET_LIBS) $(WINFLAGS) -export-dynamic 

dist_pkgdata_DATA = web-client/www/*

check_PROGRAMS = \
 test_search_api 

TESTS = $(check_PROGRAMS)

test_search_api_SOURCES = \
 test_search_api.c
test_search_api_LDADD = \
  $(top_builddir)/src/search/libgnunetsearch.la \
  -lgnunetutil
test_search_api_LDFLAGS = \
 $(GNUNET_LIBS)  $(WINFLAGS) -export-dynamic 
