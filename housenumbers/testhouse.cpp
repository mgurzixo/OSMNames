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

FILE* fpLog = stderr;
static unsigned int nbStreets;
static sIndex* streets;

int main(int argc, char* argv[]) {
  ZKSNUM n;
  nbStreets = initHouse("bid", &streets);
  if (nbStreets < 0) GOTO_ERROR;

  n = findHouse(streets, nbStreets, STREETID, HOUSENUMBER);
  LOG("Addr: streetId:%d housenumber:'%s' zkn:%ld\n", STREETID, HOUSENUMBER, n);
  return 0;

error:
  LOG("Error main (line:%d errno:%d)\n", __error_line__, errno);
  return (-1);
}