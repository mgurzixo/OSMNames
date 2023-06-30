using namespace std;

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstdint>
#include <string>

#include "zkDefs.h"

typedef char UTF8;           // for code units of UTF-8 strings
typedef unsigned char BYTE;  // for binary data
#define BEOF ((BYTE)-1)

#define STREETID 4527645
#define HOUSENUMBER "25 bis"

#define GOTO_ERROR             \
  ({                           \
    __error_line__ = __LINE__; \
    __error_file__ = __FILE__; \
    goto error;                \
  })

// Alloc / Free blocs
inline void* MALLOC(const size_t x) {
  void* p;
  if (!(p = malloc(x))) {
    fprintf(stderr, "Can't malloc %lu bytes\n", (unsigned long)(x));
    exit(-1);
  }
  return (p);
}

inline void FREE(void* x) {
  if (x) free(x);
}

#define RAZ(x) (FREE(x), x = 0)

inline BYTE* XMCPY(const BYTE* str) {
  size_t len = strlen((char*)str);
  BYTE* pc = (BYTE*)MALLOC(len + 1);
  strcpy((char*)pc, (char*)str);
  return pc;
}

#define BYTE7 0xFF00000000000000
#define BYTE6 0xFF000000000000
#define BYTE76 0xFFFF000000000000

#define STRSIZ 1024
#define DEBUG true

extern FILE* fpLog;

#define LOG(...) \
  if (DEBUG) fprintf(fpLog, __VA_ARGS__), fflush(fpLog)

// Maximal number of streets in world
#define MAX_NB_STREETS 1000000000

// Maximal number of housenumbers in a street
#define MAX_HOUSES_IN_STREET 10000

typedef uint16_t H16;
typedef int64_t OSMID;
typedef int64_t ZKSNUM;  // Zonkod as a number, or INVALID_ZK
#define INVALID_ZK (-1)

/*
The 14 digits of a zk are coded as an uint64_t.
But 2**48 = 2.81*10**14 so the 2 high bytes are always 0
We code into them the hash of the housenumber
This makes a housenumber entry.
All the housenumbers of a street are stored sequentially, starting at an offset
*/

// The housenumber entry in the data file on disk: 2**3 B
typedef uint64_t ZKPLUS;  // B7-6:hash B5-0:zk

/*
We expect that there is less than 2**(56-3) (=9*10**15) housenumbers
We expect that the highest street osm_id is less than 2**56
So we code the number of housenumbers of a street in the B7 of those values
This makes for the index entry of the street (16B, stored in memory)
*/
// The index entry
typedef uint64_t OFFSETPLUS;
typedef uint64_t STREETIDPLUS;

struct sHousenumber {
  OSMID osmId;
  OSMID streetId;
  char houseNumber[64];
  double lon;
  double lat;
};

// Index entry
struct sIndex {
  STREETIDPLUS streetIdPlus;  // B7: low byte of nb entries, B0-B7: streetId
  OFFSETPLUS offsetPlus;      // B7: high byte of nb entries, B0-B7 offset
};

#define decodeOffset(pIndex) (pIndex->offsetPlus & ~BYTE7)
#define decodeNbEntries(pIndex) \
  ((pIndex->offsetPlus >> (64 - 8 - 8)) | ((pIndex->streetIdPlus >> (64 - 8))))

#define decodeHash(entry) ((entry >> (64 - 16)) & 0xffff)
#define decodeZkn(entry) (entry & ~BYTE76)

// mergestreets
#define NB_GEOFIELDS 24
struct sStreet {
  BYTE* fields[NB_GEOFIELDS];
};

extern BYTE motlu[100 * 1024];
extern int maxFieldSize;
extern char getMot(FILE* fp);

extern H16 myHash(const char* str);  // utils

extern uint64_t llToZkn(double lon, double lat);
extern char* zknToZk(ZKSNUM zkn0, char* zk);

extern sIndex* findStreet(sIndex* myStreets, int nbStreets, OSMID streetId);
extern ZKSNUM findHouse(sIndex* myStreets, int nbStreets, OSMID streetId,
                        const char* houseNum);
extern int initHouse(const char* baseName, sIndex** hstreets);
