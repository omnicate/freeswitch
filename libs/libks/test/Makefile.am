AUTOMAKE_OPTIONS = foreign

TESTS =
check_PROGRAMS = 


TESTS += testtable
check_PROGRAMS += testtable

testtable_SOURCES = testtable.c tap.c
testtable_CFLAGS = -I $(abs_top_srcdir)/src/include/ -I$(abs_top_srcdir)/test/
testtable_LDADD = $(abs_top_srcdir)/libks.la

TESTS += testpools
check_PROGRAMS += testpools

testpools_SOURCES = testpools.c
testpools_CFLAGS = -I $(abs_top_srcdir)/src/include/ -I$(abs_top_srcdir)/test/
testpools_LDADD = $(abs_top_srcdir)/libks.la

TESTS += testthreadmutex
check_PROGRAMS += testthreadmutex

testthreadmutex_SOURCES = testthreadmutex.c tap.c
testthreadmutex_CFLAGS = -I $(abs_top_srcdir)/src/include/ -I$(abs_top_srcdir)/test/ -g -ggdb -O0
testthreadmutex_LDADD = $(abs_top_srcdir)/libks.la


TESTS += testtime
check_PROGRAMS += testtime

testtime_SOURCES = testtime.c tap.c
testtime_CFLAGS = -I $(abs_top_srcdir)/src/include/ -I$(abs_top_srcdir)/test/ -g -ggdb -O0
testtime_LDADD = $(abs_top_srcdir)/libks.la
