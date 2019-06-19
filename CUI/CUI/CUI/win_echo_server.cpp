#include <WinSock2.h>
#include <stdio.h>
#include "errohanling.h"

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024

int win_echo_server() {
  WSADATA wsaData;

  // 1.加载环境,使用winsock2.2
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    ErrorHandling("WSAStartup() error!");

  // 2. 服务器Socket,端口号3000
  SOCKET hServSock;
  hServSock = socket(AF_INET, SOCK_STREAM, 0);
  if (hServSock == INVALID_SOCKET) ErrorHandling("create serer socket error.");
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(3000);

  // 3.bind socket
  if (bind(hServSock, (struct sockaddr *)&servaddr, sizeof(servaddr)) ==
      SOCKET_ERROR)
    ErrorHandling("bin error.");

  // 4. listen socket: 等待连接队列长度为5
  if (listen(hServSock, 5) == SOCKET_ERROR) ErrorHandling("listen error.");

  // 5. accept建立新连接
  SOCKET hClntSock;
  struct sockaddr_in clientaddr;
  int clientaddrlen = sizeof(clientaddr);
  int strLen;
  char msg[BUF_SIZE];

  //一个一个处理连接,即前一个连接断开后才可accept下一个连接
  //尽管下一个连接connect已经返回了.
  //但由于server没有accept,它还不能进行与server通信
  //即这个连接可以进行send(即该socket可写),但在client.cpp中,他会阻塞在recv中

  for (int i = 0; i < 5; i++) {
    hClntSock =
        accept(hServSock, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if (hClntSock == INVALID_SOCKET)
      ErrorHandling("accept error.");
    else
      printf("connected client %d \n", i + 1);

    // 6. send recv数据
    while ((strLen = recv(hClntSock, msg, BUF_SIZE, 0)) != 0)
      send(hClntSock, msg, strLen, 0);
    closesocket(hClntSock);
  }

  // 7. close listensocket
  closesocket(hServSock);
  WSACleanup();
  return 0;
}
