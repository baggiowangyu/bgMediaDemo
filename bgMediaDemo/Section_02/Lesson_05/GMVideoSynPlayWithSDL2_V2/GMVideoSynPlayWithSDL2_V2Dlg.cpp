
// GMVideoSynPlayWithSDL2_V2Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "GMVideoSynPlayWithSDL2_V2.h"
#include "GMVideoSynPlayWithSDL2_V2Dlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int audio_len = 0;
unsigned char *audio_pos = NULL;
unsigned char *audio_chunk = NULL;

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


// CGMVideoSynPlayWithSDL2_V2Dlg 对话框




CGMVideoSynPlayWithSDL2_V2Dlg::CGMVideoSynPlayWithSDL2_V2Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGMVideoSynPlayWithSDL2_V2Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	input_fmtctx_ = NULL;

	video_stream_index_ = -1;
	audio_stream_index_ = -1;
	subtitle_stream_index_ = -1;

	// 媒体流
	video_stream_ = NULL;
	audio_stream_ = NULL;
	subtitle_stream_ = NULL;

	// 编解码上下文
	video_codec_ctx_ = NULL;
	audio_codec_ctx_ = NULL;
	subtitle_codec_ctx_ = NULL;

	// 解码器
	video_codec_ = NULL;
	audio_codec_ = NULL;
	subtitle_codec_ = NULL;

	image_convert_context_ = NULL;
}

void CGMVideoSynPlayWithSDL2_V2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_URL, m_cUrl);
	DDX_Control(pDX, IDC_EDIT_INFO, m_cInfo);
	DDX_Control(pDX, IDC_CHECK_VOLUME, m_cVolumeSwitch);
	DDX_Control(pDX, IDC_SLIDER_VOLUME, m_cVolumeValue);
	DDX_Control(pDX, IDC_PROGRESS_TIME, m_cProgressTime);
}

BEGIN_MESSAGE_MAP(CGMVideoSynPlayWithSDL2_V2Dlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_DEMUXING, &CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnDemuxing)
	ON_BN_CLICKED(IDC_BTN_PLAY, &CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnPlay)
	ON_BN_CLICKED(IDC_BTN_PAUSE, &CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnPause)
	ON_BN_CLICKED(IDC_BTN_RESUME, &CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnResume)
	ON_BN_CLICKED(IDC_BTN_STOP, &CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_BROWSE, &CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnBrowse)
	ON_BN_CLICKED(IDC_CHECK_VOLUME, &CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedCheckVolume)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CGMVideoSynPlayWithSDL2_V2Dlg 消息处理程序

BOOL CGMVideoSynPlayWithSDL2_V2Dlg::OnInitDialog()
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
	av_register_all();
	avcodec_register_all();
	avformat_network_init();

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

	// 获取屏幕宽高
	CWnd *pcwnd = GetDlgItem(IDC_STATIC_SCREEN);
	RECT rect;
	pcwnd->GetClientRect(&rect);
	screen_width_ = rect.right - rect.left;
	screen_height_ = rect.bottom - rect.top;

	screen_ = SDL_CreateWindowFrom(pcwnd->GetSafeHwnd());
	screen_renderer_ = SDL_CreateRenderer(screen_, -1, SDL_RendererFlags::SDL_RENDERER_ACCELERATED);

	// 设置音量值为100%
	m_cVolumeValue.SetRange(0, 128);
	m_cVolumeValue.SetPos(128);
	volume_val_ = 128;
	volume_store_ = 0;
	m_cVolumeSwitch.SetCheck(1);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CGMVideoSynPlayWithSDL2_V2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CGMVideoSynPlayWithSDL2_V2Dlg::OnPaint()
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
HCURSOR CGMVideoSynPlayWithSDL2_V2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CGMVideoSynPlayWithSDL2_V2Dlg::SetInfo(const char *info)
{
	CString i;
	m_cInfo.GetWindowText(i);

	if (!i.IsEmpty())
		i += _T("\r\n");
	
	USES_CONVERSION;
	i.Append(A2T(info));

	m_cInfo.SetWindowText(i);
}


void CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnBrowse()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("AVI files (*.avi)|*.avi|MP4 files|*.mp4||"), this);
	INT_PTR ret = fileDlg.DoModal();
	if (ret == IDOK)
	{
		CString url = fileDlg.GetPathName();
		m_cUrl.SetWindowText(url);
	}
}

void CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnDemuxing()
{
	char msg[4096] = {0};

	USES_CONVERSION;
	CString url;
	m_cUrl.GetWindowText(url);

	int errCode = avformat_open_input(&input_fmtctx_, T2A(url.GetBuffer(0)), NULL, NULL);
	if (errCode < 0)
	{
		sprintf_s(msg, 4096, "打开输入文件失败！错误码：", errCode);
		SetInfo(msg);
		return ;
	}

	avformat_find_stream_info(input_fmtctx_, NULL);

	// 获取视频时长
	if(input_fmtctx_->duration != AV_NOPTS_VALUE){
		int hours, mins, secs, us;
		int64_t duration = input_fmtctx_->duration + 5000;
		secs = duration / AV_TIME_BASE;
		us = duration % AV_TIME_BASE;
		mins = secs / 60;
		secs %= 60;
		hours = mins/ 60;
		mins %= 60;

		CWnd *pcwnd = GetDlgItem(IDC_STATIC_TOTAL_TIME);
		sprintf_s(msg, 4096, "%02d:%02d:%02d", hours, mins, secs);
		pcwnd->SetWindowText(A2T(msg));

		sprintf_s(msg, 4096, "视频总时长：%02d:%02d:%02d.%02d\n", hours, mins, secs, (100 * us) / AV_TIME_BASE);
		SetInfo(msg);
	}

	// 设置进度条
	m_cProgressTime.SetRange32(0, (input_fmtctx_->duration + 5000) / 1000);

	for (int index = 0; index < input_fmtctx_->nb_streams; ++index)
	{
		if (input_fmtctx_->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index_ = index;
			video_stream_ = input_fmtctx_->streams[index];
			video_codec_ = avcodec_find_decoder(video_stream_->codec->codec_id);
			if (video_codec_ == NULL)
			{
				// 没有找到对应的解码器
				SetInfo("没有找到视频解码器！");
				continue;
			}

			// 这里上下文确实需要复制一份，如果我们提前关闭了input_fmtctx_，上下文就不可用了
			video_codec_ctx_ = avcodec_alloc_context3(audio_codec_);
			errCode = avcodec_copy_context(video_codec_ctx_, input_fmtctx_->streams[index]->codec);
			if (errCode < 0)
			{
				SetInfo("准备音频解码器失败！");
				continue;
			}

			errCode = avcodec_open2(video_codec_ctx_, video_codec_, NULL);
			if (errCode < 0)
			{
				// 打开解码器失败
				SetInfo("打开视频解码器失败！");
				continue;
			}

			framerate_ = (float)video_stream_->avg_frame_rate.num / (float)video_stream_->avg_frame_rate.den;

			// 输出视频参数
			sprintf_s(msg, 4096, "视频参数：");
			SetInfo(msg);
			sprintf_s(msg, 4096, "编码器：%d(%s)", video_codec_ctx_->codec_id, avcodec_get_name(video_codec_ctx_->codec_id));
			SetInfo(msg);
			sprintf_s(msg, 4096, "视频分辨率：%d*%d", video_codec_ctx_->width, video_codec_ctx_->height);
			SetInfo(msg);
			sprintf_s(msg, 4096, "码率（比特率）：%dHz", video_codec_ctx_->bit_rate);
			SetInfo(msg);
			sprintf_s(msg, 4096, "视频帧率：%f fps", framerate_);
			SetInfo(msg);
			sprintf_s(msg, 4096, "视频流时长：%I64d us", (input_fmtctx_->duration + 5000) / 1000);
			SetInfo(msg);
			SetInfo("\r\n");
		}
		else if (input_fmtctx_->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index_ = index;
			audio_stream_ = input_fmtctx_->streams[index];
			audio_codec_ = avcodec_find_decoder(audio_stream_->codec->codec_id);
			if (audio_codec_ == NULL)
			{
				// 没有找到对应的解码器
				SetInfo("没有找到音频解码器！");
				continue;
			}

			// 这里上下文确实需要复制一份，如果我们提前关闭了input_fmtctx_，上下文就不可用了
			audio_codec_ctx_ = avcodec_alloc_context3(audio_codec_);
			errCode = avcodec_copy_context(audio_codec_ctx_, input_fmtctx_->streams[index]->codec);
			if (errCode < 0)
			{
				SetInfo("准备音频解码器失败！");
				continue;
			}

			errCode = avcodec_open2(audio_codec_ctx_, audio_codec_, NULL);
			if (errCode < 0)
			{
				// 打开解码器失败
				SetInfo("打开音频解码器失败！");
				continue;
			}

			// 输出音频参数
			sprintf_s(msg, 4096, "音频参数：");
			SetInfo(msg);
			sprintf_s(msg, 4096, "编码器：%d(%s)", audio_codec_ctx_->codec_id, avcodec_get_name(audio_codec_ctx_->codec_id));
			SetInfo(msg);
			sprintf_s(msg, 4096, "采样率：%dHz", audio_codec_ctx_->sample_rate);
			SetInfo(msg);
			sprintf_s(msg, 4096, "码率（比特率）：%dbps", audio_codec_ctx_->bit_rate);
			SetInfo(msg);
			sprintf_s(msg, 4096, "声道：%d 声道", audio_codec_ctx_->channels);
			SetInfo(msg);
			sprintf_s(msg, 4096, "音频流时长：%f fps", framerate_);
			SetInfo(msg);
			SetInfo("\r\n");
		}
		else if (input_fmtctx_->streams[index]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
		{
			subtitle_stream_index_ = index;
			subtitle_stream_ = input_fmtctx_->streams[index];
			subtitle_codec_ctx_ = input_fmtctx_->streams[index]->codec;
			subtitle_codec_ = avcodec_find_decoder(subtitle_codec_ctx_->codec_id);
			if (subtitle_codec_ == NULL)
			{
				// 没有找到对应的解码器
				SetInfo("没有找到文本解码器！");
				continue;
			}

			errCode = avcodec_open2(subtitle_codec_ctx_, subtitle_codec_, NULL);
			if (errCode < 0)
			{
				// 打开解码器失败
				SetInfo("打开文本解码器失败！");
				continue;
			}
		}
	}

	// 汇总信息，输出
}

void CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnPlay()
{
	current_video_pts_ = 0;
	framerate_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	pause_event_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	SetEvent(pause_event_);
	is_paused_ = false;

	// 启动帧率控制线程
	CreateThread(NULL, 0, VideoFrameRateControlThread, this, 0, NULL);
	Sleep(1);

	// 启动视频解码渲染线程
	CreateThread(NULL, 0, VideoDecodeThread, this, 0, NULL);
	Sleep(1);

	// 启动音频解码播放线程
	CreateThread(NULL, 0, AudioDecodeThread, this, 0, NULL);
	Sleep(1);

	// 根据解复用信息，准备好视频图像转换、音频编码转换的前期工作

	//////////////////////////////////////////////////////////////////////////
	// 视频前期准备
	video_frame_yuv_ = av_frame_alloc();

	int video_frame_yuv_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
		video_codec_ctx_->width,
		video_codec_ctx_->height,
		1);

	unsigned char *video_frame_yuv_buffer = (unsigned char *)av_malloc(video_frame_yuv_buffer_size);

	av_image_fill_arrays(video_frame_yuv_->data,
		video_frame_yuv_->linesize,
		video_frame_yuv_buffer,
		AV_PIX_FMT_YUV420P,
		video_codec_ctx_->width,
		video_codec_ctx_->height,
		1);

	image_convert_context_ = sws_getContext(video_codec_ctx_->width,
		video_codec_ctx_->height,
		video_codec_ctx_->pix_fmt,
		video_codec_ctx_->width,
		video_codec_ctx_->height,
		AV_PIX_FMT_YUV420P,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL);

	screen_texture_ = SDL_CreateTexture(screen_renderer_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video_codec_ctx_->width, video_codec_ctx_->height);

	total_played_duration_ = 0;

	//////////////////////////////////////////////////////////////////////////
	// 音频前期准备
	uint64_t output_channel_layout = AV_CH_LAYOUT_STEREO;
	int output_frame_size = audio_codec_ctx_->frame_size;	// AAC是1024；MP3是1152；
	AVSampleFormat output_sample_format = AV_SAMPLE_FMT_S16;
	int output_sample_rate = audio_codec_ctx_->sample_rate;	// 44100
	int output_channels = av_get_channel_layout_nb_channels(output_channel_layout);
	audio_output_buffer_size_ = av_samples_get_buffer_size(NULL, output_channels, output_frame_size, output_sample_format, 1);

	audio_output_buffer_ = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	int input_channel_layout = av_get_default_channel_layout(audio_codec_ctx_->channels);

	audio_convert_context_ = swr_alloc();
	audio_convert_context_ = swr_alloc_set_opts(audio_convert_context_,
		output_channel_layout,
		output_sample_format,
		output_sample_rate,
		input_channel_layout,
		audio_codec_ctx_->sample_fmt,
		audio_codec_ctx_->sample_rate,
		0,
		NULL);
	swr_init(audio_convert_context_);

	// 这里根据前面解复用得到的音频信息，专门对SDL音频处理进行配置
	SDL_AudioSpec audio_spec;
	audio_spec.freq = output_sample_rate;
	audio_spec.format = AUDIO_S16SYS;
	audio_spec.channels = output_channels;
	audio_spec.silence = 0;
	audio_spec.samples = output_frame_size;
	audio_spec.callback = audio_fill_callback;
	audio_spec.userdata = this;

	// SDL打开音频通道
	int errCode = SDL_OpenAudio(&audio_spec, NULL);
	if (errCode < 0)
	{
		printf("Can't open audio...\n");
		return ;
	}

	// Play
	SDL_PauseAudio(0);

	// 启动线程，读取视音频编码帧，扔到缓冲队列中
	CreateThread(NULL, 0, WorkingThread, this, 0, NULL);
	Sleep(1);
}

void CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnPause()
{
	// TODO: 在此添加控件通知处理程序代码
	is_paused_ = true;
	SDL_PauseAudio(1);
}

void CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnResume()
{
	// TODO: 在此添加控件通知处理程序代码
	is_paused_ = false;
	SDL_PauseAudio(0);
}

void CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedBtnStop()
{
	// 首先暂停，然后清空播放队列，然后恢复播放，最后触发等待停止线程跳出
	// 清理资源，停止
	OnBnClickedBtnPause();
	video_packet_queue_.cleanup();
	audio_packet_queue_.cleanup();
	subtitle_packet_queue_.cleanup();

	// 结束掉几个主要线程

	//OnBnClickedBtnResume();
}

//////////////////////////////////////////////////////////////////////////
//
DWORD WINAPI CGMVideoSynPlayWithSDL2_V2Dlg::VideoFrameRateControlThread(LPVOID lpParam)
{
	CGMVideoSynPlayWithSDL2_V2Dlg *dlg = (CGMVideoSynPlayWithSDL2_V2Dlg*)lpParam;

	while (true)
	{
		// 这里等待控制事件的到来
		Sleep(1000);
	}

	return 0;
}

DWORD WINAPI CGMVideoSynPlayWithSDL2_V2Dlg::VideoDecodeThread(LPVOID lpParam)
{
	int errCode = 0;
	char msg[4096] = {0};
	CGMVideoSynPlayWithSDL2_V2Dlg *dlg = (CGMVideoSynPlayWithSDL2_V2Dlg*)lpParam;
	dlg->SetInfo("已启动视频解码渲染线程...");

	// 根据帧率计算平均等待事件
	int wait_time = 1000 / dlg->framerate_;

	// 微秒时间戳
	AVRational ms_base;
	ms_base.num = 1;
	ms_base.den = 1000;

	int64_t played_duration = 0;
	while (true)
	{
		// 这里等待帧率控制事件
		SDL_Delay(wait_time);
		//DWORD dwRet = WaitForSingleObject(dlg->framerate_event_, wait_time);
		//if (dwRet != WAIT_TIMEOUT)
		//{
		//	continue;
		//}

		// 这里等待暂停事件
		//dwRet = WaitForSingleObject(dlg->pause_event_, INFINITE);
		while (true)
		{
			if (dlg->is_paused_)
				Sleep(1);
			else
				break;
		}

		// 从视频编码帧队列中取出编码帧，解码，转换，渲染
		AVFrame *frame = av_frame_alloc();
		int data_size = 0;
		AVPacket *pkt = av_packet_alloc();
		//av_init_packet(&pkt);

		// 取队列，如果没有好像是会阻塞的吧
		bool b = dlg->video_packet_queue_.pop(&pkt, true);
		if (!b)
		{
			errCode = -1;
			break;
		}

		// 解码
		int got_pic = 0;
		errCode = avcodec_decode_video2(dlg->video_codec_ctx_, frame, &got_pic, pkt);
		if (errCode < 0)
		{
			av_make_error_string(msg, AV_ERROR_MAX_STRING_SIZE, errCode);
			//break;
			continue;
		}

		if (got_pic == 0)
			continue;

		// 视音频同步使用的时间戳
		dlg->current_video_pts_ = av_rescale_q(frame->pkt_pts, dlg->video_stream_->time_base, ms_base);

		// 得到当前的pts，要注意时间基准
		// 最终最好还是转换成秒
		played_duration += frame->pkt_duration;
		int64_t ms_pts = av_rescale_q(played_duration, dlg->video_stream_->time_base, ms_base);
		dlg->total_played_duration_ = (ms_pts);
		TRACE("已播放时长：%d ms\n", dlg->total_played_duration_);
		dlg->m_cProgressTime.SetPos(dlg->total_played_duration_);


		// 转换图像
		sws_scale(dlg->image_convert_context_, 
			(const unsigned char* const*)frame->data,
			frame->linesize,
			0,
			dlg->video_codec_ctx_->height,
			dlg->video_frame_yuv_->data,
			dlg->video_frame_yuv_->linesize);

		// 渲染显示
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = dlg->screen_width_;
		rect.h = dlg->screen_height_;

		SDL_UpdateTexture(dlg->screen_texture_, NULL, dlg->video_frame_yuv_->data[0], dlg->video_frame_yuv_->linesize[0]);
		SDL_RenderCopy(dlg->screen_renderer_, dlg->screen_texture_, NULL, &rect);
		SDL_RenderPresent(dlg->screen_renderer_);

		av_frame_free(&frame);
		av_packet_unref(pkt);
		av_free_packet(pkt);
	}

	avcodec_close(dlg->video_codec_ctx_);
	dlg->video_codec_ctx_ = NULL;
	dlg->SetInfo("视频解码渲染线程准备退出...");
	return errCode;
}

