//用纯重叠I/O实现Echo服务器(不使用IOCP模型)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024

//回调读函数
void CALLBACK ReadCompRoutine(IN DWORD dwError, IN DWORD cbTransferred,
                              IN LPWSAOVERLAPPED lpOverlapped,
                              IN DWORD dwFlags);
void CALLBACK WriteCompRoutine(IN DWORD dwError, IN DWORD cbTransferred,
                               IN LPWSAOVERLAPPED lpOverlapped,
                               IN DWORD dwFlags);

typedef struct {
  SOCKET hClntSock;
  char buf[BUF_SIZE];
  WSABUF wsaBuf;
} PER_IO_DATA, *LPPER_IO_DATA;

int pureOverlappedEchoServer() {
  // 1. WSAStartup
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) == -1) {
    printf("WSAStartup() error.");
    exit(1);
  }
  // 2. listensocket serverAddr
  //建立支持重叠I/O的socket
  unsigned long mode = 1;
  SOCKET hListenSock =
      WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  //将socket改为非阻塞模式,FIONBIO用于更改socketI/O模式
  //此处需深入了解
  ioctlsocket(hListenSock, FIONBIO, &mode);

  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(3000);

  // 3.bind
  if (bind(hListenSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) ==
      SOCKET_ERROR) {
    printf("error handling.");
    exit(1);
  }
  // 4.listen
  if (listen(hListenSock, SOMAXCONN) == -1) {
    printf("listen error.");
    exit(1);
  }

  // 5.accept clientSocket
  SOCKET hClientSocket;
  struct sockaddr_in clientAddr;
  int clientAddrLen = sizeof(clientAddr);
  LPWSAOVERLAPPED lpOvlp;  // overlapper结构体指针
  DWORD recvBytes;         //接收到的字节数
  DWORD flagsInfo =
      0;  // 调用WSARecv(...,&flagsInfo)前一定要赋值为0,因为他是IN_OUT
  LPPER_IO_DATA hbInfo;

  while (true) {
    // SleepEx第二个参数为TRUE可以将进程设置为alertable wait状态
    // alertable wait状态是等待接收系统消息的线程状态.
    //只有alertable wait的线程才可以回调Completion Routine函数
    SleepEx(100, TRUE);
    //非阻塞socket,accept会立即返回
    hClientSocket =
        accept(hListenSock, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (hClientSocket == INVALID_SOCKET) {
      //注意此处hListenSock,为非阻塞socket.返回-1不一定代表错误,可能是未accept完.
      //此处不能写EWOULDBLOCK,EWOULDBLOCK是指系统的错误errorNum
      if (WSAGetLastError() == WSAEWOULDBLOCK)
        continue;
      else {
        printf("accept error.");
        exit(1);
      }
    }

    printf("client socket %d conneted...", hClientSocket);
    //每个客户端都有自己的Overlapped结构
    lpOvlp = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
    memset(lpOvlp, 0, sizeof(WSAOVERLAPPED));

    //创捷自定义结构体对象
    hbInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
    hbInfo->hClntSock = (DWORD)hClientSocket;
    (hbInfo->wsaBuf).buf = hbInfo->buf;
    (hbInfo->wsaBuf).len = BUF_SIZE;

    //基于Completion routine的重叠I/O不需要event对象进行完成I/O的操作
    //故hEvent成员可以存放其他变量.
    //由于lpOvlp是回调函数compRoutine的参数,同时我们想把hbInfo传进去,所以我们将hEvent成员变为hbInfo
    //这里hEvent实际上是void*,故可以传
    lpOvlp->hEvent = (HANDLE)hbInfo;

    // 6. wsarecv
    //此处由于时非阻塞的socket,WSARecv立即返回,进入下一次的while
    // 而一旦I/O完成时将回调这个ReadCompRoutine函数(会中断线程),因为线程时alertable的.
    int recvRes;
    recvRes = WSARecv(hClientSocket, &(hbInfo->wsaBuf), 1, &recvBytes,
                      &flagsInfo, lpOvlp, ReadCompRoutine);
    if (recvRes == SOCKET_ERROR) {
      int errorno = WSAGetLastError();
      // eerrorno会置为ERROR_IO_PENDING,代表Overlapped I/O operation is in
      // progress.
      int t = 1;
    }
  }

  closesocket(hListenSock);
  closesocket(hClientSocket);
  WSACleanup();
  return 0;
}

void CALLBACK ReadCompRoutine(IN DWORD dwError, IN DWORD cbTransferred,
                              IN LPWSAOVERLAPPED lpOverlapped,
                              IN DWORD dwFlags) {
  // cbTransferred 是count bytes即传输的字节数
  //这里即recvBytes,已经接收到的字节数
  LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
  SOCKET hSock = hbInfo->hClntSock;
  LPWSABUF wsaBuf = &(hbInfo->wsaBuf);
  DWORD sendBytes;

  if (cbTransferred == 0) {
    //对方已经关闭连接
    closesocket(hSock);
    free(lpOverlapped->hEvent);
    free(lpOverlapped);
    puts("client disconnected...");
  } else {
    //回声echo
    //char tmp[BUF_SIZE] = " :from server";
    //strcat_s(wsaBuf->buf, BUF_SIZE, tmp);
    wsaBuf->len = cbTransferred;
    // I/O完成操作时,调用WriteCompRoutine函数
    int sendRes;
    sendRes = WSASend(hSock, wsaBuf, 1, &sendBytes, 0, lpOverlapped,
                      WriteCompRoutine);
    if (sendRes == -1) {
      int errorno = WSAGetLastError();
      int t;
    }
  }
}

void CALLBACK WriteCompRoutine(IN DWORD dwError, IN DWORD cbTransferred,
                               IN LPWSAOVERLAPPED lpOverlapped,
                               IN DWORD dwFlags) {
  LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
  SOCKET hSock = hbInfo->hClntSock;
  LPWSABUF wsaBuf = &(hbInfo->wsaBuf);
  DWORD sendBytes;
  DWORD flagInfo = 0;
  //注意此处,接收完数据继续回调ReadCompRoutine
  //即交替调用ReadCompRoutine和WriteCompRoutine
  int recvRes;
  recvRes = WSARecv(hSock, wsaBuf, 1, &cbTransferred, &flagInfo, lpOverlapped,
                    ReadCompRoutine);
  if (recvRes == -1) {
    int errorno = WSAGetLastError();
    int t;
  }
}
