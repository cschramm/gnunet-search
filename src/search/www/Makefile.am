SUBDIRS = .

pkglocalstatedir = $(localstatedir)/$(PACKAGE)

bin_PROGRAMS = index.cgi

index_cgi_SOURCES = index.c
index_cgi_LDADD = \
  -lz

dist_pkglocalstate_DATA = bootstrap.min.css logo.png style.css template.html
