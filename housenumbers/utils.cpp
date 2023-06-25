using namespace std;

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <string>

#include "gennumbers.h"
#include "zkDefs.h"

static int __error_line__;
static const char* __error_file__;

H16 myHash(const char* str) {
  uint32_t hash = 5381;
  const char* pcd;
  for (pcd = str; *pcd; pcd++) {
    hash = (hash * 33) ^ *pcd;
  }
  return (hash & 0xffff);
}

sIndex* findStreet(sIndex* myStreets, int nbStreets, OSMID streetId) {
  int low = 0;
  int high = nbStreets - 1;
  OSMID si;
  // Repeat until the pointers low and high meet each other
  while (low <= high) {
    int mid = low + (high - low) / 2;

    // Extract streetId (B0-B6)
    si = myStreets[mid].streetIdPlus & ~BYTE7;
    if (si == streetId) return myStreets + mid;

    if (si < streetId) low = mid + 1;

    else high = mid - 1;
  }

  return NULL;
}

uint64_t llToZkn(double lon, double lat) {
  char zk[64];
  zkLatLon zkLl;
  uint64_t res;
  zkNadsToZk(zkDegToNads(lat), zkDegToNads(lon), &zkLl);
  res = strtoull(zkLl.zk, NULL, 10);
  return res;
}

// Convert a ZKSNUM or ZKPLUS to ZK
char* zknToZk(ZKSNUM zkn0, char* zk) {
  int i;
  char zk0[15];
  char* pc = zk0;
  ZKSNUM zkn = zkn0 & ~BYTE76;
  for (i = 0; i < 14; i++) {
    *pc++ = zkn % 10 + '0';
    zkn /= 10;
  }
  for (i = 0; i < 14; i++) {
    zk[i] = zk0[14 - i];
  }
  zk[14] = 0;
  return zk;
}
