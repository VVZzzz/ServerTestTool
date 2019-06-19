//重叠I/O,用CompletionRoutine检测I/O完成(recv)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024

//LPWSAOVERLAPPED_COMPLETION_ROUTINE为CompRoutine原型
void CALLBACK CompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
WSABUF dataBuf;
char buf[BUF_SIZE];
DWORD recvBytes = 0;

int cmplroutine_recv() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
    printf("WSAStartup error.");
    exit(1);
  }

  //创建overlapped socket(非阻塞socket)
  //设置sockaddr
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

  //初始状态为nosignal,人工重置的event对象
  WSAEVENT evObj = WSACreateEvent();

  dataBuf.buf = buf;
  dataBuf.len = BUF_SIZE;

  WSAOVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(overlapped));
  overlapped.hEvent = evObj;

  DWORD flags = 0;
  int idx = 0;

  // recv
  // recv完成返回0,还仍有数据recv则返回-1,置erronode 为 WSA_IO_PENDING
  if (WSARecv(clientSock, &dataBuf, 1, &recvBytes, &flags, &overlapped,
              CompRoutine) == -1) {
    //仍有数据要recv,用WaitForEvents等待recv完成,传输完成之后事件为signal
    if (WSAGetLastError() == WSA_IO_PENDING) {
      puts("Background data recv");
    }
  }
  //将最后一项的alertable设为TRUE,表示I/O完成时回调函数
  //该函数返回后,立即执行回调函数
  idx = WSAWaitForMultipleEvents(1, &evObj, FALSE, WSA_INFINITE, TRUE);

  for (int i = 0; i < 100; i++) {
    printf("%d ", i);
    Sleep(1000);
  }

  if (idx == WAIT_IO_COMPLETION)
    puts("Overlapped I/O completed");
  else {
    printf("WSARecv() error.");
    exit(1);
  }

  WSACloseEvent(evObj);
  closesocket(hListenSock);
  closesocket(clientSock);
  WSACleanup();
  system("pause");
  return 0;
}

//当I/O完成时会回调该函数
//CompRoutine原型
void CALLBACK CompRoutine(DWORD dwError, DWORD szRecvBytes,
                          LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags) {
  if (dwError != 0) {
    printf("compRoutine error.");
    exit(1);
  } else {
    recvBytes = szRecvBytes;
    printf("Received message: %s \n", buf);
  }
}