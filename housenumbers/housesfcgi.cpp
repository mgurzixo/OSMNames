#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fcgiapp.h"
#include "gennumbers.h"
#include "jsmn.h"

FCGX_Stream *in, *out, *err;
FCGX_ParamArray envp;

static int __error_line__;
static const char* __error_file__;

#define NB_TOKENS 256
#define MAX_INPUT_SIZE 5000
// Maximal number of housenumbers in a street
#define MAX_HOUSES_IN_QUERY 1000

static jsmn_parser parser;
static jsmntok_t tokens[NB_TOKENS];

static unsigned int nbStreets;
static sIndex* streets;

FILE* fpLog;

void initialize(void) {
  fpLog = (fopen("fcgi.log", "a"));
  LOG("-------------------------\n");
  nbStreets = initHouse("housenumbers", &streets);
  if (nbStreets < 0) LOG("[initialize] Can't initialize streets\n");
}

void logVar(char* varName) {
  const char* pc = FCGX_GetParam(varName, envp);
  if (!pc) pc = "???";
  LOG("%s:'%s'\n", varName, pc);
}

void return_error(const char* errorString) {
  LOG("[return_error] %s\n", errorString);
  FCGX_FPrintF(out,
               "Status: %s\r\n"
               "Content-type: text/html,; charset=utf-8\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "\r\n",
               errorString);
}

struct sQuery {
  OSMID streetId;
  char* pHousenumber;
  ZKSNUM zkn;
};

int nbQueries;
struct sQuery tabQueries[MAX_HOUSES_IN_QUERY];

void doQuery() {
  H16 hash;
  struct sQuery* pq;
  for (int i = 0; i < nbQueries; i++) {
    pq = tabQueries + i;
    pq->zkn = findHouse(streets, nbStreets, pq->streetId, pq->pHousenumber);
    // LOG("[doQuery] streetId:%ld housenumber:'%s' zkn:%ld\n", pq->streetId,
    // pq->pHousenumber, pq->zkn);
  }
  return;
}

void returnQuery(sQuery* pq) {
  char zk[15];
  char* pzk = zknToZk(pq->zkn, zk);
  FCGX_FPrintF(out, "{\"streetId\":%ld,\"housenumber\":\"%s\", \"zk\": \"%s\"}",
               pq->streetId, pq->pHousenumber, pzk);
  LOG("[doQuery] streetId:%ld housenumber:'%s' zkn:%s\n", pq->streetId,
      pq->pHousenumber, pzk);
}

