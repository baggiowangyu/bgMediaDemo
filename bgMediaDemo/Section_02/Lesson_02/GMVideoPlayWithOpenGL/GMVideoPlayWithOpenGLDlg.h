
// GMVideoPlayWithOpenGLDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "GxxGmDecoderPlay.h"


// CGMVideoPlayWithOpenGLDlg 对话框
class CGMVideoPlayWithOpenGLDlg : public CDialog
{
// 构造
public:
	CGMVideoPlayWithOpenGLDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_GMVIDEOPLAYWITHOPENGL_DIALOG };

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
	GxxGmDecoderPlay decoder;
	CEdit m_cUrl;
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnDemuxing();
	afx_msg void OnBnClickedBtnPlay();
	afx_msg void OnBnClickedBtnStop();
};
