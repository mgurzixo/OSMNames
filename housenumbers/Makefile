BASE=/home/mgouget/dev
TEST_BASENAME=test

# TEST_COUNTRIES=paris
TEST_COUNTRIES=portugal-latest
# TEST_COUNTRIES=luxembourg-latest

WORLD_BASENAME=world

PROFILE_FLAGS=
# PROFILE_FLAGS=-pg
# PROFILE_FLAGS=-pg -fprofile-arcs -ftest-coverage

LDFLAGS=
# LDFLAGS=--no-relax 

# CFLAGS=-O3 $(PROFILE_FLAGS) -mcmodel=large -g
# CFLAGS=-O0 $(PROFILE_FLAGS)  -g
# CFLAGS=-O0 $(PROFILE_FLAGS)  -g
CFLAGS=-O3 -mcmodel=large -g

CPPFLAGS=$(CFLAGS)
CC=gcc
CPP=g++
ZKC=$(BASE)/zkc
ZKJS=$(BASE)/zkjs
JSMN=$(BASE)/jsmn
INCDIRS= -I. -I$(ZKC) -I$(ZKJS) -I$(JSMN)
LIBS=-ltcmalloc -lm

BINARIES= gennumbers testhouse tinyfcgi fcgihn mergestreets
ZKOBJS=$(ZKC)/zonKod.o $(ZKC)/zkUtils.o $(ZKJS)/zkBands.o
ZKINCS=gennumbers.h
OBJS=utils.o

### Rules ###
%.o : %.c
	$(CC) $(INCDIRS) -c $(CFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CPP) $(INCDIRS) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

.PHONY: all

all: $(BINARIES)

$(OBJS):$(ZKINCS)

# Create worldIndex worldData 
makeworld: gennumbers mergestreets 
	time sh -x ./genhousenumbers.sh $(WORLD_BASENAME) `../comcountries ../pbf_file_url`

maketest: gennumbers mergestreets 
	time ./genhousenumbers.sh $(TEST_BASENAME) $(TEST_COUNTRIES)

spawnworld: fcgihn
	- kill `cat fcgi$(WORLD_BASENAME).pid`
	sleep 1
	spawn-fcgi -u mgouget -p 3031 -P fcgi$(WORLD_BASENAME).pid -- ./fcgihn -b$(WORLD_BASENAME)

spawntest: fcgihn
	- kill `cat fcgi$(TEST_BASENAME).pid`
	sleep 1
	spawn-fcgi -u mgouget -p 3033 -P fcgi$(TEST_BASENAME).pid -- ./fcgihn -b$(TEST_BASENAME)

test: maketest spawntest

mergestreets: mergestreets.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS)  $(LDFLAGS) -o $@ $^ $(LIBS)

gennumbers: gennumbers.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ $(LIBS)

fcgihn: fcgihn.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lfcgi $(LIBS)

testhouse: testhouse.o findhouse.o $(ZKOBJS) ${OBJS}
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ $(LIBS)

tinyfcgi: tinyfcgi.o
	$(CPP) $(INCDIRS) $(CFLAGS) $(LDFLAGS) -o $@  $^ -lfcgi $(LIBS)



rtest:
	- killall -9 fcgihn
	- kill `cat testPid`
	spawn-fcgi -P testPid -u mgouget -p 3033 ./fcgihn

gentesthn:
	genhousenumbers $(TEST_COUNTRIES)

test: gennumbers fcgihn
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
