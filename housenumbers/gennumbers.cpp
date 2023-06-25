using namespace std;

#include "gennumbers.h"

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cstdint>
#include <string>

#include "zkDefs.h"

// TODO check MAX_NB_STREETS MAX_HOUSES_IN_STREET

static int __error_line__;
static const char* __error_file__;

static char* pLine;
static char line[STRSIZ];
static char motlu[STRSIZ];

static unsigned int nbStreets;
static sIndex streets[MAX_NB_STREETS];

#define ERR_OK 0
#define ERR_EOF 1
#define ERR_BAD_LINE 2

char getMot(FILE* fp) {
  char* pca;
  char c;
  for (pca = motlu; *pLine;) {
    c = *pLine++;
    switch (c) {
      case '\t':
      case '\n':
        *pca = 0;
        // LOG("[getMot] motlu:'%s'\n", motlu);
        return (c);
      default:
        *pca++ = c;
    }
  }
  *pca = 0;
  fprintf(stderr, "Error getMot motlu:'%s' str:'%s'\n", motlu, line);
  return 0;
}

int getHn(FILE* fp, sHousenumber* phn) {
  if (!fgets(line, sizeof(line), fp)) return ERR_EOF;
  pLine = line;

  // osm_id
  if (getMot(fp) != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("osm_id:'%s'\n", motlu);
  phn->osmId = strtoull(motlu, NULL, 10);
  if (!phn->osmId) GOTO_ERROR;

  // street_id
  if (getMot(fp) != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("street_id:'%s'\n", motlu);
  phn->streetId = strtoull(motlu, NULL, 10);
  if (!phn->streetId) GOTO_ERROR;

  // street
  if (getMot(fp) != '\t') GOTO_ERROR;
  // LOG("street:'%s'\n", motlu);

  // housenumber
  if (getMot(fp) != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("housenumber:'%s'\n", motlu);
  strncpy(phn->houseNumber, motlu, 64);

  // lon
  if (getMot(fp) != '\t') GOTO_ERROR;
  // LOG("lon:'%s'\n", motlu);
  if (!*motlu) GOTO_ERROR;
  phn->lon = atof(motlu);

  // lat
  if (getMot(fp) != '\n') GOTO_ERROR;
  // LOG("lat:'%s'\n", motlu);
  if (!*motlu) GOTO_ERROR;
  phn->lat = atof(motlu);

  if (!phn->lon && !phn->lat) GOTO_ERROR;

  // LOG("----------\n");
  return ERR_OK;
error:
  fprintf(stderr, "Error getHn line:%d str:'%s'\n", __error_line__, line);
  return ERR_BAD_LINE;
}

uint64_t toZk(double lon, double lat) {
  char zk[64];
  zkLatLon zkLl;
  uint64_t res;
  zkNadsToZk(zkDegToNads(lat), zkDegToNads(lon), &zkLl);
  res = strtoull(zkLl.zk, NULL, 10);
  return res;
}

ZKPLUS makeHouseEntry(sHousenumber* phn) {
  uint64_t zk;
  ZKPLUS zkPlus;
  uint16_t hash;

  // printf("[makeHouseEntry] line:'%s'\n", line);
  hash = myHash(phn->houseNumber);

  zk = toZk(phn->lon, phn->lat);
  zkPlus = (zk & ~BYTE76) | ((uint64_t)hash << (64 - 16));

  if ((phn->streetId == STREETID) && !strcmp(phn->houseNumber, HOUSENUMBER)) {
    printf(
        "[makeHouseEntry] streetId:%ld #:'%s' hash:0x%04x zk:%ld "
        "zkPlus:0x%08lx\n",
        phn->streetId, phn->houseNumber, hash, zk, zkPlus);
  }
  return zkPlus;
}

void makeStreetEntry(unsigned int streetId, int nbEntries, ssize_t offset,
                     sIndex* pEntry) {
  // printf("[makeStreetEntry]'\n");
  pEntry->streetIdPlus = (streetId & ~BYTE7) | (STREETIDPLUS)(nbEntries & 0xff)
                                                   << (64 - 8);
  pEntry->offsetPlus = (offset & ~BYTE7) | (STREETIDPLUS)(nbEntries & 0xff00)
                                               << (64 - 8 - 8);
}

void doIt(FILE* fp, int fdData, int fdIndex) {
  sHousenumber hn;
  ZKPLUS houses[MAX_HOUSES_IN_STREET];
  int nbEntries = 0;
  int currentStreetId = -1;
  ssize_t currentOffset;
  int nbHouses = 0;
  bool doStop = false;
  int n;

  for (;;) {
  again:
    switch (getHn(fp, &hn)) {
      case ERR_OK:
        break;
      case ERR_EOF:
        doStop = true;
        break;
      default:
        goto again;
    }
    if (currentStreetId == -1) currentStreetId = hn.streetId;

    if ((nbEntries != 0) && ((currentStreetId != hn.streetId) || doStop)) {
      if (findStreet(streets, nbStreets, currentStreetId) == NULL) {
        // Create street entry only if street does not exist already
        makeStreetEntry(currentStreetId, nbEntries, currentOffset,
                        streets + nbStreets);
        if (write(fdData, houses, nbEntries * sizeof(ZKPLUS)) < 0) GOTO_ERROR;
        if (currentStreetId == STREETID) {
          printf("[doIt] streetId:%ld offset:%ld %d entries\n", hn.streetId,
                 currentOffset, nbEntries);
        }

        ++nbStreets;
        currentOffset += nbEntries * sizeof(ZKPLUS);
        nbHouses += nbEntries;
        if (!(nbStreets % 100000)) LOG("nbStreets:%d\n", nbStreets);
      }

      currentStreetId = hn.streetId;
      nbEntries = 0;
    }

    if (doStop) break;

    houses[nbEntries] = makeHouseEntry(&hn);
    nbEntries++;
    if (nbEntries >= MAX_HOUSES_IN_STREET) {
      printf("[doIt] too many houses. nbEntries:%d currentStreetId:%d\n",
             nbEntries, currentStreetId);
      GOTO_ERROR;
    }
  }
  printf("nb streets:%d nb houses:%d\n", nbStreets, nbHouses);
  if (write(fdIndex, streets, nbStreets * sizeof(sIndex)) < 0) GOTO_ERROR;
  return;
error:
  fprintf(stderr, "Error doIt line:%d\n", __error_line__);
  return;
}

int main(int argc, char* argv[]) {
  FILE* fp;
  int fdIndex, fdData;
  char c;
  int i;
  if (argc == 2) {
    if (!(fp = fopen(argv[1], "r"))) GOTO_ERROR;
  } else fp = stdin;
  // if (!(fp = fopen("bid.tsv", "r")))
  //   GOTO_ERROR;
  if ((fdIndex =
           open("housenumbersIndex", O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
    GOTO_ERROR;
  if ((fdData = open("housenumbersData", O_WRONLY | O_CREAT | O_TRUNC, 0644)) <
      0)
    GOTO_ERROR;
  doIt(fp, fdData, fdIndex);
  return 0;

error:
  fprintf(stderr, "Error main (line:%d errno:%d)\n", __error_line__, errno);
  return (-1);
}