DWORD WINAPI CGMVideoSynPlayWithSDL2_V2Dlg::AudioDecodeThread(LPVOID lpParam)
{
	int errCode = 0;
	char msg[4096] = {0};
	CGMVideoSynPlayWithSDL2_V2Dlg *dlg = (CGMVideoSynPlayWithSDL2_V2Dlg*)lpParam;
	dlg->SetInfo("已启动音频解码渲染线程...");

	// 微秒时间戳
	AVRational ms_base;
	ms_base.num = 1;
	ms_base.den = 1000;

	while (true)
	{
		// 从音频编码帧队列中取出编码帧，解码，转换，播放
		AVFrame *frame = av_frame_alloc();
		int data_size = 0;
		AVPacket *pkt = av_packet_alloc();

		// 取队列，如果没有好像是会阻塞的吧
		bool b = dlg->audio_packet_queue_.pop(&pkt, true);
		if (!b)
		{
			errCode = -1;
			break;
		}

		// 如果有视频流的时候这里需要匹配一下视频帧的pts
		while (true)
		{
			if (dlg->current_video_pts_ <= av_rescale_q(pkt->pts, dlg->video_stream_->time_base, ms_base))
			{
				TRACE("音频帧的时间戳超越视频帧了，等待一下...\n");
				SDL_Delay(1);
			}
			else
				break;
		}

		// 解码
		int got_sound = 0;
		errCode = avcodec_decode_audio4(dlg->audio_codec_ctx_, frame, &got_sound, pkt);
		if (errCode < 0)
		{
			av_make_error_string(msg, AV_ERROR_MAX_STRING_SIZE, errCode);
			continue;
		}

		if (got_sound == 0)
			continue;

		// 执行转换
		swr_convert(dlg->audio_convert_context_, &dlg->audio_output_buffer_, MAX_AUDIO_FRAME_SIZE, (const unsigned char **)frame->data, frame->nb_samples);

		while (audio_len > 0)
			SDL_Delay(1);

		audio_chunk = (unsigned char *)dlg->audio_output_buffer_;
		audio_len = dlg->audio_output_buffer_size_;
		audio_pos = audio_chunk;

		av_frame_free(&frame);
		av_packet_unref(pkt);
		av_packet_free(&pkt);
	}

	avcodec_close(dlg->audio_codec_ctx_);
	dlg->audio_codec_ctx_ = NULL;
	dlg->SetInfo("音频解码渲染线程准备退出...");
	return 0;
}

