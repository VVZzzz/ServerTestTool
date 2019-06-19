//�ô��ص�I/Oʵ��Echo������(��ʹ��IOCPģ��)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024

//�ص�������
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
  //����֧���ص�I/O��socket
  unsigned long mode = 1;
  SOCKET hListenSock =
      WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  //��socket��Ϊ������ģʽ,FIONBIO���ڸ���socketI/Oģʽ
  //�˴��������˽�
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
  LPWSAOVERLAPPED lpOvlp;  // overlapper�ṹ��ָ��
  DWORD recvBytes;         //���յ����ֽ���
  DWORD flagsInfo =
      0;  // ����WSARecv(...,&flagsInfo)ǰһ��Ҫ��ֵΪ0,��Ϊ����IN_OUT
  LPPER_IO_DATA hbInfo;

  while (true) {
    // SleepEx�ڶ�������ΪTRUE���Խ���������Ϊalertable wait״̬
    // alertable wait״̬�ǵȴ�����ϵͳ��Ϣ���߳�״̬.
    //ֻ��alertable wait���̲߳ſ��Իص�Completion Routine����
    SleepEx(100, TRUE);
    //������socket,accept����������
    hClientSocket =
        accept(hListenSock, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (hClientSocket == INVALID_SOCKET) {
      //ע��˴�hListenSock,Ϊ������socket.����-1��һ���������,������δaccept��.
      //�˴�����дEWOULDBLOCK,EWOULDBLOCK��ָϵͳ�Ĵ���errorNum
      if (WSAGetLastError() == WSAEWOULDBLOCK)
        continue;
      else {
        printf("accept error.");
        exit(1);
      }
    }

    printf("client socket %d conneted...", hClientSocket);
    //ÿ���ͻ��˶����Լ���Overlapped�ṹ
    lpOvlp = (LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
    memset(lpOvlp, 0, sizeof(WSAOVERLAPPED));

    //�����Զ���ṹ�����
    hbInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
    hbInfo->hClntSock = (DWORD)hClientSocket;
    (hbInfo->wsaBuf).buf = hbInfo->buf;
    (hbInfo->wsaBuf).len = BUF_SIZE;

    //����Completion routine���ص�I/O����Ҫevent����������I/O�Ĳ���
    //��hEvent��Ա���Դ����������.
    //����lpOvlp�ǻص�����compRoutine�Ĳ���,ͬʱ�������hbInfo����ȥ,�������ǽ�hEvent��Ա��ΪhbInfo
    //����hEventʵ������void*,�ʿ��Դ�
    lpOvlp->hEvent = (HANDLE)hbInfo;

    // 6. wsarecv
    //�˴�����ʱ��������socket,WSARecv��������,������һ�ε�while
    // ��һ��I/O���ʱ���ص����ReadCompRoutine����(���ж��߳�),��Ϊ�߳�ʱalertable��.
    int recvRes;
    recvRes = WSARecv(hClientSocket, &(hbInfo->wsaBuf), 1, &recvBytes,
                      &flagsInfo, lpOvlp, ReadCompRoutine);
    if (recvRes == SOCKET_ERROR) {
      int errorno = WSAGetLastError();
      // eerrorno����ΪERROR_IO_PENDING,����Overlapped I/O operation is in
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
  // cbTransferred ��count bytes��������ֽ���
  //���ＴrecvBytes,�Ѿ����յ����ֽ���
  LPPER_IO_DATA hbInfo = (LPPER_IO_DATA)(lpOverlapped->hEvent);
  SOCKET hSock = hbInfo->hClntSock;
  LPWSABUF wsaBuf = &(hbInfo->wsaBuf);
  DWORD sendBytes;

  if (cbTransferred == 0) {
    //�Է��Ѿ��ر�����
    closesocket(hSock);
    free(lpOverlapped->hEvent);
    free(lpOverlapped);
    puts("client disconnected...");
  } else {
    //����echo
    //char tmp[BUF_SIZE] = " :from server";
    //strcat_s(wsaBuf->buf, BUF_SIZE, tmp);
    wsaBuf->len = cbTransferred;
    // I/O��ɲ���ʱ,����WriteCompRoutine����
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
  //ע��˴�,���������ݼ����ص�ReadCompRoutine
  //���������ReadCompRoutine��WriteCompRoutine
  int recvRes;
  recvRes = WSARecv(hSock, wsaBuf, 1, &cbTransferred, &flagInfo, lpOverlapped,
                    ReadCompRoutine);
  if (recvRes == -1) {
    int errorno = WSAGetLastError();
    int t;
  }
}
