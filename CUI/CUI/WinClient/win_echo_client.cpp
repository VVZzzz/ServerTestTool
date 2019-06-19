#include <WS2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>
#include "errohanling.h"

#pragma comment(lib, "ws2_32.lib")
#define BUF_SIZE 1024

int main() {
  // 1. 加载环境
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    ErrorHandling("WSAStartup() error.");

  // 2. 建立clientsocket
  SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSock == INVALID_SOCKET) ErrorHandling("create socket error.");
  struct sockaddr_in serveraddr;
  serveraddr.sin_family = AF_INET;
  // serveraddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
  inet_pton(AF_INET, "127.0.0.1", (void *)&(serveraddr.sin_addr));
  serveraddr.sin_port = htons(3000);

  // 3. connect
  if (connect(clientSock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) ==
      SOCKET_ERROR)
    ErrorHandling("connect error");
  else
    printf("connected ......\n");

  // 4. send recv
  char msg[BUF_SIZE];
  int strLen;
  int readLen;
  while (1) {
    fputs("Input (Q to quit): ", stdout);
    fgets(msg, BUF_SIZE, stdin);

    if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) break;
    strLen = strlen(msg);
    send(clientSock, msg, strLen, 0);
    readLen = 0;
	//循环接收直至全部收完
    while (true) {
      readLen += recv(clientSock, msg, BUF_SIZE - 1, 0);
      if (readLen>=strLen)
		  break;
    }
    msg[strLen] = '\0';
    printf("msg from server: %s", msg);
  }

  closesocket(clientSock);
  WSACleanup();
  return 0;
}
