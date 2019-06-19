#ifndef ERRO_HANDLE_H_
#define ERRO_HANDLE_H_
#include <stdio.h>
#include <stdlib.h>
inline void ErrorHandling(const char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
#endif
