//�ص�I/O,ʹ���¼������ʶI/O���
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

int overlapped_send() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
    printf("WSAStartup error.");
    exit(1);
  }

  //����overlapped socket(������socket)
  //����sockaddr
  SOCKET hSocket =
      WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  struct sockaddr_in sendAdr;
  sendAdr.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", (void *)&(sendAdr.sin_addr));
  sendAdr.sin_port = htons(3000);
  int sendAdrLen = sizeof(sendAdr);

  // connect
  if (connect(hSocket, (struct sockaddr *)&sendAdr, sizeof(sendAdr)) == -1) {
    printf("connect error!");
    exit(1);
  }

  //��ʼ״̬Ϊnosignal,�˹����õ�event����
  WSAEVENT evObj = WSACreateEvent();

  WSABUF dataBuf;
  char msg[1024] = {0};
  memset(msg, 65, 1024);
  msg[1023] = 0;
  dataBuf.buf = msg;
  dataBuf.len = strlen(msg) + 1;
  DWORD sendBytes = 0;

  WSAOVERLAPPED overlapped;
  overlapped.hEvent = evObj;

  // send
  //������ɷ���0,���������ݴ��䷵��-1,��erronode Ϊ WSA_IO_PENDING
  if (WSASend(hSocket, &dataBuf, 1, &sendBytes, 0, &overlapped, NULL) == -1) {
    //��������Ҫ����,��WaitForEvents�ȴ��������,�������֮���¼�Ϊsignal
    if (WSAGetLastError() == WSA_IO_PENDING) {
      puts("Background data send");
      WSAWaitForMultipleEvents(1, &evObj, TRUE, WSA_INFINITE, FALSE);
      // sendBytesΪʵ�ʴ����С
      WSAGetOverlappedResult(hSocket, &overlapped, &sendBytes, FALSE, NULL);
    } else {
      printf("send error.");
    }
  }

  printf("Send data size : %d\n", sendBytes);
  WSACloseEvent(evObj);
  closesocket(hSocket);
  WSACleanup();
  system("pause");
  return 0;
}

int overlapped_recv() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
    printf("WSAStartup error.");
    exit(1);
  }

  //����overlapped socket(������socket)
  //����sockaddr
  SOCKET hListenSock =
      WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  struct sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  // inet_pton(AF_INET, "127.0.0.1", (void *)&(servAddr.sin_addr));
  servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(3000);
  int servAddrLen = sizeof(servAddr);

  // bind
  if (bind(hListenSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
    printf("connect error!");
    exit(1);
  }

  // listen
  listen(hListenSock, SOMAXCONN);

  // accept
  SOCKET clientSock;
  struct sockaddr_in recvAddr;
  int recvAddrLen = sizeof(recvAddr);
  clientSock = accept(hListenSock, (struct sockaddr *)&recvAddr, &recvAddrLen);

  //��ʼ״̬Ϊnosignal,�˹����õ�event����
  WSAEVENT evObj = WSACreateEvent();

  WSABUF dataBuf;
  char msg[1024];
  dataBuf.buf = msg;
  dataBuf.len = 1024;
  DWORD sendBytes = 0;

  WSAOVERLAPPED overlapped;
  //��������Ϊ0
  memset(&overlapped, 0, sizeof(overlapped));
  overlapped.hEvent = evObj;

  DWORD flags = 0;

  // recv
  //recv��ɷ���0,����������recv�򷵻�-1,��erronode Ϊ WSA_IO_PENDING
  //���һ������ΪComplete Routine,������ΪNULL
  if (WSARecv(clientSock, &dataBuf, 1, &sendBytes, &flags, &overlapped, NULL) == -1) {
    //��������Ҫrecv,��WaitForEvents�ȴ�recv���,�������֮���¼�Ϊsignal
    if (WSAGetLastError() == WSA_IO_PENDING) {
      puts("Background data recv");
      WSAWaitForMultipleEvents(1, &evObj, TRUE, WSA_INFINITE, FALSE);
      // sendBytesΪʵ�ʴ����С
      WSAGetOverlappedResult(clientSock, &overlapped, &sendBytes, FALSE, NULL);
    } else {
      printf("send error.");
    }
  }

  printf("Recv data size : %d\n", sendBytes);
  WSACloseEvent(evObj);
  closesocket(hListenSock);
  closesocket(clientSock);
  WSACleanup();
  system("pause");
  return 0;
}