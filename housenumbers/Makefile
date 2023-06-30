BASE=/home/mgouget/dev
ZKC=$(BASE)/zkc
ZKJS=$(BASE)/zkjs
JSMN=$(BASE)/jsmn

INCDIRS= -I. -I$(ZKC) -I$(ZKJS) -I$(JSMN)

PROFILE_FLAGS=
# PROFILE_FLAGS=-pg
# PROFILE_FLAGS=-pg -fprofile-arcs -ftest-coverage

LDFLAGS=
# LDFLAGS=--no-relax 

CFLAGS=-O3 $(PROFILE_FLAGS) -mcmodel=large -g
# CFLAGS=-O0 $(PROFILE_FLAGS)  -g
# CFLAGS=-mcmodel=large -g

CPPFLAGS=$(CFLAGS)
CC=gcc
CPP=g++
BINARIES= gennumbers testhouse tinyfcgi housesfcgi mergestreets
.PHONY: all
LIBS=-ltcmalloc -lm

ZKOBJS=$(ZKC)/zonKod.o $(ZKC)/zkUtils.o $(ZKJS)/zkBands.o
ZKINCS=gennumbers.h
OBJS=utils.o
TEST_COUNTRY=paris

### Rules ###
%.o : %.c
	$(CC) $(INCDIRS) -c $(CFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CPP) $(INCDIRS) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

all: $(BINARIES)

$(OBJS):$(ZKINCS)


mergestreets: mergestreets.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS)  $(LDFLAGS) -o $@  $^ $(LIBS)

gennumbers: gennumbers.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ $(LIBS)

testhouse: testhouse.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ $(LIBS)

tinyfcgi: tinyfcgi.o
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lfcgi $(LIBS)

housesfcgi: housesfcgi.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lfcgi $(LIBS)

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
	rm -f *.o $(BINARIES)
