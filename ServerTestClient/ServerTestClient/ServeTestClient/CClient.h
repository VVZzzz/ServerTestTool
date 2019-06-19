/*
 * 指定并发线程数模拟多个客户端连接服务器,并发送信息.对服务器进行压力测试.
 * 该类为客户端类,客户端采用简单的多线程阻塞式socket.
 * 每个线程发送一次数据(可自行修改)
 */
#pragma once

// 屏蔽deprecation警告
#pragma warning(disable : 4996)

#define MAX_BUF_LEN 8096
#define DEFAULT_LOCALIP _T("127.0.0.1")
#define DEFAULT_MESSAGE _T("Hello,Server!")
#define DEFAULT_NUMTHREADS 100
#define DEFAULT_PORT 3000

class CClient;
class CServeTestClientDlg;

//传给连接线程函数的参数结构,保存pClient指针
typedef struct _threadParam_connection {
  CClient* pClient;
} THREADPARAM_CONNECTION, *LPTHREADPARAM_CONNECTION;

//传递给收发数据线程函数的参数结构
typedef struct _threadParam_sendrecv {
  CClient* pClient;
  SOCKET sock;
  int nThreadID;
  char szBuffer[MAX_BUF_LEN];  //缓冲区
} THREADPARAM_SENDRECV, *LPTHREADPARAM_SENDRECV;

class CClient {
 public:
  CClient();
  ~CClient();
  //加载socket环境
  bool LoadSocketLib();
  //卸载socket环境
  void UnLoadSocketLib() { WSACleanup(); }

  //在主界面上显示消息
  void ShowMessage(const CString strFmt, ...);

  //获取服务器IP地址
  CString GetLocalIP();

  /***********设置信息函数***************/
  //设置主界面指针
  void SetMainDlgPointer(CServeTestClientDlg* pDlg) { m_pMainDlg = pDlg; }
  //设置服务器IP地址
  void SetServerIP(const CString& strIP) { m_strServerIP = strIP; }
  //设置端口
  void SetPort(int nport) { m_nPort = nport; }
  //设置并发线程数
  void SetNumThreads(int nThreads) { m_nThreads = nThreads; }
  //设置发送消息
  void SetMessage(const CString& strMsg) { m_strMessage = strMsg; }

  /************测试相关函数************/
  //开始测试
  bool Start();
  //停止测试
  bool Stop();

 private:

  void CleanUp();

  //连接线程函数
  static DWORD WINAPI _ConnectionThreadFunc(LPVOID lpParam);
  //收发工作线程函数
  static DWORD WINAPI _SendRecvWorkThreadFunc(LPVOID lpParam);

  //与服务器建立连接
  bool EstablishConnections();
  bool ConnectToServer(SOCKET *pSock,CString strServerName,int nPort);

 private:
  CServeTestClientDlg*		m_pMainDlg;  // 主界面指针

  CString			m_strLocalIP;   // 本机IP地址
  CString			m_strServerIP;  // 服务器IP地址
  CString			m_strMessage;   // 要发送的消息

  int				m_nPort;     // 端口
  int				m_nThreads;  // 开启线程数

  HANDLE			m_hShutdownEvent;  // 通知线程退出事件内核对象
  HANDLE			m_hConnectThread;  // 连接线程对象

  HANDLE*			m_pThreads;  //保存开启的线程的句柄
  THREADPARAM_SENDRECV* m_pThreadParamSendRecv;  //保存收发线程参数
  int               m_nFinishThreads;  //标志有多少线程已经发送完毕
};
