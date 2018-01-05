
// UACToolDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "UACTool.h"
#include "UACToolDlg.h"

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


// CUACToolDlg 对话框




CUACToolDlg::CUACToolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUACToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUACToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_UAS_IP, m_cUasIp);
	DDX_Control(pDX, IDC_EDIT_UAS_PORT, m_cUasPort);
	DDX_Control(pDX, IDC_EDIT_UAS_ID, m_cUasCode);
	DDX_Control(pDX, IDC_EDIT_UAC_IP, m_cUacIp);
	DDX_Control(pDX, IDC_EDIT_UAC_PORT, m_cUacPort);
	DDX_Control(pDX, IDC_EDIT_UAC_ID, m_cUacCode);
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_cMessage);
	DDX_Control(pDX, IDC_LIST_INFO, m_cInfo);
}

BEGIN_MESSAGE_MAP(CUACToolDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	//ON_EN_CHANGE(IDC_EDIT2, &CUACToolDlg::OnEnChangeEdit2)
	ON_BN_CLICKED(IDC_BTN_START, &CUACToolDlg::OnBnClickedBtnStart)
	ON_BN_CLICKED(IDC_BTN_REGISTER, &CUACToolDlg::OnBnClickedBtnRegister)
	ON_BN_CLICKED(IDC_BTN_UNREGISTER, &CUACToolDlg::OnBnClickedBtnUnregister)
	ON_BN_CLICKED(IDC_BTN_CALL, &CUACToolDlg::OnBnClickedBtnCall)
	ON_BN_CLICKED(IDC_BTN_CLOSE_CALL, &CUACToolDlg::OnBnClickedBtnCloseCall)
	ON_BN_CLICKED(IDC_BTN_SEND_MESSAGE, &CUACToolDlg::OnBnClickedBtnSendMessage)
END_MESSAGE_MAP()


// CUACToolDlg 消息处理程序

BOOL CUACToolDlg::OnInitDialog()
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

	m_cUacIp.SetWindowText(_T("127.0.0.1"));
	m_cUacPort.SetWindowText(_T("5061"));
	m_cUacCode.SetWindowText(_T("44011200002110000002"));

	m_cUasIp.LimitText(15);
	m_cUasPort.LimitText(5);
	m_cUasCode.LimitText(20);

	m_cUacIp.LimitText(15);
	m_cUacPort.LimitText(5);
	m_cUacCode.LimitText(20);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CUACToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CUACToolDlg::OnPaint()
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
HCURSOR CUACToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CUACToolDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}

void CUACToolDlg::OnBnClickedBtnStart()
{
	// 先拿相关数据，保存下来
	CString uac_ip, uac_port, uac_code;

	m_cUacIp.GetWindowText(uac_ip);
	m_cUacPort.GetWindowText(uac_port);
	m_cUacCode.GetWindowText(uac_code);

	USES_CONVERSION;
	int errCode = uac_.Init(T2A(uac_code.GetString()), T2A(uac_ip.GetString()), _ttoi(uac_port.GetString()));
	if (errCode != 0)
	{
		m_cInfo.AddString(_T("初始化 UAC 失败！"));
		return ;
	}
	else
		m_cInfo.AddString(_T("初始化 UAC 成功！"));
}

void CUACToolDlg::OnBnClickedBtnRegister()
{
	//
	// 注册
	//

	CString uas_ip, uas_port, uas_code;

	m_cUasIp.GetWindowText(uas_ip);
	m_cUasPort.GetWindowText(uas_port);
	m_cUasCode.GetWindowText(uas_code);

	USES_CONVERSION;
	uac_.SetUASEnvironment(T2A(uas_code.GetString()), T2A(uas_ip.GetString()), _ttoi(uas_port.GetString()));

	m_cInfo.AddString(_T("发送注册请求..."));

	int errCode = uac_.Register();
	if (errCode != 0)
		m_cInfo.AddString(_T("注册失败！"));
	else
		m_cInfo.AddString(_T("注册成功！"));
}

void CUACToolDlg::OnBnClickedBtnUnregister()
{
	//
	// 注销
	//
	m_cInfo.AddString(_T("发送注销请求..."));

	int errCode = uac_.Unregister();
	if (errCode != 0)
		m_cInfo.AddString(_T("注销失败！"));
	else
		m_cInfo.AddString(_T("注销成功！"));
}

void CUACToolDlg::OnBnClickedBtnCall()
{
	//
	// 通话
	//

	m_cInfo.AddString(_T("发送通话请求..."));

	int errCode = uac_.Call();
	if (errCode != 0)
		m_cInfo.AddString(_T("通话失败！"));
	else
		m_cInfo.AddString(_T("通话成功！"));
}

void CUACToolDlg::OnBnClickedBtnCloseCall()
{
	//
	// 挂断
	//

	m_cInfo.AddString(_T("发送挂断请求..."));

	int errCode = uac_.ReleaseCall();
	if (errCode != 0)
		m_cInfo.AddString(_T("挂断失败！"));
	else
		m_cInfo.AddString(_T("挂断成功！"));
}

void CUACToolDlg::OnBnClickedBtnSendMessage()
{
	//
	// 发短消息
	//
	CString uas_ip, uas_port, uas_code;

	m_cUasIp.GetWindowText(uas_ip);
	m_cUasPort.GetWindowText(uas_port);
	m_cUasCode.GetWindowText(uas_code);

	USES_CONVERSION;
	uac_.SetUASEnvironment(T2A(uas_code.GetString()), T2A(uas_ip.GetString()), _ttoi(uas_port.GetString()));

	CString sms;
	m_cMessage.GetWindowText(sms);

	m_cInfo.AddString(_T("发送短消息..."));

	int errCode = uac_.SendSMS(T2A(sms.GetString()));
	if (errCode != 0)
		m_cInfo.AddString(_T("短消息发送失败！"));
	else
		m_cInfo.AddString(_T("短消息发送成功！"));
}