DWORD WINAPI CGMVideoSynPlayWithSDL2_V2Dlg::WorkingThread(LPVOID lpParam)
{
	CGMVideoSynPlayWithSDL2_V2Dlg *dlg = (CGMVideoSynPlayWithSDL2_V2Dlg*)lpParam;

	while(true)
	{
		// 这里等待暂停事件
		//DWORD dwRet = WaitForSingleObject(dlg->pause_event_, INFINITE);

		AVPacket pkt;
		int errCode = av_read_frame(dlg->input_fmtctx_, &pkt);
		if (errCode < 0)
		{
			break;
		}

		bool b = false;
		if (pkt.stream_index == dlg->video_stream_index_)
		{
			// 将视频编码包扔进视频队列
			b = dlg->video_packet_queue_.push(&pkt);
		}
		else if (pkt.stream_index == dlg->audio_stream_index_)
		{
			// 将音频编码包扔进音频队列
			b = dlg->audio_packet_queue_.push(&pkt);
		}
		else if (pkt.stream_index == dlg->subtitle_stream_index_)
		{
			//b = dlg->subtitle_packet_queue_.push(&pkt);
		}

		if (!b)
		{
			// 编码包加入队列失败
			dlg->SetInfo("编码包加入队列失败...");
		}

		av_free_packet(&pkt);
	}

	// 到这里为止，应该是所有数据包都读取完毕了
	//avformat_close_input(&dlg->input_fmtctx_);
	//dlg->input_fmtctx_ = NULL;
	TRACE("视音频流分离线程工作完成...\n");

	return 0;
}

