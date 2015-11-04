ACLOCAL_AMFLAGS=-I build
EXTRA_DIST =
SUBDIRS = . test
AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES	  = libblade.la
libblade_la_SOURCES  = src/blade.c src/blade_stack.c src/blade_peer.c bpcp.c
libblade_la_CFLAGS	  = $(AM_CFLAGS) $(AM_CPPFLAGS)
libblade_la_LDFLAGS  =  -version-info 0:1:0 -lncurses -lpthread -lm $(AM_LDFLAGS)

library_includedir	= $(prefix)/include
library_include_HEADERS = src/include/blade.h

tests: libblade.la
	$(MAKE) -C test tests




