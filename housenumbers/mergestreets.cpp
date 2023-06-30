using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

#include <cstdint>
// #include <string>

#include "gennumbers.h"
#include "zkDefs.h"

static int __error_line__;
static const char* __error_file__;

FILE* fpLog = stderr;

static unsigned int nbHouseStreets;
static sIndex* streets;

// static BYTE motlu[100 * 1024];

static int nbStreets = 0;

sStreet* newStreet() {
  sStreet* pStreet = (sStreet*)MALLOC(sizeof(sStreet));
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    pStreet->fields[i] = NULL;
    pStreet->osmId = 0;
  }
  return pStreet;
}

void razStreet(sStreet* pStreet) {
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    RAZ(pStreet->fields[i]);
  }
}

void freeStreet(sStreet* pStreet) {
  razStreet(pStreet);
  FREE(pStreet);
}

#define ERR_OK 0
#define ERR_EOF 1
#define ERR_BAD_LINE 2

// Field numbers (starts @ 0)
#define F_STREET_NAME 0
#define F_STREET_ALTERNATIVE_NAMES 1
#define F_STREET_OSM_TYPE 2
#define F_STREET_ID 3
#define F_STREET_CLASS 4
#define F_STREET_DISPLAY_NAME 16
#define F_STREET_HOUSENUMBERS 23

static int analyseStreet(sStreet* pStreet) {
  BYTE** hf = pStreet->fields;
  BYTE* pf;

  pf = hf[F_STREET_ID];
  if (!pf) GOTO_ERROR;
  if (!*pf) goto badStreet;
  pStreet->osmId = strtoull((char*)pf, NULL, 10);
  if (pStreet->osmId <= 0) goto badStreet;
  return ERR_OK;
badStreet:
  fprintf(stderr, "[analyseStreet] streetLine:%d invalid streetId:'%s'\n",
          nbStreets, pf);
  return (ERR_BAD_LINE);
error:
  fprintf(stderr, "Error analyseStreet@%d streetLine:%d\n", __error_line__,
          nbStreets);
  return ERR_BAD_LINE;
}

static int getStreet(FILE* fp, sStreet* pStreet) {
  static bool isEof = false;
  if (isEof) return (ERR_EOF);
  razStreet(pStreet);
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    const char sep = (i == NB_GEOFIELDS - 1) ? '\n' : '\t';
    const char c = getMot(fp);
    // LOG("[getStreet] c:0x%02x sep:0x%02x motlu:'%s'\n", c, sep, motlu);
    if (c != sep) {
      if (c == EOF) {
        isEof = true;
      }
      if ((c == '\n') || (c == EOF)) {
        // Short line, missing end fields
        pStreet->fields[i] = XMCPY(motlu);
        break;
      }
      LOG("[getStreet] error i=%d c:0x%02x wanted:0x%02x motlu:'%s'\n", i, c,
          sep, motlu);
      GOTO_ERROR;
    } else pStreet->fields[i] = XMCPY(motlu);
  }
  return analyseStreet(pStreet);
error:
  fprintf(stderr, "Error getStreet@%d nbStreets:%d\n", __error_line__,
          nbStreets);
  return ERR_BAD_LINE;
}

// TODO Check overflow
void makeHnTab(sStreet* pStreet, char** tabHn, int* pNbHn) {
  BYTE* ps = pStreet->fields[F_STREET_HOUSENUMBERS];
  if (!*ps) return;
  *pNbHn = 0;
  char* pch = strtok((char*)ps, ",");
  while (pch != NULL) {
    bool found = false;
    char* pt = trimwhitespace(pch);
    if (!pt) found = true;
    else {
      // LOG("[mHn]   To: n:%d '%s'\n", *pNbHn, pch);
      for (int i = 0; i < *pNbHn; i++) {
        // LOG("[mHn] nbHn:%d i:'%d'\n", *pNbHn, i);
        if (!strcmp(tabHn[i], pt)) {
          // LOG("[removeDuplicateHn] DUP:'%s'\n", pch);
          found = true;
          break;
        }
      }
    }
    if (!found) tabHn[*pNbHn++] = pt;
    pch = strtok(NULL, ",");
  }
}

void putHnTab(sStreet* pStreet, char** tabHn, int nbHn) {
  char bufHn[MAX_HOUSES_IN_STREET * 100];  // TODO Check overflow
  char* pcd;
  char* pca = bufHn;
  for (int i = 0; i < nbHn; i++) {
    for (pcd = tabHn[i]; *pcd; *pca++ = *pcd++)
      ;
    if (i != (nbHn - 1)) *pca++ = ',';
  }
  *pca = 0;
  // LOG("[removeDuplicateHn] res:'%s'\n", bufHn);
  copyStringIntoField(pStreet, F_STREET_HOUSENUMBERS, (BYTE*)bufHn);
}

