
// UASToolDlg.h : 头文件
//

#pragma once
#include "afxwin.h"

#include "UASImp.h"


// CUASToolDlg 对话框
class CUASToolDlg : public CDialog
{
// 构造
public:
	CUASToolDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_UASTOOL_DIALOG };

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
	CEdit m_cUasIp;
	CEdit m_cUasPort;
	CEdit m_cUasCode;
	afx_msg void OnBnClickedBtnStartService();
	afx_msg void OnBnClickedBtnStopService2();

public:
	bgUASImp uas_;
	CListBox m_cInfo;
};
