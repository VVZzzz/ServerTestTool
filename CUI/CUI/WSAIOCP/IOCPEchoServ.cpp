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
//���ļ���IOCPģ�͵�echo������ʵ��

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

//������������I/O���̺߳���
unsigned WINAPI EchoThreadMain(LPVOID completionPortIO);

int main() {
  // 1.WSAStartup
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) == -1) {
    printf("WSAStartup error.\n");
    exit(1);
  }

  // IOCPģ��������:1.����CP����  2.������ɶ˿ڶ�����׽��ֵĹ�ϵ
  // 2.������ɶ˿ڶ���(CP����)
  //���һ��Ϊȷ�����������߳���������I/O,Ϊ0��Ĭ�Ͽ�����CPU������ȵ��߳�
  HANDLE hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  //���˿ڶ���ͨ���������䵽�߳���
  /*
  for (int i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
    _beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
  }
  */
  for (int i = 0; i < 2; i++) {
    _beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
  }

  // 3.����֧���ص�I/Osocket,sockaddr_in
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

  //��������socketinfo��overlappedIO�ı���,ÿ���ͻ�������Ϣ����һ��
  LPPER_HANDLE_DATA lpHandleInfo;
  LPPER_IO_DATA lpIOInfo;

  DWORD recvBytes, flags = 0;

  while (1) {
    struct sockaddr_in clntAddr;
    int clntAddrLen = sizeof(clntAddr);
    //���������˴�,hClntSockͬ����֧���ص�I/O������.
    SOCKET hClntSock =
        accept(hServSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
    lpHandleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
    lpHandleInfo->hClientSock = hClntSock;
    memcpy(&(lpHandleInfo->clientAddr), &clntAddr, clntAddrLen);

    //��CP�����socket������ϵ
    //������������ͨ��GetQueued...�������صõ�,�������������I/O�����Ϣ.
    CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)lpHandleInfo, 0);

    lpIOInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
    memset(&(lpIOInfo->overlapped), 0, sizeof(OVERLAPPED));
    (lpIOInfo->wsaBuf).buf = lpIOInfo->buffer;
    (lpIOInfo->wsaBuf).len = BUF_SIZE;

    // IOCP�������������뻹�����״̬,ֻ֪ͨ���I/O״̬.�����Զ���״̬
    lpIOInfo->rwMode = READ;

    //ע������û��ʹ��compRoutine�ص�����
    //�����ڶ�������,��ָ���ַʵ���Ͼ���lpIOInfo��ַ,ͬ����GetQueue...�����õ�
	//WSARecvʵ������"Ͷ��һ���������ݵ�����",���������Ľ�������,�����������ݵĻ���̺߳�����.
    int recvRes;
    recvRes = WSARecv(hClntSock, &(lpIOInfo->wsaBuf), 1, &recvBytes, &flags,
                      &(lpIOInfo->overlapped), NULL);
    if (recvRes == SOCKET_ERROR) {
      // Overlapped I/O,WSAGetLastError����I/O Pending
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
    //ʹ�����º���ȷ��I/O��ɵ�ȷ��
    //���CP�����socket��û�н�����ϵ����û�����ݴ���,���̺߳��������ռ��CPUʱ��Ƭ.
    //��������ݴ���,�򷵻�
    GetQueuedCompletionStatus((HANDLE)hComPort, &bytesTrans,
                              (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo,
                              INFINITE);
    sock = handleInfo->hClientSock;
    if (ioInfo->rwMode == READ) {
      puts("message received.");
      if (bytesTrans == 0) {
        //����EOF,�Է��ر�����
        closesocket(sock);
        free(handleInfo);
        free(ioInfo);
        continue;
      }

      //�յ���Ϣ���͸��ͻ���
      memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
      ioInfo->wsaBuf.len = bytesTrans;
      ioInfo->rwMode = WRITE;
      WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, flags, &(ioInfo->overlapped),
              NULL);

      //�ٴν��տͻ�����Ϣ
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
