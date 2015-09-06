LINE=--------------------------------------------------------------------------------
PWD=$(shell pwd)
INCS=-I$(PWD)/src/include
DEBUG=-g -ggdb
BASE_FLAGS=$(INCS) $(DEBUG) -I$(LIBEDIT_DIR)/src/ -fPIC -g -ggdb
PICKY=-O2
CFLAGS=$(BASE_FLAGS) $(PICKY)
CXXFLAGS=$(BASE_FLAGS)
MYLIB=libks.a
LIBS=-lncurses -lks -lpthread -lm
LDFLAGS=-L.
OBJS=src/ks.o src/ks_threadmutex.o src/ks_config.o src/ks_json.o src/ks_buffer.o src/ks_mpool.o src/table.o src/simclist.o
SRC=src/ks.c src/ks_json.c src/ks_threadmutex.c src/ks_config.c src/ks_json.c src/ks_buffer.c src/ks_mpool.c src/table.c src/simclist.c
HEADERS=src/include/ks_config.h src/include/ks.h src/include/ks_threadmutex.h src/include/ks_json.h src/include/ks_buffer.h src/include/ks_mpool.h src/include/ks_mpool_loc.h src/include/table.h src/include/table_loc.h src/include/simclist.h
SOLINK=-shared -Xlinker -x

all: $(MYLIB) 

$(MYLIB): $(OBJS) $(HEADERS) $(SRC)
	ar rcs $(MYLIB) $(OBJS)
	ranlib $(MYLIB)

%.o: %.c $(HEADERS)
	$(CC) $(CC_CFLAGS) $(CFLAGS) $(CXFLAGS) -c $< -o $@

test-clean:
	rm -f test/testpools/testpools test/testtable/testtable

clean: test-clean
	rm -f *.o src/*.o libks.a *~ src/*~ src/include/*~

test/testpools/testpools: $(MYLIB) test/testpools/testpools.c
	$(CC) $(CXFLAGS) test/testpools/testpools.c -Isrc/include/ libks.a -o $@

test/testtable/testtable: $(MYLIB) test/testtable/testtable.c
	$(CC) $(CXFLAGS) test/testtable/testtable.c -Isrc/include/ libks.a -o $@

test-pools: test/testpools/testpools
	@echo Testing memory pool
	@echo $(LINE)
	test/testpools/testpools && echo ok

test-table: test/testtable/testtable
	@echo Testing hash
	@echo $(LINE)
	test/testtable/testtable && echo ok

test-all: test-pools test-table



