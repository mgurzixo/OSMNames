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

static unsigned int nbStreets;
static int fdData;
static sIndex* streets;

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

ZKSNUM findHouse(sIndex* myStreets, int nbStreets, OSMID streetId,
                 const char* houseNum) {
  sIndex* pIndex = findStreet(myStreets, nbStreets, streetId);
  int nbEntries = 0;
  uint64_t offset;
  ZKPLUS entries[MAX_HOUSES_IN_STREET];
  ZKSNUM zkn = INVALID_ZK;
  H16 h16 = myHash(houseNum);
  size_t count;
  int i;

  if (!pIndex) return INVALID_ZK;

  printf("[findHouse] h16:0x%04x\n", h16);

  nbEntries = decodeNbEntries(pIndex);
  offset = decodeOffset(pIndex);

  printf("[findHouse] offset:%ld nbEntries:%d\n", offset, nbEntries);

  if ((lseek(fdData, offset, SEEK_SET)) < 0) GOTO_ERROR;
  count = nbEntries * sizeof(ZKPLUS);
  if ((read(fdData, entries, count)) != count) GOTO_ERROR;

  // for (i = nbEntries - 1; i >= 0; i--) {
  for (i = 0; i < nbEntries; i++) {
    H16 h = decodeHash(entries[i]);
    if (h == h16) {
      printf("[findHouse] i:%d h:0x%04x zkPlus:0x%08lx\n", i, h, entries[i]);
      zkn = decodeZkn(entries[i]);
      break;
    }
  }
  return zkn;
error:
  fprintf(stderr, "Error findHouse line:%d errno:%d\n", __error_line__, errno);
  return INVALID_ZK;
}

int initHouse(const char* baseName, sIndex** hstreets) {
  char str[STRSIZ];
  struct stat statBuf;
  int fdIndex;
  sprintf(str, "%sIndex", baseName);
  if (stat(str, &statBuf) < 0) return -1;
  nbStreets = statBuf.st_size / sizeof(sIndex);
  printf("[myInit] nbStreets from houseNums:%d\n", nbStreets);
  if ((fdIndex = open(str, O_RDONLY)) < 0) return -1;
  streets =
      (sIndex*)mmap(NULL, statBuf.st_size, PROT_READ, MAP_SHARED, fdIndex, 0);

  if (streets == MAP_FAILED) {
    printf("Mapping Failed\n");
    return -1;
  }
  sprintf(str, "%sData", baseName);
  if ((fdData = open(str, O_RDONLY)) < 0) return -1;
  *hstreets = streets;
  return nbStreets;
}
