using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include <cstdint>
#include <string>

#include "gennumbers.h"
#include "zkDefs.h"

static int __error_line__;
static const char* __error_file__;

FILE* fpLog = stderr;

static char* pLine;
static char line[STRSIZ];
static char motlu[STRSIZ];

static unsigned int nbStreets;

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
  uint32_t ui32;
  int32_t i32;
  int64_t i64, j64;

  if (!fgets(line, sizeof(line), fp)) return ERR_EOF;
  pLine = line;

  // osm_id
  if (getMot(fp) != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("osm_id:'%s'\n", motlu);
  i64 = strtoull(motlu, NULL, 10);
  if (i64 < 0) {
    j64 = i64;
    ui32 = strtol(motlu, NULL, 10);
    i64 = ui32;
    LOG("[getHn] converting %ld to %ld line:%s", j64, i64, line);
  }
  phn->osmId = strtoull(motlu, NULL, 10);
  // if (phn->osmId < 0) LOG("***%s", line);
  if (!phn->osmId) GOTO_ERROR;

  // street_id
  if (getMot(fp) != '\t') GOTO_ERROR;
  if (!*motlu) GOTO_ERROR;
  // LOG("street_id:'%s'\n", motlu);
  phn->streetId = strtoull(motlu, NULL, 10);
  if (phn->streetId < 0) LOG("***%s", line);
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

void doIt(FILE* fp) {
  sHousenumber hn;
  ZKPLUS houses[MAX_HOUSES_IN_STREET];
  int nbHouses = 0;
  int n;

  for (;;) {
    int res = getHn(fp, &hn);
    if (res == ERR_EOF) break;
    if (res == ERR_BAD_LINE) continue;
    nbStreets++;
    if (!(nbStreets % 1000000)) LOG("nbStreets:%d\n", nbStreets);
  }
  printf("nb houses:%d\n", nbHouses);
  return;
error:
  fprintf(stderr, "Error doIt line:%d\n", __error_line__);
  return;
}

int main(int argc, char* argv[]) {
  FILE* fp;
  char c;
  int i;
  if (argc == 2) {
    if (!(fp = fopen(argv[1], "r"))) GOTO_ERROR;
  } else fp = stdin;
  // if (!(fp = fopen("bid.tsv", "r")))
  //   GOTO_ERROR;
  doIt(fp);
  return 0;

error:
  fprintf(stderr, "Error main (line:%d errno:%d)\n", __error_line__, errno);
  return (-1);
}