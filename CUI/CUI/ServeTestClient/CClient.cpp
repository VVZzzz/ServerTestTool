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
    ShowMessage(_T("��ʼ��Winsock2.2ʧ��!\n"));
    return false;
  }
  return true;
}

void CClient::ShowMessage(const CString strFmt, ...) {
  // strFmt��ʽ��������β�
  CString strMsg;
  va_list argList;
  va_start(argList, strFmt);
  strMsg.FormatV(strFmt, argList);
  va_end(argList);

  //����������ʾ
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
  //�޸�localIP
  m_strLocalIP = CString(inet_ntoa(inAddr));
  return m_strLocalIP;
}

bool CClient::Start() {
  //�˳��¼�Ϊ�ֶ�����,��ʼ״̬ΪFALSE
  m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  //���������߳�(connection)
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
  //���˳��¼���־��λ
  SetEvent(m_hShutdownEvent);
  //�ȴ�_ConnectionThread����
  WaitForSingleObject(m_hConnectThread, INFINITE);

  //�ȴ����й������߳̽���
  WaitForMultipleObjects(m_nThreads, m_pThreads, TRUE, INFINITE);

  //�ر������߳�socket
  for (size_t i = 0; i < m_nThreads; i++) 
    closesocket(m_pThreadParamSendRecv[i].sock);

  CleanUp();
  TRACE("����ֹͣ.\n");
  return true;
}

void CClient::CleanUp() {
  //ΪNULL˵����û�п�ʼ����
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

  TRACE(_T("_ConnectionThread����...\n"));
  pClient->EstablishConnections();
  TRACE(_T("_ConnectionThread����...\n"));

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
    sprintf(szTemp, "��%d����Ϣ: %s", pParams->szBuffer);
    nBytesSend = send(pParams->sock, szTemp, strlen(szTemp), 0);
    if (nBytesSend == SOCKET_ERROR) {
      TRACE(_T("����: ���͵�%d����Ϣʧ��,������Ϊ: %d.\n"), i, WSAGetLastError());
      return 1;
    } else if (nBytesSend == 0) {
      TRACE(_T("����: �Է��ѹر�����.\n"));
      return 1;
    }
    TRACE(_T("���͵�%d����Ϣ�ɹ�.\n", i));
    pClient->ShowMessage(_T("����������͵�%d����Ϣ�ɹ�: %s"), i, szTemp);

    // 3s���ٷ���һ����Ϣ
    Sleep(3000);
  }
  //���ܻ��о���
  nFinishThreads = ++(pClient->m_nFinishThreads);
  if (nFinishThreads == pClient->m_nThreads)
    pClient->ShowMessage(_T("���Բ���%d���߳����!"), nFinishThreads);
  return 0;
}

bool CClient::EstablishConnections() {
  m_pThreads = new HANDLE[m_nThreads];
  m_pThreadParamSendRecv = new THREADPARAM_SENDRECV[m_nThreads];
  //�����趨�õ��߳����������ӵ�������
  for (int i = 0; i < m_nThreads; i++) {
    //��������˳��¼�,��ֹͣ�����߳�
    if (WaitForSingleObject(m_hShutdownEvent, 0) == WAIT_OBJECT_0) {
      TRACE(_T("���յ�ֹͣ����!\n"));
      return true;
    }

    if (!ConnectToServer(&m_pThreadParamSendRecv[i].sock, m_strServerIP,
                         m_nPort)) {
      ShowMessage(_T("���ӷ�����ʧ��."));
      CleanUp();
      return false;
    }

    m_pThreadParamSendRecv[i].nThreadID = i + 1;
    sprintf(m_pThreadParamSendRecv[i].szBuffer, "��%d���߳� ��������%s", i + 1,
            (char *)m_strMessage.GetString());
    Sleep(10);

    //���ӷ������ɹ�,�����������߳̽����շ�
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
    TRACE(_T("����SOCKET����!\n"));
    return false;
  }

  serverHost = gethostbyname((const char *)strServerName.GetString());
  if (serverHost == NULL) {
    TRACE(_T("gethostbyname����!\n"));
    closesocket(*pSock);
    return false;
  }

  //���÷�������ַ
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
