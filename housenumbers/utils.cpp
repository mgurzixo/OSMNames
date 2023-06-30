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

BYTE motlu[100 * 1024];
int maxFieldSize = -1;

#define INCREMENT_FIELD_LENGTH \
  if (++fieldSize >= sizeof(motlu)) GOTO_ERROR

char getMot(FILE* fp) {
  BYTE* pca;
  BYTE c;
  int fieldSize = 0;
  for (pca = motlu;;) {
    if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
    c = getc(fp);
    switch (c) {
      case '\t':
      case '\n':
      case (BYTE)EOF:
        *pca = 0;
        // LOG("[getMot] c:'%c' motlu:'%s'\n", c, motlu);
        return (c);
      default:
        if (c < 0x80) {
          INCREMENT_FIELD_LENGTH;
          *pca++ = c;
        } else if (c < 0xc0) GOTO_ERROR;
        else if (c < 0xe0) {  // 2 byte UTF-8
          INCREMENT_FIELD_LENGTH;
          *pca++ = c;
          INCREMENT_FIELD_LENGTH;
          *pca++ = getc(fp);
        } else if (c < 0xf0) {  // 3 byte UTF-8
          INCREMENT_FIELD_LENGTH;
          *pca++ = c;
          INCREMENT_FIELD_LENGTH;
          *pca++ = getc(fp);
          INCREMENT_FIELD_LENGTH;
          *pca++ = getc(fp);
        } else if (c < 0xf8) {  // 4 byte UTF-8
          INCREMENT_FIELD_LENGTH;
          *pca++ = c;
          INCREMENT_FIELD_LENGTH;
          *pca++ = getc(fp);
          INCREMENT_FIELD_LENGTH;
          *pca++ = getc(fp);
          INCREMENT_FIELD_LENGTH;
          *pca++ = getc(fp);
        } else GOTO_ERROR;
    }
  }
  GOTO_ERROR;
error:
  *pca = 0;
  LOG("[getMot] ERROR@%d\n", __error_line__);
  return 0;
}

// sIndex* findStreet(sIndex* myStreets, int nbStreets, OSMID streetId) {
//   int low = 0;
//   int high = nbStreets - 1;
//   OSMID si;
//   // Repeat until the pointers low and high meet each other
//   while (low <= high) {
//     int mid = low + (high - low) / 2;

//     // Extract streetId (B0-B6)
//     si = myStreets[mid].streetIdPlus & ~BYTE7;
//     if (si == streetId) return myStreets + mid;

//     if (si < streetId) low = mid + 1;

//     else high = mid - 1;
//   }

//   return NULL;
// }

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
  *zk = 0;
  if (zkn0 < 0) {
    return (zk);
  }
  int i;
  char zk0[15];
  char* pc;
  ZKSNUM zkn = zkn0 & ~BYTE76;
  for (i = 0, pc = zk0; i < 14; i++) {
    *pc++ = zkn % 10 + '0';
    zkn /= 10;
  }
  *pc = 0;
  // LOG("[zknToZk] zkn0:%ld zk0:'%s'\n", zkn0, zk0);
  for (i = 0; i < 14; i++) {
    zk[i] = zk0[13 - i];
  }
  zk[14] = 0;
  // LOG("[zknToZk] zkn0:%ld zk:'%s'\n", zkn0, zk);
  return zk;
}
