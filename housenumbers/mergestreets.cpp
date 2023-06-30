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

// static BYTE motlu[100 * 1024];

static int nbStreets = 0;

sStreet* newStreet() {
  sStreet* pStreet = (sStreet*)MALLOC(sizeof(sStreet));
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    pStreet->fields[i] = NULL;
  }
  return pStreet;
}

void razStreet(sStreet* pStreet) {
  for (int i = 0; i < NB_GEOFIELDS; i++) {
    RAZ(pStreet->fields[i]);
  }
}

void freeStreet(sStreet* pStreet) {
  // razStreet(pStreet);
  FREE(pStreet);
}

#define ERR_OK 0
#define ERR_EOF 1
#define ERR_BAD_LINE 2

// #define INCREMENT_FIELD_LENGTH \
//   if (++fieldSize >= sizeof(motlu)) GOTO_ERROR

// int maxFieldSize = -1;

// char getMot(FILE* fp) {
//   BYTE* pca;
//   BYTE c;
//   int fieldSize = 0;
//   for (pca = motlu;;) {
//     if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
//     c = getc(fp);
//     switch (c) {
//       case '\t':
//       case '\n':
//       case (BYTE)EOF:
//         *pca = 0;
//         // LOG("[getMot] c:'%c' motlu:'%s'\n", c, motlu);
//         return (c);
//       default:
//         if (c < 0x80) {
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = c;
//         } else if (c < 0xc0) GOTO_ERROR;
//         else if (c < 0xe0) {  // 2 byte UTF-8
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = c;
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = getc(fp);
//         } else if (c < 0xf0) {  // 3 byte UTF-8
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = c;
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = getc(fp);
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = getc(fp);
//         } else if (c < 0xf8) {  // 4 byte UTF-8
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = c;
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = getc(fp);
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = getc(fp);
//           INCREMENT_FIELD_LENGTH;
//           *pca++ = getc(fp);
//         } else GOTO_ERROR;
//     }
//   }
//   GOTO_ERROR;
// error:
//   *pca = 0;
//   LOG("[getMot] ERROR line:%d  street#%d motlu:'%s'\n", __error_line__,
//       nbStreets, motlu);
//   return 0;
// }

int getStreet(FILE* fp, sStreet* pStreet) {
  static bool isEof = false;
  if (isEof) return (ERR_EOF);
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
    // razStreet(pStreet);
  }
  return ERR_OK;
error:
  fprintf(stderr, "Error getStreet@%d nbStreets:%d\n", __error_line__,
          nbStreets);
  return ERR_BAD_LINE;
}

void doIt(FILE* fp) {
  sStreet* pStreet = newStreet();
  int n;

  for (;;) {
    int res = getStreet(fp, pStreet);
    if (res == ERR_EOF) break;
    if (res == ERR_BAD_LINE) continue;
    nbStreets++;
    if (!(nbStreets % 10000000)) LOG("nbStreets:%dM\n", nbStreets / 10000000);
  }
  printf("nb streets:%d maxFieldSize:%d \n", nbStreets, maxFieldSize);
  freeStreet(pStreet);
  return;
error:
  fprintf(stderr, "Error doIt line:%d\n", __error_line__);
  freeStreet(pStreet);
  return;
}

int main(int argc, char* argv[]) {
  FILE* fpi;
  char c;
  int i;
  char str[STRSIZ];
  if (argc != 2) {
    fprintf(stderr, "Usage mergestreets basename\n");
    return (-1);
  }
  const char* basename = argv[1];
  sprintf(str, "%s_geo1.tsv", basename);
  if (!(fpi = fopen(str, "r"))) GOTO_ERROR;
  doIt(fpi);
  return 0;

error:
  fprintf(stderr, "Error main (line:%d errno:%d)\n", __error_line__, errno);
  return (-1);
}