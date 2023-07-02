using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <regex.h>

#include <cstdint>
#include <string>

#include "gennumbers.h"
#include "zkDefs.h"

// TODO check MAX_NB_STREETS MAX_HOUSES_IN_STREET

static int __error_line__;
static const char* __error_file__;

FILE* fpLog = stderr;
int currentLine = 0;

// static char* pLine;
// static char line[STRSIZ];
// static char motlu[STRSIZ];

static unsigned int nbStreets;
static sIndex streets[MAX_NB_STREETS];

#define ERR_OK 0
#define ERR_EOF 1
#define ERR_BAD_LINE 2

int getHn(FILE* fp, sHousenumber* phn) {
  OSMID osmId;
  char tok;
  char* pc;
  // if (!fgets(line, sizeof(line), fp)) return ERR_EOF;
  // pLine = line;

  // osm_id
  tok = getMot(fp);
  if (tok == EOF) goto retEof;
  if (tok != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("osm_id:'%s'\n", motlu);
  osmId = strtoull((char*)motlu, NULL, 10);
  if (osmId < 0) {
    // Dont know why!!!
    osmId = -osmId;
  }
  phn->osmId = osmId;
  if (!phn->osmId) GOTO_ERROR;

  // street_id
  tok = getMot(fp);
  if (tok != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("street_id:'%s'\n", motlu);
  osmId = strtoull((char*)motlu, NULL, 10);
  if (osmId <= 0) {
    // LOG("[getHn] negative streetId:%ld Skipping\n", osmId);
    goto skipLine;
  }
  phn->streetId = osmId;
  if (!phn->streetId) GOTO_ERROR;

  // street
  tok = getMot(fp);
  if (tok != '\t') GOTO_ERROR;
  // LOG("street:'%s'\n", motlu);

  // housenumber
  tok = getMot(fp);
  if (tok != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("housenumber:'%s'\n", motlu);
  pc = trimwhitespace((char*)motlu);
  if (!*pc) GOTO_ERROR;
  strncpy(phn->houseNumber, pc, 64);

  // lon
  tok = getMot(fp);
  if (tok != '\t') GOTO_ERROR;
  // LOG("lon:'%s'\n", motlu);
  if (!*motlu) GOTO_ERROR;
  phn->lon = atof((char*)motlu);

  // lat
  tok = getMot(fp);
  // Last field. if tok==eof, make it \n
  // next getMot will return again EOF
  if (tok == EOF) tok = '\n';
  if (tok != '\n') GOTO_ERROR;
  // LOG("lat:'%s'\n", motlu);
  if (!*motlu) GOTO_ERROR;
  phn->lat = atof((char*)motlu);

  if (!phn->lon && !phn->lat) GOTO_ERROR;

  // LOG("----------\n");
  return ERR_OK;
error:
  LOG("Error getHn@%d line:%d\n", __error_line__, currentLine);
skipLine:
  // Skip line
  for (; (tok != '\n') && (tok != EOF);) {
    tok = getMot(fp);
  }
  // LOG("tok=0x%02x\n", tok);
  if (tok != EOF) return ERR_BAD_LINE;
retEof:
  return ERR_EOF;
}

ZKPLUS makeHouseEntry(sHousenumber* phn) {
  uint64_t zk;
  ZKPLUS zkPlus;
  uint16_t hash;

  // LOG("[makeHouseEntry] line:'%s'\n", line);
  hash = myHash(phn->houseNumber);

  zk = llToZkn(phn->lon, phn->lat);
  zkPlus = (zk & ~BYTE76) | ((uint64_t)hash << (64 - 16));

  if ((phn->streetId == STREETID) && !strcmp(phn->houseNumber, HOUSENUMBER)) {
    LOG("[makeHouseEntry] streetId:%ld #:'%s' hash:0x%04x zk:%ld "
        "zkPlus:0x%08lx\n",
        phn->streetId, phn->houseNumber, hash, zk, zkPlus);
  }
  return zkPlus;
}

void makeStreetEntry(unsigned int streetId, int nbEntries, ssize_t offset,
                     sIndex* pEntry) {
  // LOG("[makeStreetEntry]'\n");
  pEntry->streetIdPlus = (streetId & ~BYTE7) | (STREETIDPLUS)(nbEntries & 0xff)
                                                   << (64 - 8);
  pEntry->offsetPlus = (offset & ~BYTE7) | (STREETIDPLUS)(nbEntries & 0xff00)
                                               << (64 - 8 - 8);
}

// int processHn(int fdData, sHousenumber* pHn, ZKPLUS houses[], int nbEntries)
// {
//   int currentStreetId = -1;
//   ssize_t currentOffset;
//   int nbHouses = 0;
//   bool doStop = false;
//   int n;

//   if ((nbEntries != 0) && ((currentStreetId != pHn->streetId) || doStop)) {
//     if (findStreet(streets, nbStreets, currentStreetId) == NULL) {
//       // Create street entry only if street does not exist already
//       makeStreetEntry(currentStreetId, nbEntries, currentOffset,
//                       streets + nbStreets);
//       if (write(fdData, houses, nbEntries * sizeof(ZKPLUS)) < 0) GOTO_ERROR;
//       if (currentStreetId == STREETID) {
//         LOG("[doIt] streetId:%ld offset:%ld %d entries\n", pHn->streetId,
//             currentOffset, nbEntries);
//       }

//       ++nbStreets;
//       currentOffset += nbEntries * sizeof(ZKPLUS);
//       nbHouses += nbEntries;
//       if (!(nbStreets % 100000)) LOG("nbStreets:%d\n", nbStreets);
//     } else LOG("[processHn] street not found\n");

//     currentStreetId = pHn->streetId;
//     nbEntries = 0;
//   }
//   houses[nbEntries] = makeHouseEntry(pHn);
//   nbEntries++;
//   if (nbEntries >= MAX_HOUSES_IN_STREET) {
//     LOG("[doIt] too many houses. nbEntries:%d currentStreetId:%d\n",
//     nbEntries,
//         currentStreetId);
//     GOTO_ERROR;
//   }
//   return nbEntries;
// error:
//   LOG("Error doIt line:%d\n", __error_line__);
//   return nbEntries;
// }

#define MAX_NB_HN_IN_HN 100

// int makeTabHn(sHousenumber* pHn, char** tabHn) {
//   int nbHn = 0;
//   char* pe = pHn->houseNumber;
//   char* pch = mystrtok((char*)pe, HN_SEPARATORS);
//   if (pch && !*pch) return (0);
//   // LOG("[cFMHn] pe:%s pch:%s\n", pe, pch);
//   while (pch != NULL) {
//     bool found = false;
//     char* pt = trimwhitespace(pch);
//     if (!*pt) found = true;
//     else {
//       // LOG("[makeTabHn] nbHn:%d pt:'%s'\n", nbHn, pt);
//       for (int i = 0; i < nbHn; i++) {
//         if (!strcmp(tabHn[i], pt)) {
//           found = true;
//           break;
//         }
//       }
//     }
//     // LOG("++++++++\n");
//     // LOG("[makeTabHn] found:%d nbHn:%d pt:'%s'\n", found, nbHn, pt);
//     if (!found) tabHn[nbHn++] = (char*)XMCPY((BYTE*)pt);
//     pch = mystrtok(NULL, HN_SEPARATORS);
//   }
//   return nbHn;
// }

void doIt(FILE* fp, int fdData, int fdIndex) {
  regex_t regexComma;
  sHousenumber hn;
  ZKPLUS houses[MAX_HOUSES_IN_STREET];
  int nbEntries = 0;
  OSMID currentStreetId = -1;
  ssize_t currentOffset;
  int nbHouses = 0;
  bool doStop = false;
  int nbHn;
  char* tabHn[MAX_NB_HN_IN_HN];

  for (currentLine = 0;;) {
  again:
    currentLine++;
    switch (getHn(fp, &hn)) {
      case ERR_OK:
        break;
      case ERR_EOF:
        doStop = true;
        break;
      default:
        goto again;
    }

    // char logBuf[10000];
    // strcpy(logBuf, hn.houseNumber);

    if (currentStreetId == -1) currentStreetId = hn.streetId;

    // LOG("[doIt] nbEntries:%d currentStreetId:%ld hn.streetId:%ld\n",
    // nbEntries, currentStreetId, hn.streetId);
    if (((currentStreetId != hn.streetId) || doStop)) {
      if (nbEntries != 0) {
        // Flush entries
        if (findStreet(streets, nbStreets, currentStreetId) == NULL) {
          // LOG("[doIt] street found\n");
          // Create street entry only if street does not exist already
          makeStreetEntry(currentStreetId, nbEntries, currentOffset,
                          streets + nbStreets);
          if (write(fdData, houses, nbEntries * sizeof(ZKPLUS)) < 0) GOTO_ERROR;
          ++nbStreets;
          currentOffset += nbEntries * sizeof(ZKPLUS);
          nbHouses += nbEntries;
          if (!(nbStreets % 1000000)) LOG("nbStreets:%d\n", nbStreets);
        } else {
          //  LOG("[doIt] street not found\n");
        }
      }
      currentStreetId = hn.streetId;
      nbEntries = 0;
    }

    // nbHn = makeTabHn(&hn, tabHn);
    nbHn = makeTabHnFromStr(tabHn, hn.houseNumber);

    // if (nbHn > 1) LOG("[doIt] nbHn:%d Hn:'%s'\n", nbHn, logBuf);
    // LOG("[doIt] nbHn:%d Hn:'%s'\n", nbHn, logBuf);
    for (int i = 0; i < nbHn; i++) {
      nbHouses++;
      strcpy(hn.houseNumber, tabHn[i]);
      FREE(tabHn[i]);

      houses[nbEntries] = makeHouseEntry(&hn);
      nbEntries++;
      if (nbEntries >= MAX_HOUSES_IN_STREET) {
        LOG("[doIt] too many houses. nbEntries:%d currentStreetId:%ld\n",
            nbEntries, currentStreetId);
        GOTO_ERROR;
      }
    }
    if (doStop) break;
  }
  LOG("Nb lines:%d Nb streets:%d Nb houses:%d\n", currentLine, nbStreets,
      nbHouses);
  // LOG("Nb negative Ids:%d\n", nbNegativeIds);
  if (write(fdIndex, streets, nbStreets * sizeof(sIndex)) < 0) GOTO_ERROR;

  return;
error:
  LOG("Error doIt line:%d\n", __error_line__);
  return;
}

int main(int argc, char* argv[]) {
  FILE* fp;
  int fdIndex, fdData;
  char c;
  int i;
  char* baseName;
  char strIndex[STRSIZ];
  char strData[STRSIZ];
  if (argc < 2) {
    fprintf(stderr, "Usage gennumbers baseName [input_file]\n");
    exit(-1);
  }
  if (argc == 3) {
    if (!(fp = fopen(argv[2], "r"))) GOTO_ERROR;
    printf("Reading %s\n", argv[2]);
  } else fp = stdin;
  baseName = argv[1];
  sprintf(strIndex, "%sIndex.bin", baseName);
  sprintf(strData, "%sData.bin", baseName);
  printf("Generating %s %s\n", strIndex, strData);
  if ((fdIndex = open(strIndex, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
    GOTO_ERROR;
  if ((fdData = open(strData, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
    GOTO_ERROR;
  doIt(fp, fdData, fdIndex);
  return 0;

error:
  LOG("Error main (line:%d errno:%d)\n", __error_line__, errno);
  return (-1);
}