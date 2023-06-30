BASE=/home/mgouget/dev
ZKC=$(BASE)/zkc
ZKJS=$(BASE)/zkjs
JSMN=$(BASE)/jsmn

INCDIRS= -I. -I$(ZKC) -I$(ZKJS) -I$(JSMN)
# CFLAGS=-O3 -mcmodel=large -g
CFLAGS=-mcmodel=large -g
CPPFLAGS=$(CFLAGS)
CC=gcc
CPP=g++
PROGRAMS= gennumbers testhouse tinyfcgi housesfcgi mergestreets
.PHONY: all

ZKOBJS=$(ZKC)/zonKod.o $(ZKC)/zkUtils.o $(ZKJS)/zkBands.o utils.o
ZKINCS=gennumbers.h
OBJS=findhouse.o utils.o
TEST_COUNTRY=paris

### Rules ###
%.o : %.c
	$(CC) $(INCDIRS) -c $(CFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CPP) $(INCDIRS) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: $(PROGRAMS)

$(OBJS):$(ZKINCS)


mergestreets: mergestreets.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lm

gennumbers: gennumbers.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lm

testhouse: testhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lm

tinyfcgi: tinyfcgi.o
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lfcgi -lm

housesfcgi: housesfcgi.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) -o $@  $^ -lfcgi -lm

rtest:
	- killall -9 housesfcgi
	- kill `cat testPid`
	spawn-fcgi -P testPid -u mgouget -p 3033 ./housesfcgi

gentesthn:
	genhousenumbers $(TEST_COUNTRIES)

test: gennumbers housesfcgi
	echo Filtering test housenumbers...
	# remove first line (columns names)
	rm -f -r test.tsv
	tail -n +2 ../data/export/$(TEST_COUNTRY)_housenumbers.tsv >>$(TEST_COUNTRY).tsv
	echo Sorting housenumbers and generating files...
	# sort -u -n -t \"\$(printf \"\t\")\" -k 1,1 <$(TEST_COUNTRY).tsv >test0
	sort -u -n -t "`cat ./tab`" -k 1,1 <$(TEST_COUNTRY).tsv >test0
	echo Sorting unique done
	sort -n -t "`cat ./tab`" -k 2,2 -k 4,4 <test0 >test1
	echo Sorting done
	./gennumbers test <test1
	echo done.
	make rtest


clean :
	rm -f *.o gennumbers
