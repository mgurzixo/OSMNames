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
    if (fieldSize >= (sizeof(motlu) - 5)) GOTO_ERROR;
    c = getc(fp);
    if (c < 0x20) {
      *pca = 0;
      //     // LOG("[getMot] c:'%c' motlu:'%s'\n", c, motlu);
      if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
      return (c);
    } else if (c < 0x80) {
      fieldSize += 1;
      *pca++ = c;
    } else if (c < 0xc0) GOTO_ERROR;
    else if (c < 0xe0) {  // 2 byte UTF-8
      fieldSize += 2;
      *pca++ = c;
      *pca++ = getc(fp);
    } else if (c < 0xf0) {  // 3 byte UTF-8
      fieldSize += 3;
      *pca++ = c;
      *pca++ = getc(fp);
      *pca++ = getc(fp);
    } else if (c < 0xf8) {  // 4 byte UTF-8
      fieldSize += 4;
      *pca++ = c;
      *pca++ = getc(fp);
      *pca++ = getc(fp);
      *pca++ = getc(fp);
    } else if (c == 0xff) {
      *pca = 0;
      // LOG("[getMot] c:'%c' motlu:'%s'\n", c, motlu);
      if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
      return (c);
    } else GOTO_ERROR;
  }
  GOTO_ERROR;
error:
  *pca = 0;
  LOG("[getMot] c:0x%02x ERROR@%d\n", c, __error_line__);
  if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
  return 0;
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

// Modifies string
char* trimwhitespace(char* str) {
  char* end;
  // Trim leading space
  while (isspace((unsigned char)*str)) str++;

  if (*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

// Separator is ANY filter char
// cf.
// https://stackoverflow.com/questions/3889992/how-does-strtok-split-the-string-into-tokens-in-c
char* mystrtok(char* str, const char* filter) {
  if (filter == NULL) {
    return str;
  }
  static char* pcd = NULL;
  static bool myEof = false;
  if (str != NULL) {
    pcd = str;
    myEof = false;
  }
  if (myEof == true) {
    return NULL;
  }
  char* ptrReturn = pcd;
  for (int j = 0; pcd; j++) {
    if (pcd[j] == '\0') {
      myEof = true;
      return ptrReturn;
    }
    for (int i = 0; filter[i] != '\0'; i++) {
      if (pcd[j] == filter[i]) {
        // if (pcd[j] == ';') LOG("[mystrtok] got ';'");
        pcd[j] = '\0';
        pcd += j + 1;
        return ptrReturn;
      }
    }
  }
  return NULL;
}

// TODO Check overflow
int makeTabHnFromStr(char** tabHn, char* str, int nbHn) {
  char* pch = mystrtok(str, HN_SEPARATORS);
  if (!pch || !*pch) return (0);
  while (pch != NULL) {
    bool found = false;
    char* pt = trimwhitespace(pch);
    if (!*pt) found = true;
    else {
      for (int i = 0; i < nbHn; i++) {
        if (!strcmp(tabHn[i], pt)) {
          found = true;
          break;
        }
      }
    }
    if (!found) tabHn[nbHn++] = (char*)XMCPY((BYTE*)pt);
    pch = mystrtok(NULL, HN_SEPARATORS);
  }
  return nbHn;
}
