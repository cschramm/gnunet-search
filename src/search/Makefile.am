SUBDIRS = .

INCLUDES = \
  -I$(top_srcdir)/src/include \
  -I$(top_srcdir) \
  @GNUNET_CFLAGS@ 

lib_LTLIBRARIES = libgnunetext.la

libgnunetext_la_SOURCES = \
  ext_api.c 
libgnunetext_la_LIBADD = \
  -lgnunetutil
libgnunetext_la_LDFLAGS = \
  $(GNUNET_LIBS)  $(WINFLAGS) \
  -version-info 0:0:0


bin_PROGRAMS = gnunet-service-ext gnunet-ext

gnunet_service_ext_SOURCES = \
  gnunet-service-ext.c
gnunet_service_ext_LDADD = \
  -lgnunetutil -lgnunetcore\
  $(INTLLIBS) 
gnunet_service_ext_LDFLAGS = \
  $(GNUNET_LIBS)  $(WINFLAGS) -export-dynamic 

gnunet_ext_SOURCES = \
  gnunet-ext.c
gnunet_ext_LDADD = \
  -lgnunetutil \
  $(INTLLIBS) 
gnunet_ext_LDFLAGS = \
 $(GNUNET_LIBS) $(WINFLAGS) -export-dynamic 


check_PROGRAMS = \
 test_ext_api 

TESTS = $(check_PROGRAMS)

test_ext_api_SOURCES = \
 test_ext_api.c
test_ext_api_LDADD = \
  $(top_builddir)/src/ext/libgnunetext.la \
  -lgnunetutil
test_ext_api_LDFLAGS = \
 $(GNUNET_LIBS)  $(WINFLAGS) -export-dynamic 