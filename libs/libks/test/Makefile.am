AM_CFLAGS += -I$(abs_top_srcdir)/src/include -g -ggdb -O0
TEST_LDADD = $(abs_top_srcdir)/libks.la
check_PROGRAMS = 

check_PROGRAMS += testtable
testtable_SOURCES = testtable.c tap.c
testtable_CFLAGS = $(AM_CFLAGS)
testtable_LDADD = $(TEST_LDADD)

check_PROGRAMS += testpools
testpools_SOURCES = testpools.c
testpools_CFLAGS = $(AM_CFLAGS)
testpools_LDADD = $(TEST_LDADD)

check_PROGRAMS += testthreadmutex
testthreadmutex_SOURCES = testthreadmutex.c tap.c
testthreadmutex_CFLAGS = $(AM_CFLAGS)
testthreadmutex_LDADD = $(TEST_LDADD)

check_PROGRAMS += testtime
testtime_SOURCES = testtime.c tap.c
testtime_CFLAGS = $(AM_CFLAGS)
testtime_LDADD = $(TEST_LDADD)

TESTS=$(check_PROGRAMS)

tests: $(check_PROGRAMS)