static void setupHouseNumbers(sStreet* pStreet) {
  BYTE* ps = pStreet->fields[F_STREET_HOUSENUMBERS];
  if (!*ps) return;
  char* tabHn[MAX_HOUSES_IN_STREET];  // TODO Check overflow
  int nbHn = 0;
  makeHnTab(pStreet, tabHn, &nbHn);
  // ICI
  putHnTab(pStreet, tabHn, nbHn);
}

static void outputStreet(FILE* fp, sStreet* pStreet) {
  setupHouseNumbers(pStreet);
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    fputs((const char*)pStreet->fields[i], fp);
    if (i != (NB_GEOFIELDS - 1)) fputc('\t', fp);
    else fputc('\n', fp);
  }
}

static void copyStringIntoField(sStreet* pInto, int fieldNum, BYTE* str) {
  FREE(pInto->fields[fieldNum]);
  pInto->fields[fieldNum] = XMCPY(str);
}

static void copyField(sStreet* pInto, sStreet* pFrom, int fieldNum) {
  copyStringIntoField(pInto, fieldNum, pFrom->fields[fieldNum]);
}

static void copyFields(sStreet* pInto, sStreet* pFrom) {
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    copyField(pInto, pFrom, i);
  }
}

static void copyFieldsExceptHousenumbers(sStreet* pInto, sStreet* pFrom) {
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    if (i == F_STREET_HOUSENUMBERS) continue;
    copyField(pInto, pFrom, i);
  }
}

static void mergeHousenumbers(sStreet* pInto, sStreet* pFrom) {
  BYTE* pf = pFrom->fields[F_STREET_HOUSENUMBERS];
  BYTE* pt = pInto->fields[F_STREET_HOUSENUMBERS];
  // LOG("[mergeHousenumbers]  pf:'%s'\n", pf);
  // LOG("[mergeHousenumbers]  pt:'%s'\n", pt);
  char* tabHn[MAX_HOUSES_IN_STREET];  // TODO Check overflow
  int nbHn = 0;
  char* pch = strtok((char*)pt, ",");
  while (pch != NULL) {
    bool found = false;
    char* pt = trimwhitespace(pch);
    if (!pt) found = true;
    else {
      // LOG("[mHn]   To: n:%d '%s'\n", nbHn, pch);
      for (int i = 0; i < nbHn; i++) {
        // LOG("[mHn] nbHn:%d i:'%d'\n", nbHn, i);
        if (!strcmp(tabHn[i], pt)) {
          found = true;
          break;
        }
      }
    }
    if (!found) tabHn[nbHn++] = pt;
    pch = strtok(NULL, ",");
  }
  pch = strtok((char*)pf, ",");
  while (pch != NULL) {
    bool found = false;
    // LOG("[mHn] From: n:%d '%s'\n", nbHn, pch);
    char* pt = trimwhitespace(pch);
    if (!pt) found = true;
    else {
      for (int i = 0; i < nbHn; i++) {
        // LOG("[mHn] nbHn:%d i:'%d'\n", nbHn, i);
        if (!strcmp(tabHn[i], pt)) {
          found = true;
          break;
        }
      }
    }
    if (!found) tabHn[nbHn++] = pt;
    pch = strtok(NULL, ",");
  }
  char bufHn[MAX_HOUSES_IN_STREET * 100];  // TODO Check overflow
  char* pcd;
  char* pca = bufHn;
  for (int i = 0; i < nbHn; i++) {
    for (pcd = tabHn[i]; *pcd; *pca++ = *pcd++)
      ;
    if (i != (nbHn - 1)) *pca++ = ',';
  }
  *pca = 0;
  // LOG("[mergeHousenumbers] res:'%s'\n", bufHn);
  copyStringIntoField(pInto, F_STREET_HOUSENUMBERS, (BYTE*)bufHn);
}

static void removeDuplicateHn(sStreet* pStreet) {
  BYTE* ps = pStreet->fields[F_STREET_HOUSENUMBERS];
  if (!*ps) return;
  char* tabHn[MAX_HOUSES_IN_STREET];  // TODO Check overflow
  int nbHn = 0;
  char* pch = strtok((char*)ps, ",");
  while (pch != NULL) {
    bool found = false;
    char* pt = trimwhitespace(pch);
    if (!pt) found = true;
    else {
      // LOG("[mHn]   To: n:%d '%s'\n", nbHn, pch);
      for (int i = 0; i < nbHn; i++) {
        // LOG("[mHn] nbHn:%d i:'%d'\n", nbHn, i);
        if (!strcmp(tabHn[i], pt)) {
          // LOG("[removeDuplicateHn] DUP:'%s'\n", pch);
          found = true;
          break;
        }
      }
    }
    if (!found) tabHn[nbHn++] = pt;
    pch = strtok(NULL, ",");
  }
  char bufHn[MAX_HOUSES_IN_STREET * 100];  // TODO Check overflow
  char* pcd;
  char* pca = bufHn;
  for (int i = 0; i < nbHn; i++) {
    for (pcd = tabHn[i]; *pcd; *pca++ = *pcd++)
      ;
    if (i != (nbHn - 1)) *pca++ = ',';
  }
  *pca = 0;
  // LOG("[removeDuplicateHn] res:'%s'\n", bufHn);
  copyStringIntoField(pStreet, F_STREET_HOUSENUMBERS, (BYTE*)bufHn);
}

