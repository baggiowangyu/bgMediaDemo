
// UASToolDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "UASTool.h"
#include "UASToolDlg.h"

#include <atlconv.h>

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


// CUASToolDlg 对话框




CUASToolDlg::CUASToolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUASToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUASToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_UAS_IP, m_cUasIp);
	DDX_Control(pDX, IDC_EDIT_UAS_PORT, m_cUasPort);
	DDX_Control(pDX, IDC_EDIT_UAS_CODE, m_cUasCode);
	DDX_Control(pDX, IDC_LIST_INFO, m_cInfo);
}

BEGIN_MESSAGE_MAP(CUASToolDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_START_SERVICE, &CUASToolDlg::OnBnClickedBtnStartService)
	ON_BN_CLICKED(IDC_BTN_STOP_SERVICE2, &CUASToolDlg::OnBnClickedBtnStopService2)
END_MESSAGE_MAP()


// CUASToolDlg 消息处理程序

BOOL CUASToolDlg::OnInitDialog()
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
	m_cUasIp.SetWindowText(_T("127.0.0.1"));
	m_cUasPort.SetWindowText(_T("5060"));
	m_cUasCode.SetWindowText(_T("44011200002110000001"));

	m_cUasIp.LimitText(15);
	m_cUasPort.LimitText(5);
	m_cUasCode.LimitText(20);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CUASToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CUASToolDlg::OnPaint()
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
HCURSOR CUASToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CUASToolDlg::OnBnClickedBtnStartService()
{
	// 先取出服务器参数
	CString ip, port, code;
	m_cUasIp.GetWindowText(ip);
	m_cUasPort.GetWindowText(port);
	m_cUasCode.GetWindowText(code);

	USES_CONVERSION;
	int errCode = uas_.Init(T2A(code.GetString()), T2A(ip.GetString()), _ttoi(port.GetString()));
	if (errCode != 0)
	{
		m_cInfo.AddString(_T("初始化 UAS 失败！"));
		return ;
	}
	else
		m_cInfo.AddString(_T("初始化 UAS 成功！"));

	errCode = uas_.Start();
	if (errCode != 0)
		m_cInfo.AddString(_T("启动 UAS 服务失败！"));
	else
		m_cInfo.AddString(_T("启动 UAS 服务成功！"));
}

void CUASToolDlg::OnBnClickedBtnStopService2()
{
	// TODO: 在此添加控件通知处理程序代码
}