void sendReply() {
  struct sQuery* pq;
  FCGX_FPrintF(out,
               "Status: 200\r\n"
               "Content-type: text/html,; charset=utf-8\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "\r\n");
  FCGX_FPrintF(out, "[");
  for (int i = 0; i < nbQueries; i++) {
    pq = tabQueries + i;
    returnQuery(pq);
    if (i < (nbQueries - 1)) FCGX_FPrintF(out, ",");
  }
  FCGX_FPrintF(out, "]");
}

// cf.
// https://web.archive.org/web/20150214174439/https://alisdair.mcdiarmid.org/2012/08/14/jsmn-example.html

bool jtoken_streq(char* buf, jsmntok_t* t, char* val) {
  return (strncmp(buf + t->start, val, t->end - t->start) == 0 &&
          strlen(val) == (size_t)(t->end - t->start));
}

// WARNING: Destructive
char* jtoken_tostr(char* buf, jsmntok_t* t) {
  buf[t->end] = '\0';
  return buf + t->start;
}

const char* jtokenType(jsmntok_t* t) {
  switch (t->type) {
    case JSMN_UNDEFINED:
      return "JSMN_UNDEFINED";
    case JSMN_OBJECT:
      return "JSMN_OBJECT";
    case JSMN_ARRAY:
      return "JSMN_ARRAY";
    case JSMN_STRING:
      return "JSMN_STRING";
    case JSMN_PRIMITIVE:
      return "JSMN_PRIMITIVE";
    default:
      return "!UNKNOWN TOKEN TYPE!";
  }
}

typedef enum { JSMN_BOOLEAN, JSMN_NULL, JSMN_NUMBER } jsmn_primitive_t;

typedef enum { START = 1, QUERY, KEY, VAL, STOP } parse_state;
parse_state state;

const char* stateStr(parse_state state) {
  switch (state) {
    case START:
      return "START";
    case QUERY:
      return "QUERY";
    case KEY:
      return "KEY";
    case VAL:
      return "VAL";
    case STOP:
      return "STOP";
    default:
      return "!UNKNOWN STATE!";
  }
}

void logQuery(struct sQuery queries[], int nbQueries) {
  struct sQuery* pQuery;
  int i;
  for (i = 0; i < nbQueries; i++) {
    pQuery = queries + i;
    LOG("[logQuery] #%d streetId:%ld  pHousenumber:'%s'\n", i, pQuery->streetId,
        pQuery->pHousenumber);
  }
}

int parseInput(int len, char* body) {
  int nbTokens;
  size_t object_tokens = 0;
  uint64_t streetId;
  char *pHousenumber, *pStreetId, *pKey;
  int nkeys = -1;

  // LOG("[parseInput] %s\n", body);
  jsmn_init(&parser);
  nbTokens = jsmn_parse(&parser, body, len, tokens, NB_TOKENS);
  nbQueries = 0;
  state = START;
  // LOG("[parseInput] nbTokens %d\n", nbTokens);
  // i: token num
  // j: nb remaining tokens
  for (size_t i = 0, j = 1; j > 0; i++, j--) {
    jsmntok_t* t = &tokens[i];
    // LOG("[parseInput] token type: %s state:%s j:%d nkeys:%d\n",
    // jtokenType(t), stateStr(state), j, nkeys);
    // Should never reach uninitialized tokens
    if ((t->start == -1) || (t->end == -1)) GOTO_ERROR;
    if (t->type == JSMN_ARRAY) j += t->size;
    else if (t->type == JSMN_OBJECT) j += 2 * t->size;
    switch (state) {
      case START:
        if (t->type != JSMN_ARRAY) GOTO_ERROR;
        state = QUERY;
        object_tokens = t->size;
        if (object_tokens == 0) state = STOP;
        break;
      case QUERY:
        if (t->type != JSMN_OBJECT) GOTO_ERROR;
        state = KEY;
        object_tokens = t->size;
        if (object_tokens == 0) state = STOP;
        if (object_tokens != 2) GOTO_ERROR;  // only 2 keys in object
        tabQueries[nbQueries].pHousenumber = NULL;
        tabQueries[nbQueries].streetId = -1;
        nbQueries++;
        nkeys = object_tokens;

        break;
      case KEY:
        object_tokens--;
        if (t->type != JSMN_STRING) GOTO_ERROR;
        pKey = jtoken_tostr(body, t);
        // LOG("[parseInput] key:'%s'\n", pKey);
        state = VAL;
        break;
      case VAL:
        object_tokens--;
        if (!strcmp(pKey, "streetId")) {
          if (t->type != JSMN_PRIMITIVE) GOTO_ERROR;
          pStreetId = jtoken_tostr(body, t);
          streetId = 0;
          sscanf(pStreetId, "%lu", &streetId);
          if (!streetId) GOTO_ERROR;
          // LOG("[parseInput] #%d pStreetId:'%s' streetId:%lu\n",
          // nbQueries,
          //     pStreetId, streetId);
          tabQueries[nbQueries - 1].streetId = streetId;
        } else if (!strcmp(pKey, "housenumber")) {
          if (t->type != JSMN_STRING) GOTO_ERROR;
          pHousenumber = jtoken_tostr(body, t);
          if (!*pHousenumber) GOTO_ERROR;
          // LOG("[parseInput] #%d housenumber:'%s'\n", nbQueries,
          // pHousenumber);
          tabQueries[nbQueries - 1].pHousenumber = pHousenumber;
        } else GOTO_ERROR;
        if (--nkeys == 0) {
          // Parsing of entry complete, check that we got all keys
          if (!*tabQueries[nbQueries - 1].pHousenumber ||
              (tabQueries[nbQueries - 1].streetId < 0))
            GOTO_ERROR;
          state = QUERY;
        } else {
          // Continue parsing other keys
          state = KEY;
        }
        break;
      case STOP:
        // Just consume the tokens
        break;
      default:
        GOTO_ERROR;
    }
  }
  // LOG("[parseInput] got %d queries\n", nbQueries);
  // logQuery(tabQueries, nbQueries);
  return nbQueries;
error:
  LOG("[parseInput] Error line:%d\n", __error_line__);
  return -1;
}

int main() {
  char* pc;
  char c;
  char body[MAX_INPUT_SIZE + 1];
  // int count = 0;
  /* Initialization. */
  initialize();

  /* Response loop. */
  while (FCGX_Accept(&in, &out, &err, &envp) >= 0) {
    LOG("----\n");
    size_t contentLength;
    const char* pcc;

    pcc = FCGX_GetParam("CONTENT_LENGTH", envp);
    contentLength = strtol(pcc, NULL, 10);
    // LOG("contentLength:%lu\n", contentLength);
    if (contentLength > MAX_INPUT_SIZE) {
      return_error("413 Request too large");
      continue;
    }

    if (nbStreets < 0) {
      return_error("500 Internal Server Error");
      continue;
    }
    pc = body;
    while ((c = FCGX_GetChar(in)) != EOF) {
      // LOG("%c", c);
      *pc++ = c;
    }
    *pc = 0;
    if (parseInput(contentLength, body) < 0) {
      return_error("400 Bad Request");
      continue;
    }
    doQuery();
    sendReply();
  }
  return 0;
error:
  LOG("[main] Error line:%d\n", __error_line__);
  return -1;
}