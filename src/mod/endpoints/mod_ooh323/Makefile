BASE=../../../..

LOCAL_OBJS=h323_glue.o


#Linux
#LOCAL_LDFLAGS= -looh323c

#Mac
H323=ooh323c
LOCAL_CFLAGS=-I$(H323)/src -I$(H323)/src/h323_v6 -I$(H323)/src/h323 -I.
#LOCAL_LDFLAGS=-L$(H323)/lib -looh323c
LOCAL_LDFLAGS=-static $(H323)/lib/libooh323c.a

include $(BASE)/build/modmake.rules
