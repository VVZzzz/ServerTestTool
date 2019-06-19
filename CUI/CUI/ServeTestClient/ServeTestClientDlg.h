
// ServeTestClientDlg.h: 头文件
//

#pragma once
#include "Resource.h"
#include "CClient.h"

// CServeTestClientDlg 对话框
class CServeTestClientDlg : public CDialogEx {
  // 构造
 public:
  CServeTestClientDlg(CWnd* pParent = nullptr);  // 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_SERVETESTCLIENT_DIALOG };
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV 支持

  // 实现
 protected:
  HICON m_hIcon;

  // 生成的消息映射函数
  virtual BOOL OnInitDialog();
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  //开始测试
  afx_msg void OnBnClickedOk();
  //停止测试
  afx_msg void OnBnClickedStop();
  //退出
  afx_msg void OnBnClickedCancel();
  //销毁对话框
  afx_msg void OnDestroy();
  DECLARE_MESSAGE_MAP()

  public:
	  //向CList控件添加显示信息
  inline void AddInformation(const CString& strMsg) {
    CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_INFO);
    pList->InsertItem(0, strMsg);
  }

 private:
  //初始化Clist控件
  void InitListCtrl();
  //初始化界面信息
  void InitGUI();

 private:
  CClient m_Client;  //客户端对象
};
