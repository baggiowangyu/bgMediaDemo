
// UACToolDlg.h : 头文件
//

#pragma once
#include "afxwin.h"

#include "UACImp.h"


// CUACToolDlg 对话框
class CUACToolDlg : public CDialog
{
// 构造
public:
	CUACToolDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_UACTOOL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnBnClickedBtnRegister();
	afx_msg void OnBnClickedBtnUnregister();
	afx_msg void OnBnClickedBtnCall();
	afx_msg void OnBnClickedBtnCloseCall();
	afx_msg void OnBnClickedBtnSendMessage();

public:
	CEdit m_cUasIp;
	CEdit m_cUasPort;
	CEdit m_cUasCode;
	CEdit m_cUacIp;
	CEdit m_cUacPort;
	CEdit m_cUacCode;
	CEdit m_cMessage;
	CListBox m_cInfo;

public:
	bgUACImp uac_;
	
};
