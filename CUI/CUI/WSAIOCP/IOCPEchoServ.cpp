#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winerror.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024
#define READ 3
#define WRITE 4
//该文件是IOCP模型的echo服务器实现

// socket info
typedef struct {
  SOCKET hClientSock;
  struct sockaddr_in clientAddr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// overlapped i/o info
typedef struct {
  OVERLAPPED overlapped;
  WSABUF wsaBuf;
  char buffer[BUF_SIZE];
  int rwMode;  // Read or Write
} PER_IO_DATA, *LPPER_IO_DATA;

//单独处理所有I/O的线程函数
unsigned WINAPI EchoThreadMain(LPVOID completionPortIO);

int main() {
  // 1.WSAStartup
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) == -1) {
    printf("WSAStartup error.\n");
    exit(1);
  }

  // IOCP模型有两步:1.创建CP对象  2.建立完成端口对象和套接字的关系
  // 2.创建完成端口对象(CP对象)
  //最后一个为确定开启多少线程用来处理I/O,为0则默认开启和CPU核数相等的线程
  HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  //将端口对象通过参数分配到线程中
  /*
  for (int i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
    _beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
  }
  */
  for (int i = 0; i < 2; i++) {
    _beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
  }

  // 3.创建支持重叠I/Osocket,sockaddr_in
  SOCKET hServSock =
      WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(3000);
  // 4. bind  listen
  bind(hServSock, (struct sockaddr *)&servAddr, sizeof(servAddr));
  listen(hServSock, SOMAXCONN);

  //创建保存socketinfo和overlappedIO的变量,每个客户端下信息都不一样
  LPPER_HANDLE_DATA lpHandleInfo;
  LPPER_IO_DATA lpIOInfo;

  DWORD recvBytes, flags = 0;

  while (1) {
    struct sockaddr_in clntAddr;
    int clntAddrLen = sizeof(clntAddr);
    //会阻塞到此处,hClntSock同样是支持重叠I/O操作的.
    SOCKET hClntSock =
        accept(hServSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
    lpHandleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
    lpHandleInfo->hClientSock = hClntSock;
    memcpy(&(lpHandleInfo->clientAddr), &clntAddr, clntAddrLen);

    //将CP对象和socket建立联系
    //第三个参数是通过GetQueued...函数返回得到,用来传递已完成I/O相关信息.
    CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)lpHandleInfo, 0);

    lpIOInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
    memset(&(lpIOInfo->overlapped), 0, sizeof(OVERLAPPED));
    (lpIOInfo->wsaBuf).buf = lpIOInfo->buffer;
    (lpIOInfo->wsaBuf).len = BUF_SIZE;

    // IOCP不会区分是输入还是输出状态,只通知完成I/O状态.故需自定义状态
    lpIOInfo->rwMode = READ;

    //注意这里没有使用compRoutine回调函数
    //倒数第二个参数,该指针地址实际上就是lpIOInfo地址,同样由GetQueue...函数得到
	//WSARecv实际上是"投递一个接收数据的请求",而非真正的接收数据,真正接收数据的活给线程函数干.
    int recvRes;
    recvRes = WSARecv(hClntSock, &(lpIOInfo->wsaBuf), 1, &recvBytes, &flags,
                      &(lpIOInfo->overlapped), NULL);
    if (recvRes == SOCKET_ERROR) {
      // Overlapped I/O,WSAGetLastError返回I/O Pending
      int temp = WSAGetLastError();
      int t = 0;
    }
  }

  return 0;
}

unsigned WINAPI EchoThreadMain(LPVOID pCompletionPortIO) {
  HANDLE hComPort = pCompletionPortIO;
  SOCKET sock;
  DWORD bytesTrans, flags = 0;
  LPPER_IO_DATA ioInfo;
  LPPER_HANDLE_DATA handleInfo;
  while (true) {
    //使用如下函数确认I/O完成的确认
    //如果CP对象和socket还没有建立联系或者没有数据传输,该线程函数会挂起不占用CPU时间片.
    //如果有数据传输,则返回
    GetQueuedCompletionStatus((HANDLE)hComPort, &bytesTrans,
                              (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo,
                              INFINITE);
    sock = handleInfo->hClientSock;
    if (ioInfo->rwMode == READ) {
      puts("message received.");
      if (bytesTrans == 0) {
        //传输EOF,对方关闭连接
        closesocket(sock);
        free(handleInfo);
        free(ioInfo);
        continue;
      }

      //收到消息后发送给客户端
      memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
      ioInfo->wsaBuf.len = bytesTrans;
      ioInfo->rwMode = WRITE;
      WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, flags, &(ioInfo->overlapped),
              NULL);

      //再次接收客户端消息
      ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
      memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
      ioInfo->wsaBuf.len = BUF_SIZE;
      ioInfo->wsaBuf.buf = ioInfo->buffer;
      ioInfo->rwMode = READ;
      WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped),
              NULL);
    } else {
      puts("message sent!");
      free(ioInfo);
    }
  }
  return 0;
}
