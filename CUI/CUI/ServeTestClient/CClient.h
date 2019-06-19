/*
 * ָ�������߳���ģ�����ͻ������ӷ�����,��������Ϣ.�Է���������ѹ������.
 * ����Ϊ�ͻ�����,�ͻ��˲��ü򵥵Ķ��߳�����ʽsocket.
 * ÿ���̷߳���һ������(�������޸�)
 */
#pragma once

// ����deprecation����
#pragma warning(disable : 4996)

#define MAX_BUF_LEN 8096
#define DEFAULT_LOCALIP _T("127.0.0.1")
#define DEFAULT_MESSAGE _T("Hello,Server!")
#define DEFAULT_NUMTHREADS 100
#define DEFAULT_PORT 3000

class CClient;
class CServeTestClientDlg;

//���������̺߳����Ĳ����ṹ,����pClientָ��
typedef struct _threadParam_connection {
  CClient* pClient;
} THREADPARAM_CONNECTION, *LPTHREADPARAM_CONNECTION;

//���ݸ��շ������̺߳����Ĳ����ṹ
typedef struct _threadParam_sendrecv {
  CClient* pClient;
  SOCKET sock;
  int nThreadID;
  char szBuffer[MAX_BUF_LEN];  //������
} THREADPARAM_SENDRECV, *LPTHREADPARAM_SENDRECV;

class CClient {
 public:
  CClient();
  ~CClient();
  //����socket����
  bool LoadSocketLib();
  //ж��socket����
  void UnLoadSocketLib() { WSACleanup(); }

  //������������ʾ��Ϣ
  void ShowMessage(const CString strFmt, ...);

  //��ȡ������IP��ַ
  CString GetLocalIP();

  /***********������Ϣ����***************/
  //����������ָ��
  void SetMainDlgPointer(CServeTestClientDlg* pDlg) { m_pMainDlg = pDlg; }
  //���÷�����IP��ַ
  void SetServerIP(const CString& strIP) { m_strServerIP = strIP; }
  //���ö˿�
  void SetPort(int nport) { m_nPort = nport; }
  //���ò����߳���
  void SetNumThreads(int nThreads) { m_nThreads = nThreads; }
  //���÷�����Ϣ
  void SetMessage(const CString& strMsg) { m_strMessage = strMsg; }

  /************������غ���************/
  //��ʼ����
  bool Start();
  //ֹͣ����
  bool Stop();

 private:

  void CleanUp();

  //�����̺߳���
  static DWORD WINAPI _ConnectionThreadFunc(LPVOID lpParam);
  //�շ������̺߳���
  static DWORD WINAPI _SendRecvWorkThreadFunc(LPVOID lpParam);

  //���������������
  bool EstablishConnections();
  bool ConnectToServer(SOCKET *pSock,CString strServerName,int nPort);

 private:
  CServeTestClientDlg*		m_pMainDlg;  // ������ָ��

  CString			m_strLocalIP;   // ����IP��ַ
  CString			m_strServerIP;  // ������IP��ַ
  CString			m_strMessage;   // Ҫ���͵���Ϣ

  int				m_nPort;     // �˿�
  int				m_nThreads;  // �����߳���

  HANDLE			m_hShutdownEvent;  // ֪ͨ�߳��˳��¼��ں˶���
  HANDLE			m_hConnectThread;  // �����̶߳���

  HANDLE*			m_pThreads;  //���濪�����̵߳ľ��
  THREADPARAM_SENDRECV* m_pThreadParamSendRecv;  //�����շ��̲߳���
  int               m_nFinishThreads;  //��־�ж����߳��Ѿ��������
};