bool fromIsBetter(sStreet* pInto, sStreet* pFrom, int fieldNum) {
  int lFrom, linto;
  lFrom = strlen((char*)pFrom->fields[fieldNum]);
  linto = strlen((char*)pInto->fields[fieldNum]);
  if (lFrom > linto) return (true);
  return (false);
}

static bool sameStreets(sStreet* pStreet0, sStreet* pStreet1) {
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    if (strcmp((char*)pStreet0->fields[i], (char*)pStreet1->fields[i])) {
      // LOG("[sameStreets] DIFFERS: field:%d prev:'%s' new:'%s'\n", i,
      // (char*)pStreet0->fields[i], (char*)pStreet1->fields[i]);
      return false;
    }
  }
  return true;
}

static sStreet* mergeStreets(sStreet* pInto, sStreet* pFrom) {
  if (sameStreets(pInto, pFrom)) return pInto;
  if (fromIsBetter(pInto, pFrom, F_STREET_NAME) ||
      fromIsBetter(pInto, pFrom, F_STREET_DISPLAY_NAME) ||
      fromIsBetter(pInto, pFrom, F_STREET_HOUSENUMBERS)) {
    copyFieldsExceptHousenumbers(pInto, pFrom);
  }
  if (!*pInto->fields[F_STREET_HOUSENUMBERS] &&
      !*pInto->fields[F_STREET_HOUSENUMBERS]) {
    removeDuplicateHn(pInto);
    return pInto;
  }
  mergeHousenumbers(pInto, pFrom);
  return pInto;
}

static void doIt(FILE* fpi, FILE* fpo) {
  sStreet* pLastStreet = NULL;
  sStreet* pStreet = newStreet();
  int n;

  for (;;) {
    int res = getStreet(fpi, pStreet);
    if (res == ERR_EOF) break;
    if (res == ERR_BAD_LINE) continue;
    nbStreets++;
    if (!(nbStreets % 10000000)) LOG("nbStreets:%dM\n", nbStreets / 10000000);
    if (!pLastStreet) {
      // First time
      pLastStreet = pStreet;
      pStreet = newStreet();
      continue;
    }
    // LOG("[doIt] osmId:%ld lastOsmId:%ld\n", pStreet->osmId,
    // pLastStreet->osmId);
    if (pStreet->osmId != pLastStreet->osmId) {
      removeDuplicateHn(pLastStreet);
      goto flushStreet;
    }
    // LOG("[doIt] HN:0x%08lx '%s'\n",
    // (long unsigned int)pLastStreet->fields[F_STREET_HOUSENUMBERS],
    // pLastStreet->fields[F_STREET_HOUSENUMBERS]);
    // LOG("[doIt] '%s'\n", pLastStreet->fields[F_STREET_DISPLAY_NAME]);
    // LOG("[doIt] osmId:%ld osm_type:'%s' class:'%s' dName:'%s'\n",
    // pStreet->osmId, pStreet->fields[F_STREET_OSM_TYPE],
    // pStreet->fields[F_STREET_CLASS],
    // pStreet->fields[F_STREET_DISPLAY_NAME]);
    mergeStreets(pLastStreet, pStreet);
    continue;

  flushStreet:
    if (pLastStreet) {
      outputStreet(fpo, pLastStreet);
      freeStreet(pLastStreet);
    }
    pLastStreet = pStreet;
    pStreet = newStreet();
  }
  if (pStreet->fields[F_STREET_ID]) outputStreet(fpo, pStreet);
  printf("nb geom entries:%d maxFieldSize:%d \n", nbStreets, maxFieldSize);
  freeStreet(pStreet);
  return;
error:
  fprintf(stderr, "Error doIt line:%d\n", __error_line__);
  freeStreet(pStreet);
  return;
}

int main(int argc, char* argv[]) {
  FILE *fpi, *fpo;
  char c;
  int i;
  char str[STRSIZ];
  if (argc != 2) {
    fprintf(stderr, "Usage mergestreets basename\n");
    return (-1);
  }
  const char* basename = argv[1];
  nbStreets = initHouse(basename, &streets);
  if (nbStreets < 0) GOTO_ERROR;
  sprintf(str, "%s_geo1.tsv", basename);
  LOG("[main] reading '%s'\n", str);
  if (!(fpi = fopen(str, "r"))) GOTO_ERROR;
  sprintf(str, "%s_search0.tsv", basename);
  LOG("[main] writing '%s'\n", str);
  if (!(fpo = fopen(str, "w"))) GOTO_ERROR;
  doIt(fpi, fpo);
  return 0;

error:
  fprintf(stderr, "Error main (line:%d errno:%d)\n", __error_line__, errno);
  return (-1);
}