#include "fcgi_stdio.h" /* fcgi library; put it first*/

#include <stdlib.h>

int count;

void initialize(void) {
  count = 0;
}

void printvar(const char* varName) {
  char* pc = getenv(varName);
  if (!pc)
    pc = "???";
  printf("%s:'%s'<br>", varName, pc);
}

void main(int argc, char** argv, char** envp) {
  char* pc;
  /* Initialization. */
  initialize();

  /* Response loop. */
  while (FCGI_Accept() >= 0) {
    printf(
        "Content-type: text/html\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "FastCGI Hello! (C, fcgi_stdio library)<br>");
    printvar("SERVER_SOFTWARE");
    printvar("SERVER_NAME");
    printvar("GATEWAY_INTERFACE");
    printvar("SERVER_PROTOCOL");
    printvar("SERVER_PORT");
    printvar("SCRIPT_NAME");
    printvar("QUERY_STRING");
    printvar("REMOTE_HOST");
    printvar("REMOTE_ADDR");
    printvar("REMOTE_USER");
    printvar("CONTENT_TYPE");
    printvar("CONTENT_LENGTH");
    printvar("HTTP_ACCEPT");
    printvar("HTTP_USER_AGENT");
    printvar("REQUEST_METHOD");
    printvar("REQUEST_URI");
    printf("--------<br>");
    printvar("PATH_INFO");
    printvar("PATH_TRANSLATED");
    printvar("AUTH_TYPE");
    printvar("REMOTE_IDENT");
  }
}