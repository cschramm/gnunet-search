SUBDIRS = www .

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


bin_PROGRAMS = gnunet-service-search gnunet-search

gnunet_service_search_SOURCES = \
  gnunet-service-search.c \
  gnunet-service-search-web.c
gnunet_service_search_LDADD = \
  -lgnunetutil -lgnunetcore -lgnunetdht\
  -lmicrohttpd \
  $(INTLLIBS) 
gnunet_service_search_LDFLAGS = \
  $(GNUNET_LIBS)  $(WINFLAGS) -export-dynamic 

gnunet_search_SOURCES = \
  gnunet-search.c
gnunet_search_LDADD = \
  -lgnunetutil \
  $(INTLLIBS) 
gnunet_search_LDFLAGS = \
 $(GNUNET_LIBS) $(WINFLAGS) -export-dynamic 


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
