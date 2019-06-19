#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#include "errohanling.h"

int win_echo_select_serv() {
  // 1.加载环境
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) == -1) {
    ErrorHandling("WSAStartup error.");
  }

  // 2.建立listensocket
  SOCKET listensock = socket(AF_INET, SOCK_STREAM, 0);
  if (listensock == INVALID_SOCKET) ErrorHandling("listen socket error.");
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(3000);

  // 3.bind
  if (bind(listensock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    ErrorHandling("bind socket error.");
  // 4.listen
  if (listen(listensock, SOMAXCONN) == -1) ErrorHandling("bind socket error.");

  // 5.select
  fd_set fds, cpyfds;
  FD_ZERO(&fds);
  FD_SET(listensock, &fds);
  int fdnum = 0;
  timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  SOCKET clientsock;
  struct sockaddr_in clientaddr;
  int clientaddrlen = sizeof(clientaddr);
  int strLen = 0;
  char strBuf[1024] = {0};
  while (1) {
    cpyfds = fds;
    //每次都要重置(linux会修改这个时间,windows不会)
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if ((fdnum = select(0, &cpyfds, NULL, NULL, &timeout)) == SOCKET_ERROR)
      break;
    if (fdnum == 0) continue;
    for (int i = 0; i < fds.fd_count; i++) {
      if (FD_ISSET(fds.fd_array[i], &cpyfds)) {
        //新连接
        if (fds.fd_array[i] == listensock) {
          clientsock = accept(listensock, (struct sockaddr *)&clientaddr,
                              &clientaddrlen);
          if (clientsock == INVALID_SOCKET) ErrorHandling("accept error.");
          //将新的clientsocket置为,以下一次的观察
          FD_SET(clientsock, &fds);
          printf("connected : %d", clientsock);

        } else {
          //有数据可读
          strLen = recv(fds.fd_array[i], strBuf, 1024, 0);
          if (strLen == 0) {
            //对端关闭连接
            printf("close client : %d [fds]\n", fds.fd_array[i]);
            FD_CLR(fds.fd_array[i], &fds);
            printf("close client : %d [fds](after clear)\n", fds.fd_array[i]);
            closesocket(cpyfds.fd_array[i]);
            printf("close client : %d [cpyfds]\n", cpyfds.fd_array[i]);
          } else {
			  //send echo
            send(fds.fd_array[i], strBuf, strLen, 0);
          }
        }
      }
    }
  }
  closesocket(listensock);
  WSACleanup();
  return 0;
}