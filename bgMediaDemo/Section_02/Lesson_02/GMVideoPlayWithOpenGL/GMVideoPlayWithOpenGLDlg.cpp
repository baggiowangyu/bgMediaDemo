
// GMVideoPlayWithOpenGLDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "GMVideoPlayWithOpenGL.h"
#include "GMVideoPlayWithOpenGLDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CGMVideoPlayWithOpenGLDlg 对话框




CGMVideoPlayWithOpenGLDlg::CGMVideoPlayWithOpenGLDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGMVideoPlayWithOpenGLDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGMVideoPlayWithOpenGLDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_URL, m_cUrl);
}

BEGIN_MESSAGE_MAP(CGMVideoPlayWithOpenGLDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_BROWSE, &CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnBrowse)
	ON_BN_CLICKED(IDC_BTN_DEMUXING, &CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnDemuxing)
	ON_BN_CLICKED(IDC_BTN_PLAY, &CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnPlay)
	ON_BN_CLICKED(IDC_BTN_STOP, &CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnStop)
END_MESSAGE_MAP()


// CGMVideoPlayWithOpenGLDlg 消息处理程序

BOOL CGMVideoPlayWithOpenGLDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_cUrl.SetWindowText(_T("C:\\Users\\WANGY\\Desktop\\video\\小哥哥你为什么没有女朋友.mp4"));

	CWnd *pcwnd = GetDlgItem(IDC_STATIC_SCREEN);
	decoder.Initialize((int)pcwnd->GetSafeHwnd());

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CGMVideoPlayWithOpenGLDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CGMVideoPlayWithOpenGLDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CGMVideoPlayWithOpenGLDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnBrowse()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("MP4 File (*.mp4)|*.mp4||"), this);
	INT_PTR ret = dlg.DoModal();

	if (ret == IDOK)
	{
		CString url = dlg.GetPathName();
		m_cUrl.SetWindowText(url);
	}
}

void CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnDemuxing()
{
	USES_CONVERSION;
	CString str_url;
	m_cUrl.GetWindowText(str_url);

	// 解复用
	int errCode = decoder.Demuxing(T2A(str_url.GetBuffer(0)));
	if (errCode != 0)
		TRACE("解复用失败！\n");
}

void CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnPlay()
{
	int errCode = decoder.Play();
}

void CGMVideoPlayWithOpenGLDlg::OnBnClickedBtnStop()
{
	decoder.Stop();
}