void SDLCALL CGMVideoSynPlayWithSDL2_V2Dlg::audio_fill_callback(void *userdata, Uint8 * stream, int len)
{
	CGMVideoSynPlayWithSDL2_V2Dlg *dlg = (CGMVideoSynPlayWithSDL2_V2Dlg*)userdata;

	if (len <= 0)
		return ;

	SDL_memset(stream, 0, len);

	if(audio_len <= 0)
		return;

	len = (len > audio_len ? audio_len : len);

	SDL_MixAudio(stream, audio_pos, len, dlg->volume_val_);
	audio_pos += len;
	audio_len -= len;
}



void CGMVideoSynPlayWithSDL2_V2Dlg::OnBnClickedCheckVolume()
{
	if (m_cVolumeSwitch.GetCheck())
	{
		// 被选中，开启声音
		volume_val_ = volume_store_;
		sound_switch_ = true;

		m_cVolumeValue.SetPos(volume_val_);
	}
	else
	{
		// 取消选中，关闭声音，缓存音量
		sound_switch_ = false;
		volume_store_ = volume_val_;
		volume_val_ = 0;

		m_cVolumeValue.SetPos(0);
	}
}

void CGMVideoSynPlayWithSDL2_V2Dlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	volume_val_ = m_cVolumeValue.GetPos();

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}
