#include "stdafx.h"
#include "CClient.h"
#include <WinSock2.h>
#include "ServeTestClientDlg.h"

#pragma comment(lib, "ws2_32.lib")

CClient::CClient()
    : m_pMainDlg(nullptr),
      m_strLocalIP(DEFAULT_LOCALIP),
      m_strServerIP(DEFAULT_LOCALIP),
      m_strMessage(DEFAULT_MESSAGE),
      m_nPort(DEFAULT_PORT),
      m_nThreads(DEFAULT_NUMTHREADS),
      m_hShutdownEvent(NULL),
      m_pThreads(NULL),
      m_pThreadParamSendRecv(NULL),
      m_nFinishThreads(0) {}

CClient::~CClient() {}

bool CClient::LoadSocketLib() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
    ShowMessage(_T("初始化Winsock2.2失败!\n"));
    return false;
  }
  return true;
}

void CClient::ShowMessage(const CString strFmt, ...) {
  // strFmt格式化传入的形参
  CString strMsg;
  va_list argList;
  va_start(argList, strFmt);
  strMsg.FormatV(strFmt, argList);
  va_end(argList);

  //在主界面显示
  m_pMainDlg->AddInformation(strMsg);
}

CString CClient::GetLocalIP() {
  char hostname[MAX_PATH];
  gethostname(hostname, MAX_PATH);
  struct hostent *lpHostEnt = gethostbyname(hostname);
  if (lpHostEnt == NULL) return DEFAULT_LOCALIP;
  LPSTR lpAddr = lpHostEnt->h_addr_list[0];
  struct in_addr inAddr;
  inAddr.S_un.S_addr = *(ULONG *)lpAddr;
  //修改localIP
  m_strLocalIP = CString(inet_ntoa(inAddr));
  return m_strLocalIP;
}

bool CClient::Start() {
  //退出事件为手动重置,初始状态为FALSE
  m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  //启动连接线程(connection)
  DWORD nThreadID;
  THREADPARAM_CONNECTION *pThreadParams = new THREADPARAM_CONNECTION();
  pThreadParams->pClient = this;
  m_hConnectThread = CreateThread(NULL, 0, _ConnectionThreadFunc, (LPVOID)pThreadParams, 0,
               &nThreadID);

  return true;
}

bool CClient::Stop() {
  if (NULL == m_hShutdownEvent) 
    return true;
  //将退出事件标志置位
  SetEvent(m_hShutdownEvent);
  //等待_ConnectionThread结束
  WaitForSingleObject(m_hConnectThread, INFINITE);

  //等待所有工作者线程结束
  WaitForMultipleObjects(m_nThreads, m_pThreads, TRUE, INFINITE);

  //关闭所有线程socket
  for (size_t i = 0; i < m_nThreads; i++) 
    closesocket(m_pThreadParamSendRecv[i].sock);

  CleanUp();
  TRACE("测试停止.\n");
  return true;
}

void CClient::CleanUp() {
  //为NULL说明并没有开始测试
  if (m_hShutdownEvent == NULL) return;

  if (m_pThreads) delete m_pThreads;
  m_pThreads = NULL;
  if (m_pThreadParamSendRecv) delete m_pThreadParamSendRecv;
  m_pThreadParamSendRecv = NULL;

  CloseHandle(m_hShutdownEvent);
  m_hShutdownEvent = NULL;
  CloseHandle(m_hConnectThread);
  m_hConnectThread = NULL;
}

DWORD __stdcall CClient::_ConnectionThreadFunc(LPVOID lpParam) {
  THREADPARAM_CONNECTION *pParams = (THREADPARAM_CONNECTION *)lpParam;
  CClient *pClient = pParams->pClient;

  TRACE(_T("_ConnectionThread开启...\n"));
  pClient->EstablishConnections();
  TRACE(_T("_ConnectionThread结束...\n"));

  if (pParams != NULL) delete pParams;
  pParams = NULL;
  return 0;
}

