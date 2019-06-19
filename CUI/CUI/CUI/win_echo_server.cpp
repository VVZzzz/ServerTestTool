#include <WinSock2.h>
#include <stdio.h>
#include "errohanling.h"

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024

int win_echo_server() {
  WSADATA wsaData;

  // 1.���ػ���,ʹ��winsock2.2
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    ErrorHandling("WSAStartup() error!");

  // 2. ������Socket,�˿ں�3000
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

  // 4. listen socket: �ȴ����Ӷ��г���Ϊ5
  if (listen(hServSock, 5) == SOCKET_ERROR) ErrorHandling("listen error.");

  // 5. accept����������
  SOCKET hClntSock;
  struct sockaddr_in clientaddr;
  int clientaddrlen = sizeof(clientaddr);
  int strLen;
  char msg[BUF_SIZE];

  //һ��һ����������,��ǰһ�����ӶϿ���ſ�accept��һ������
  //������һ������connect�Ѿ�������.
  //������serverû��accept,�������ܽ�����serverͨ��
  //��������ӿ��Խ���send(����socket��д),����client.cpp��,����������recv��

  for (int i = 0; i < 5; i++) {
    hClntSock =
        accept(hServSock, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if (hClntSock == INVALID_SOCKET)
      ErrorHandling("accept error.");
    else
      printf("connected client %d \n", i + 1);

    // 6. send recv����
    while ((strLen = recv(hClntSock, msg, BUF_SIZE, 0)) != 0)
      send(hClntSock, msg, strLen, 0);
    closesocket(hClntSock);
  }

  // 7. close listensocket
  closesocket(hServSock);
  WSACleanup();
  return 0;
}
