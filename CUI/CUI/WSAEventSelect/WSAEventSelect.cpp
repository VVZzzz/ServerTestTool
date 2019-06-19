/*WSAEventSelect�첽serverģ��*/
#include <WinSock2.h>
#include <stdio.h>
#include <list>
#include <vector>
#include "errohanling.h"

#pragma comment(lib, "ws2_32.lib")

template <typename T>
void deleteArrIdx1(std::vector<T> &dataArr, std::vector<int> &idxArr) {
  //ʱ�临�Ӷ�O(n),�ռ临�Ӷ�O(n),���û�
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
  //�ռ临�Ӷ�O(1),ʱ�临�Ӷ�O(n)
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
  // 1.��ʼ���׽��ֿ�
  WSADATA wsaData;
  int nError = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (nError != NO_ERROR) return -1;

  // 2.��������socket
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

  // 4. ��������
  if (listen(sockSrv, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(sockSrv);
    return -1;
  }

  // WSACreateEvente����һ���ֶ����õĳ�ʼ״̬Ϊnon-signaled��event�ں˶���(����)
  WSAEVENT hListenEvent = WSACreateEvent();
  // selectȥ������socket�Ƿ���������
  // WSAEventSelectʵ�����ǽ�һ��socket��һ���¼���������,�������¼�signaled,˵�����socket�ǿɶ���д��.
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
    //�ȴ�hEventArr�е��¼���Ϊsignaled,�õ���С�¼���idx
    posInfo = WSAWaitForMultipleEvents(numOfClntSock, &hEventArr[0], FALSE,
                                       WSA_INFINITE, FALSE);
    startIndx = posInfo - WSA_WAIT_EVENT_0;
    //��ÿ���ѱ�Ϊsignaled���¼����еȴ�
    for (int i = startIndx; i < numOfClntSock; i++) {
      int sigEventIdx =
          WSAWaitForMultipleEvents(1, &hEventArr[i], TRUE, 0, FALSE);
      //����ȴ�ʧ�ܻ��߳�ʱ,continue
      if (sigEventIdx == WSA_WAIT_FAILED || sigEventIdx == WSA_WAIT_TIMEOUT)
        continue;
      //�õ���signaled��idx
      sigEventIdx = i;
      //�õ�hSockArr[i]���socket�����������¼�(FD_ACCEPT,...),������������
      //��reset�¼�hEventArr[i],ʹ֮Ϊnosignaled.
      WSAEnumNetworkEvents(hSockArr[sigEventIdx], hEventArr[sigEventIdx],
                           &netEvents);
      //����"������"�����¼�
      if (netEvents.lNetworkEvents & FD_ACCEPT) {
        if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
          puts("Accept error.");
          break;
        }
        clientSock = accept(hSockArr[sigEventIdx], (struct sockaddr *)&clntaddr,
                            &clntaddrlen);

        //�����һ������clientSock���¼�(����Ӧ��Array��),���¼�ΪFD_READ��FD_CLOSEʱ��signaled.
        hListenEvent = WSACreateEvent();
        WSAEventSelect(clientSock, hListenEvent, FD_READ | FD_CLOSE);

        // hEventArr[numOfClntSock] = hListenEvent;
        // hSockArr[numOfClntSock] = clientSock;
        hEventArr.push_back(hListenEvent);
        hSockArr.push_back(clientSock);
        numOfClntSock++;
        printf("connected new client : %d ...", clientSock);
      }
      //��������ʱ
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
      //�Ͽ�����
      if (netEvents.lNetworkEvents & FD_CLOSE) {
        if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
          puts("Close error");
          break;
        }
        printf("FD_CLOSE: %d", hSockArr[sigEventIdx]);
        //�ر��¼��ں˶�����socket
        WSACloseEvent(hEventArr[sigEventIdx]);
        closesocket(hSockArr[sigEventIdx]);

        //��event��socket�����������,�ȼ�ס��־,��ͳһ���
        // numOfClntSock--;
        // deleteEvents(hEventArr, sigEventIdx, numOfClntSock);
        // deleteSockets(hSockArr, sigEventIdx, numOfClntSock);
        toDeleteIdxArr.push_back(sigEventIdx);
      }
    }

    numOfClntSock -= toDeleteIdxArr.size();
    //��������vector��ɾ��toDeleteIdxArr�е�idx��ӦԪ��
    deleteArrIdx2<SOCKET>(hSockArr, toDeleteIdxArr);
    deleteArrIdx2<WSAEVENT>(hEventArr, toDeleteIdxArr);
    toDeleteIdxArr.clear();
  }
  closesocket(sockSrv);
  WSACleanup();
  return 0;
}