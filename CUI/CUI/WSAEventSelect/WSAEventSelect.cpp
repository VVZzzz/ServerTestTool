/*WSAEventSelect异步server模型*/
#include <WinSock2.h>
#include <stdio.h>
#include <list>
#include <vector>
#include "errohanling.h"

#pragma comment(lib, "ws2_32.lib")

template <typename T>
void deleteArrIdx1(std::vector<T> &dataArr, std::vector<int> &idxArr) {
  //时间复杂度O(n),空间复杂度O(n),简单置换
  if (idxArr.empty()) return;
  std::vector<T> tempdata;
  int compidxidx = 0;
  int compidx = idxArr[compidxidx];
  for (int i = 0; i < dataArr.size(); i++) {
    if (i != compidx) {
      tempdata.push_back(dataArr[i]);
    } else {
      if (++compidxidx < idxArr.size()) compidx = idxArr[compidxidx];
    }
  }
  dataArr = tempdata;
}

template <typename T>
void deleteArrIdx2(std::vector<T> &dataArr, std::vector<int> &idxArr) {
  //空间复杂度O(1),时间复杂度O(n)
  if (idxArr.empty()) return;
  int newSize = dataArr.size() - idxArr.size();
  idxArr.push_back(dataArr.size());
  int exchangePoint = idxArr[0];
  for (int i = 0; i < idxArr.size() - 1; i++) {
    int j = idxArr[i] + 1;
    for (; j < idxArr[i + 1]; j++) {
      dataArr[exchangePoint++] = dataArr[j];
    }
  }
  dataArr.resize(newSize);
  idxArr.pop_back();
}

int main(int argc, TCHAR *argv[]) {
  // 1.初始化套接字库
  WSADATA wsaData;
  int nError = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (nError != NO_ERROR) return -1;

  // 2.创建监听socket
  SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN addrSrv;
  addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  addrSrv.sin_family = AF_INET;
  addrSrv.sin_port = htons(3000);

  // 3.bind socket
  if (bind(sockSrv, (sockaddr *)&addrSrv, sizeof(addrSrv)) == SOCKET_ERROR) {
    closesocket(sockSrv);
    return -1;
  }

  // 4. 开启监听
  if (listen(sockSrv, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(sockSrv);
    return -1;
  }

  // WSACreateEvente创建一个手动重置的初始状态为non-signaled的event内核对象(匿名)
  WSAEVENT hListenEvent = WSACreateEvent();
  // select去检测监听socket是否有新连接
  // WSAEventSelect实际上是将一个socket与一个事件连接起来,如果这个事件signaled,说明这个socket是可读可写等.
  if (WSAEventSelect(sockSrv, hListenEvent, FD_ACCEPT) == -1) {
    WSACloseEvent(hListenEvent);
    closesocket(sockSrv);
    WSACleanup();
    return -1;
  }

  std::vector<SOCKET> hSockArr;
  std::vector<WSAEVENT> hEventArr;
  std::vector<int> toDeleteIdxArr;

  // SOCKET hSockArr[WSA_MAXIMUM_WAIT_EVENTS];
  // WSAEVENT hEventArr[WSA_MAXIMUM_WAIT_EVENTS];
  WSANETWORKEVENTS netEvents;
  int numOfClntSock = 0;
  // hSockArr[numOfClntSock] = sockSrv;
  // hEventArr[numOfClntSock] = hListenEvent;
  hSockArr.push_back(sockSrv);
  hEventArr.push_back(hListenEvent);
  numOfClntSock++;

  int posInfo = 0;
  int startIndx = 0;

  SOCKET clientSock;
  struct sockaddr_in clntaddr;
  int clntaddrlen = sizeof(clntaddr);

  int strLen = 0;
  char strBuf[1024] = {0};

  while (1) {
    //等待hEventArr中的事件变为signaled,得到较小事件的idx
    posInfo = WSAWaitForMultipleEvents(numOfClntSock, &hEventArr[0], FALSE,
                                       WSA_INFINITE, FALSE);
    startIndx = posInfo - WSA_WAIT_EVENT_0;
    //对每个已变为signaled的事件进行等待
    for (int i = startIndx; i < numOfClntSock; i++) {
      int sigEventIdx =
          WSAWaitForMultipleEvents(1, &hEventArr[i], TRUE, 0, FALSE);
      //如果等待失败或者超时,continue
      if (sigEventIdx == WSA_WAIT_FAILED || sigEventIdx == WSA_WAIT_TIMEOUT)
        continue;
      //得到已signaled的idx
      sigEventIdx = i;
      //得到hSockArr[i]这个socket发生的网络事件(FD_ACCEPT,...),即第三个参数
      //并reset事件hEventArr[i],使之为nosignaled.
      WSAEnumNetworkEvents(hSockArr[sigEventIdx], hEventArr[sigEventIdx],
                           &netEvents);
      //发生"新连接"网络事件
      if (netEvents.lNetworkEvents & FD_ACCEPT) {
        if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
          puts("Accept error.");
          break;
        }
        clientSock = accept(hSockArr[sigEventIdx], (struct sockaddr *)&clntaddr,
                            &clntaddrlen);

        //新添加一个关联clientSock的事件(到相应的Array中),该事件为FD_READ和FD_CLOSE时会signaled.
        hListenEvent = WSACreateEvent();
        WSAEventSelect(clientSock, hListenEvent, FD_READ | FD_CLOSE);

        // hEventArr[numOfClntSock] = hListenEvent;
        // hSockArr[numOfClntSock] = clientSock;
        hEventArr.push_back(hListenEvent);
        hSockArr.push_back(clientSock);
        numOfClntSock++;
        printf("connected new client : %d ...", clientSock);
      }
      //接收数据时
      if (netEvents.lNetworkEvents & FD_READ) {
        if (netEvents.iErrorCode[FD_READ_BIT] != 0) {
          puts("Read Error");
          break;
        }
        printf("FD_READ: %d",hSockArr[sigEventIdx]);
        strLen = recv(hSockArr[sigEventIdx], strBuf, 1024, 0);
        // echo
        send(hSockArr[sigEventIdx], strBuf, strLen, 0);
      }
      //断开连接
      if (netEvents.lNetworkEvents & FD_CLOSE) {
        if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
          puts("Close error");
          break;
        }
        printf("FD_CLOSE: %d", hSockArr[sigEventIdx]);
        //关闭事件内核对象与socket
        WSACloseEvent(hEventArr[sigEventIdx]);
        closesocket(hSockArr[sigEventIdx]);

        //将event和socket从数组中清除,先记住标志,再统一清除
        // numOfClntSock--;
        // deleteEvents(hEventArr, sigEventIdx, numOfClntSock);
        // deleteSockets(hSockArr, sigEventIdx, numOfClntSock);
        toDeleteIdxArr.push_back(sigEventIdx);
      }
    }

    numOfClntSock -= toDeleteIdxArr.size();
    //从那两个vector中删除toDeleteIdxArr中的idx对应元素
    deleteArrIdx2<SOCKET>(hSockArr, toDeleteIdxArr);
    deleteArrIdx2<WSAEVENT>(hEventArr, toDeleteIdxArr);
    toDeleteIdxArr.clear();
  }
  closesocket(sockSrv);
  WSACleanup();
  return 0;
}