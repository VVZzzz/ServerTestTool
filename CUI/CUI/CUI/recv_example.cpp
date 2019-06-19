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
  //初始化Winsock,加载环境和库文件
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != NO_ERROR) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
  }

  //--------------------------------------------
  //创建Socket
  // IPV4,基于连接,TCP协议
  ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ConnectSocket == INVALID_SOCKET) {
    //显示错误Errorcode
    printf("Error at socket() :%ld\n", WSAGetLastError());
  }

  // sockaddr_in结构确定要连接服务器的family,ip,port
  // sin_addr是IPV4的地址结构是一个IN_ADDR类型.
  clientService.sin_family = AF_INET;
  //clientService.sin_addr.s_addr = InetPton(AF_INET,"192.168.20.129",NULL);
  clientService.sin_addr.s_addr = inet_addr("192.168.20.129");
  // host to net short.转为网络字节顺序
  clientService.sin_port = htons(4321);

  //--------------------------------------------
  //connect连接到server上
  //此处注意SOCKADDR结构,它的第一个ushort表示family,剩下的char[14]表示地址(net顺序).
  //它的大小成员正好是和sockaddr_in对应.
  //但建议使用SOCKADDR_STORGE结构
  //此处使用阻塞socket
  iResult =
      connect(ConnectSocket, (SOCKADDR *)&clientService, sizeof(clientService));
  if (iResult == SOCKET_ERROR) {
    //1. closesocket关闭socket,注意一旦关闭socket则这个socket的值可以立即分配给其他socket的.
    //   故一旦调用closesocket之后,就不能再对这个socket进行其他操作了.
    //2. closesocket返回时,套接字上未完成的I/O操作不一定都能保证完成.
    //   故应用程序不应清除未完成的I/O请求所引用的任何资源,直到I/O请求确实完成为止.
    closesocket(ConnectSocket);
    printf("Unable to connect to server: %d\n", WSAGetLastError());
    //终止使用ws2_32.dll
    WSACleanup();
    return 1;
  }

  //--------------------------------------------
  //send发送起始信息
  // 1. send只能说明已成功发送,并不能确定server是否成功收到
  // 2.
  // 如果传输系统中没有可用的缓冲区空间来保存要传输的数据，则发送将阻止，除非套接字已置于非阻塞模式。
  // 3.
  // 调用len参数为零的发送是允许的，并且将被实现视为成功。
  // 在这种情况下，send将返回零作为有效值。对于面向消息的套接字，将发送零长度传输数据报。
  // 4. 不应该用不同线程同时向同一个面向连接的socket调用send,这可能导致数据交错及将一个大的send请求分成多个.
  iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
  if (iResult == SOCKET_ERROR) {
    closesocket(ConnectSocket);
    printf("Send error: %d\n", WSAGetLastError());
    WSACleanup();
    return 1;
  }

  printf("Sent %ld Bytes\n", iResult);

  //----------------------------------------------
  // shutdown关闭send recv
  // shutdown用来禁用套接字上的发送和接收

  // 1.SD_RECEIVE
  //   对于TCP
  //   若该socket上仍有等待接收的数据或随后到达的数据,则reset这个socket.因为无法将数据传递给用户.
  //   对于UDP
  //   则接受传入的数据并排队,在任何情况下都不会生成ICMP错误数据包

  // 2.SD_SEND,不允许后续对send函数的调用
  //   对于TCP套接字，在接收方发送和确认所有数据后将发送FIN。

  // 3.shutdown不会关闭socket,更不会释放资源
  //   故应在调用closesocket之前使用shutdown确保发送接收所有数据.
  iResult = shutdown(ConnectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed: %d\n", SD_SEND);
    closesocket(ConnectSocket);
    return 1;
  }

  //--------------------------------------------
  //接收数据直到对方关闭socket
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
  //清除资源
  closesocket(ConnectSocket);
  WSACleanup();
  return 0;
}