DWORD __stdcall CClient::_SendRecvWorkThreadFunc(LPVOID lpParam) {
  THREADPARAM_SENDRECV *pParams = (THREADPARAM_SENDRECV *)lpParam;
  CClient *pClient = (CClient *)pParams->pClient;

  char szTemp[MAX_BUF_LEN] = {0};
  int nBytesSend = 0;
  int nFinishThreads= 0;

  for (int i = 1; i < 4; i++) {
    memset(szTemp, 0, MAX_BUF_LEN);
    sprintf(szTemp, "第%d条信息: %s", pParams->szBuffer);
    nBytesSend = send(pParams->sock, szTemp, strlen(szTemp), 0);
    if (nBytesSend == SOCKET_ERROR) {
      TRACE(_T("错误: 发送第%d次信息失败,错误码为: %d.\n"), i, WSAGetLastError());
      return 1;
    } else if (nBytesSend == 0) {
      TRACE(_T("错误: 对方已关闭连接.\n"));
      return 1;
    }
    TRACE(_T("发送第%d次信息成功.\n", i));
    pClient->ShowMessage(_T("向服务器发送第%d条信息成功: %s"), i, szTemp);

    // 3s后再发下一条信息
    Sleep(3000);
  }
  //可能会有竞争
  nFinishThreads = ++(pClient->m_nFinishThreads);
  if (nFinishThreads == pClient->m_nThreads)
    pClient->ShowMessage(_T("测试并发%d个线程完毕!"), nFinishThreads);
  return 0;
}

bool CClient::EstablishConnections() {
  m_pThreads = new HANDLE[m_nThreads];
  m_pThreadParamSendRecv = new THREADPARAM_SENDRECV[m_nThreads];
  //开启设定好的线程数进行连接到服务器
  for (int i = 0; i < m_nThreads; i++) {
    //如果发生退出事件,则停止开启线程
    if (WaitForSingleObject(m_hShutdownEvent, 0) == WAIT_OBJECT_0) {
      TRACE(_T("接收到停止命令!\n"));
      return true;
    }

    if (!ConnectToServer(&m_pThreadParamSendRecv[i].sock, m_strServerIP,
                         m_nPort)) {
      ShowMessage(_T("连接服务器失败."));
      CleanUp();
      return false;
    }

    m_pThreadParamSendRecv[i].nThreadID = i + 1;
    sprintf(m_pThreadParamSendRecv[i].szBuffer, "第%d号线程 发送数据%s", i + 1,
            (char *)m_strMessage.GetString());
    Sleep(10);

    //连接服务器成功,接下来开启线程进行收发
    DWORD nThreadID;
    m_pThreadParamSendRecv[i].pClient = this;
    m_pThreads[i] =
        CreateThread(NULL, 0, _SendRecvWorkThreadFunc,
                     (LPVOID)(&m_pThreadParamSendRecv[i]), 0, &nThreadID);
  }
  return true;
}

bool CClient::ConnectToServer(SOCKET *pSock, CString strServerName, int nPort) {
  struct sockaddr_in ServerAddr = {0};
  struct hostent *serverHost;
  *pSock = socket(AF_INET, SOCK_STREAM, 0);
  if (INVALID_SOCKET == *pSock) {
    TRACE(_T("创建SOCKET错误!\n"));
    return false;
  }

  serverHost = gethostbyname((const char *)strServerName.GetString());
  if (serverHost == NULL) {
    TRACE(_T("gethostbyname错误!\n"));
    closesocket(*pSock);
    return false;
  }

  //设置服务器地址
  ServerAddr.sin_family = AF_INET;
  ServerAddr.sin_addr.S_un.S_addr = *(ULONG *)serverHost->h_addr_list[0];
  // memcpy_s(&(ServerAddr.sin_addr.S_un.S_addr), serverHost->h_length,
  //        serverHost->h_addr_list, serverHost->h_length);
  ServerAddr.sin_port = htons(m_nPort);

  if (SOCKET_ERROR ==
      connect(*pSock, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr))) {
    TRACE(_T("connect to server failed!\n"));
    closesocket(*pSock);
    return false;
  }

  return true;
}
