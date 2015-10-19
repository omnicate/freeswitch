ACLOCAL_AMFLAGS=-I build
EXTRA_DIST =
SUBDIRS = . test
AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS    += -I./src -I./src/include
AM_CPPFLAGS  = $(AM_CFLAGS)

lib_LTLIBRARIES	  = libks.la
libks_la_SOURCES  = src/ks.c src/ks_string.c src/ks_json.c src/ks_thread.c src/ks_mutex.c src/ks_config.c
libks_la_SOURCES += src/ks_log.c src/ks_socket.c src/ks_buffer.c src/ks_pool.c src/simclist.c
libks_la_SOURCES += src/ks_time.c src/ks_printf.c src/ks_hash.c src/ks_q.c src/ks_dso.c src/ks_dht.c
libks_la_CFLAGS	  = $(AM_CFLAGS)
libks_la_LDFLAGS  =  -version-info 0:1:0 -lncurses -lpthread -lm

library_includedir	= $(prefix)/include
library_include_HEADERS = src/include/ks_config.h src/include/ks.h src/include/ks_threadmutex.h src/include/ks_json.h src/include/ks_buffer.h src/include/ks_pool.h src/include/simclist.h src/include/ks_time.h src/include/ks_q.h src/include/ks_socket.h src/include/ks_dso.h src/include/ks_dht.h

tests: libks.la
	$(MAKE) -C test tests




