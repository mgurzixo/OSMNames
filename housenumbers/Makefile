BASE=/home/mgouget/dev
ZKC=$(BASE)/zkc
ZKJS=$(BASE)/zkjs
JSMN=$(BASE)/jsmn

INCDIRS= -I. -I$(ZKC) -I$(ZKJS) -I$(JSMN)
# CFLAGS=-O2
CFLAGS=-g
CPPFLAGS=$(CFLAGS)
CC=gcc
CPP=g++
PROGRAMS= gennumbers findhouse tinyfcgi housesfcgi
.PHONY: all

ZKOBJS=$(ZKC)/zonKod.o $(ZKC)/zkUtils.o $(ZKJS)/zkBands.o utils.o
ZKINCS=gennumbers.h
OBJS=gennumbers.o findhouse.o utils.o

### Rules ###
%.o : %.c
	$(CC) $(INCDIRS) -c $(CFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CPP) $(INCDIRS) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: $(PROGRAMS)

$(OBJS):$(ZKINCS)

gennumbers: gennumbers.o $(ZKOBJS)
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lm

findhouse: findhouse.o $(ZKOBJS)
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lm

tinyfcgi: tinyfcgi.o
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lfcgi -lm

housesfcgi: housesfcgi.o
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lfcgi -lm
	- killall -9 housesfcgi
	spawn-fcgi -u mgouget -p 5001 ./housesfcgi

clean :
	rm -f *.o gennumbers
