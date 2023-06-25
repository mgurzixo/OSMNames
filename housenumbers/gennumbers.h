using namespace std;

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cstdint>
#include <string>

#include "zkDefs.h"

#define STREETID 4527645
#define HOUSENUMBER "25 bis"

#define GOTO_ERROR             \
  ({                           \
    __error_line__ = __LINE__; \
    __error_file__ = __FILE__; \
    goto error;                \
  })

#define BYTE7 0xFF00000000000000
#define BYTE6 0xFF000000000000
#define BYTE76 0xFFFF000000000000

#define STRSIZ 1024
#define DEBUG true
#define LOG \
  if (DEBUG) printf

// Maximal number of streets in world
#define MAX_NB_STREETS 1000000000

// Maximal number of housenumbers in a street
#define MAX_HOUSES_IN_STREET 10000

typedef uint16_t H16;
typedef uint64_t OSMID;
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

extern H16 myHash(const char* str);
extern sIndex* findStreet(sIndex* myStreets, int nbStreets, OSMID streetId);
