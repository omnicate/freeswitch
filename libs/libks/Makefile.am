ACLOCAL_AMFLAGS=-I build
EXTRA_DIST =
SUBDIRS = 
AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS    += -I./src -I./src/include
AM_CPPFLAGS  = $(AM_CFLAGS)

lib_LTLIBRARIES	  = libks.la
libks_la_SOURCES  = src/ks.c src/ks_string.c src/ks_json.c src/ks_thread.c src/ks_mutex.c src/ks_config.c
libks_la_SOURCES += src/ks_socket.c src/ks_buffer.c src/ks_mpool.c src/table.c src/simclist.c
libks_la_CFLAGS	  = $(AM_CFLAGS)
libks_la_LDFLAGS  =  -version-info 0:1:0 -lncurses -lpthread -lm

library_includedir	= $(prefix)/include
library_include_HEADERS = src/include/ks_config.h src/include/ks.h src/include/ks_threadmutex.h src/include/ks_json.h src/include/ks_buffer.h src/include/ks_mpool.h src/include/ks_mpool_loc.h src/include/table.h src/include/table_loc.h src/include/simclist.h

noinst_PROGRAMS = testpools testtable

testpools_SOURCES = test/testpools/testpools.c
testpools_LDADD = libks.la

testtable_SOURCES = test/testtable/testtable.c
testtable_LDADD = libks.la

test-pools: testpools
	@echo Testing memory pool
	@echo $(LINE)
	./testpools && echo ok

test-table: testtable
	@echo Testing hash
	@echo $(LINE)
	./testtable && echo ok

test-all: test-pools test-table



