
// ServeTestClientDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "ServeTestClientDlg.h"
#include "ServeTestClient.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx {
 public:
  CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_ABOUTBOX };
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV 支持

  // 实现
 protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CServeTestClientDlg 对话框

CServeTestClientDlg::CServeTestClientDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_SERVETESTCLIENT_DIALOG, pParent) {
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServeTestClientDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CServeTestClientDlg, CDialogEx)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(IDOK, &CServeTestClientDlg::OnBnClickedOk)
ON_BN_CLICKED(IDC_STOP, &CServeTestClientDlg::OnBnClickedStop)
ON_BN_CLICKED(IDCANCEL, &CServeTestClientDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

// CServeTestClientDlg 消息处理程序

BOOL CServeTestClientDlg::OnInitDialog() {
  CDialogEx::OnInitDialog();

  // 将“关于...”菜单项添加到系统菜单中。

  // IDM_ABOUTBOX 必须在系统命令范围内。
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != nullptr) {
    BOOL bNameValid;
    CString strAboutMenu;
    bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
    ASSERT(bNameValid);
    if (!strAboutMenu.IsEmpty()) {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
  //  执行此操作
  SetIcon(m_hIcon, TRUE);   // 设置大图标
  SetIcon(m_hIcon, FALSE);  // 设置小图标

  // TODO: 在此添加额外的初始化代码
  InitGUI();

  return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CServeTestClientDlg::OnSysCommand(UINT nID, LPARAM lParam) {
  if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  } else {
    CDialogEx::OnSysCommand(nID, lParam);
  }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CServeTestClientDlg::OnPaint() {
  if (IsIconic()) {
    CPaintDC dc(this);  // 用于绘制的设备上下文

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()),
                0);

    // 使图标在工作区矩形中居中
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // 绘制图标
    dc.DrawIcon(x, y, m_hIcon);
  } else {
    CDialogEx::OnPaint();
  }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CServeTestClientDlg::OnQueryDragIcon() {
  return static_cast<HCURSOR>(m_hIcon);
}

void CServeTestClientDlg::OnBnClickedOk() {
  int nPort = GetDlgItemInt(IDC_EDIT_PORT);
  int nThreads = GetDlgItemInt(IDC_EDIT_THREADS);
  CString strMsg, strIP;
  GetDlgItemText(IDC_EDIT_MESSAGE,strMsg);
  GetDlgItemText(IDC_IPADDRESS_SERVER, strIP);
  if (nPort < 0 || nThreads < 0 || strMsg.IsEmpty() || strIP.IsEmpty()) {
    AfxMessageBox(_T("请输入合法参数!"));
    return;
  }
  m_Client.SetServerIP(strIP);
  m_Client.SetPort(nPort);
  m_Client.SetNumThreads(nThreads);
  m_Client.SetMessage(strMsg);

  //开始测试
  if (!m_Client.Start()) {
    AfxMessageBox(_T("启动失败"));
    return;
  }
  AddInformation(_T("开始测试!"));
  GetDlgItem(IDOK)->EnableWindow(FALSE);
  GetDlgItem(IDC_STOP)->EnableWindow(TRUE);
   //CDialogEx::OnOK();
}

//停止测试
void CServeTestClientDlg::OnBnClickedStop() { 
  AddInformation(_T("停止测试"));
  m_Client.Stop();
  GetDlgItem(IDOK)->EnableWindow(TRUE);
  GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
}

//退出
void CServeTestClientDlg::OnBnClickedCancel() {
  m_Client.Stop();
  m_Client.UnLoadSocketLib();
  CDialogEx::OnCancel();
}

void CServeTestClientDlg::OnDestroy() {
  OnBnClickedCancel();
  CDialogEx::OnDestroy();
}

void CServeTestClientDlg::InitListCtrl() {
  CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_INFO);
  pList->SetBkColor(RGB(120, 250, 112));
  //设置列表是项目及其子项目一起突出显示,并显示栅格
  pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
  pList->InsertColumn(0, _T("信息"), LVCFMT_LEFT,500);
}

void CServeTestClientDlg::InitGUI() {
  //初始化socket环境
  if (false == m_Client.LoadSocketLib()) {
    AfxMessageBox(_T("加载Winsock2.2失败,服务器端无法运行"));
    //该线程发送WM_QUIT消息到消息队列,该函数立即返回
    //检测到WM_QUIT消息,该线程退出.
    PostMessage(0);
  }

  //设置本机IP地址
  SetDlgItemText(IDC_IPADDRESS_SERVER, m_Client.GetLocalIP());
  //设置端口
  SetDlgItemInt(IDC_EDIT_PORT, DEFAULT_PORT);
  //设置默认并发线程数
  SetDlgItemInt(IDC_EDIT_THREADS, DEFAULT_NUMTHREADS);
  //设置默认发送消息
  SetDlgItemText(IDC_EDIT_MESSAGE, DEFAULT_MESSAGE);

  //初始化CListCtrl
  InitListCtrl();

  //设置主界面指针
  m_Client.SetMainDlgPointer(this);
}
