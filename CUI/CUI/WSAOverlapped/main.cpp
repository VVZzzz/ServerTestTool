#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

extern int cmplroutine_recv();
extern int pureOverlappedEchoServer();

int main(int argc, char *argv[]) {
  /*
  if (argc != 3) {
    printf("usage: [-m]\n");
    printf("-m 1 : test compelete routine recv.\n");
    printf("-m 2 : start pure overlapped echo server.\n");
    printf("-m 3 : start pure overlapped echo client.\n");
    return 1;
  }
  switch (argv[2][0]) {
    case '1':
      cmplroutine_recv();
      break;
    case '2':
      pureOverlappedEchoServer();
      break;
    case '3':
      break;
    default:
      break;
  }
  */
  pureOverlappedEchoServer();
  return 0;
}
