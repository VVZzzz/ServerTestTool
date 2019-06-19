#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")


#define DEFAULT_BUFLEN 512

int recv_example() {
  WSADATA wsaData;
  int iResult;

  SOCKET ConnectSocket = INVALID_SOCKET;
  struct sockaddr_in clientService;
  const char *sendbuf = "this is test.";
  char recvbuf[DEFAULT_BUFLEN];
  int recvbuflen = DEFAULT_BUFLEN;

  //--------------------------------------------
  //��ʼ��Winsock,���ػ����Ϳ��ļ�
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != NO_ERROR) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
  }

  //--------------------------------------------
  //����Socket
  // IPV4,��������,TCPЭ��
  ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ConnectSocket == INVALID_SOCKET) {
    //��ʾ����Errorcode
    printf("Error at socket() :%ld\n", WSAGetLastError());
  }

  // sockaddr_in�ṹȷ��Ҫ���ӷ�������family,ip,port
  // sin_addr��IPV4�ĵ�ַ�ṹ��һ��IN_ADDR����.
  clientService.sin_family = AF_INET;
  //clientService.sin_addr.s_addr = InetPton(AF_INET,"192.168.20.129",NULL);
  clientService.sin_addr.s_addr = inet_addr("192.168.20.129");
  // host to net short.תΪ�����ֽ�˳��
  clientService.sin_port = htons(4321);

  //--------------------------------------------
  //connect���ӵ�server��
  //�˴�ע��SOCKADDR�ṹ,���ĵ�һ��ushort��ʾfamily,ʣ�µ�char[14]��ʾ��ַ(net˳��).
  //���Ĵ�С��Ա�����Ǻ�sockaddr_in��Ӧ.
  //������ʹ��SOCKADDR_STORGE�ṹ
  //�˴�ʹ������socket
  iResult =
      connect(ConnectSocket, (SOCKADDR *)&clientService, sizeof(clientService));
  if (iResult == SOCKET_ERROR) {
    //1. closesocket�ر�socket,ע��һ���ر�socket�����socket��ֵ�����������������socket��.
    //   ��һ������closesocket֮��,�Ͳ����ٶ����socket��������������.
    //2. closesocket����ʱ,�׽�����δ��ɵ�I/O������һ�����ܱ�֤���.
    //   ��Ӧ�ó���Ӧ���δ��ɵ�I/O���������õ��κ���Դ,ֱ��I/O����ȷʵ���Ϊֹ.
    closesocket(ConnectSocket);
    printf("Unable to connect to server: %d\n", WSAGetLastError());
    //��ֹʹ��ws2_32.dll
    WSACleanup();
    return 1;
  }

  //--------------------------------------------
  //send������ʼ��Ϣ
  // 1. sendֻ��˵���ѳɹ�����,������ȷ��server�Ƿ�ɹ��յ�
  // 2.
  // �������ϵͳ��û�п��õĻ������ռ�������Ҫ��������ݣ����ͽ���ֹ�������׽��������ڷ�����ģʽ��
  // 3.
  // ����len����Ϊ��ķ���������ģ����ҽ���ʵ����Ϊ�ɹ���
  // ����������£�send����������Ϊ��Чֵ������������Ϣ���׽��֣��������㳤�ȴ������ݱ���
  // 4. ��Ӧ���ò�ͬ�߳�ͬʱ��ͬһ���������ӵ�socket����send,����ܵ������ݽ�����һ�����send����ֳɶ��.
  iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
  if (iResult == SOCKET_ERROR) {
    closesocket(ConnectSocket);
    printf("Send error: %d\n", WSAGetLastError());
    WSACleanup();
    return 1;
  }

  printf("Sent %ld Bytes\n", iResult);

  //----------------------------------------------
  // shutdown�ر�send recv
  // shutdown���������׽����ϵķ��ͺͽ���

  // 1.SD_RECEIVE
  //   ����TCP
  //   ����socket�����еȴ����յ����ݻ���󵽴������,��reset���socket.��Ϊ�޷������ݴ��ݸ��û�.
  //   ����UDP
  //   ����ܴ�������ݲ��Ŷ�,���κ�����¶���������ICMP�������ݰ�

  // 2.SD_SEND,�����������send�����ĵ���
  //   ����TCP�׽��֣��ڽ��շ����ͺ�ȷ���������ݺ󽫷���FIN��

  // 3.shutdown����ر�socket,�������ͷ���Դ
  //   ��Ӧ�ڵ���closesocket֮ǰʹ��shutdownȷ�����ͽ�����������.
  iResult = shutdown(ConnectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed: %d\n", SD_SEND);
    closesocket(ConnectSocket);
    return 1;
  }

  //--------------------------------------------
  //��������ֱ���Է��ر�socket
  do {
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0) {
      printf(">-------Bytes received: %d\n", iResult);
      printf("%s\n", recvbuf);
    }
    else if (iResult==0){
      printf("Connection closed\n");
    } else
      printf("recv failed: %d\n", WSAGetLastError());
  } while (iResult > 0);

  //--------------------------------------------
  //�����Դ
  closesocket(ConnectSocket);
  WSACleanup();
  return 0;
}
