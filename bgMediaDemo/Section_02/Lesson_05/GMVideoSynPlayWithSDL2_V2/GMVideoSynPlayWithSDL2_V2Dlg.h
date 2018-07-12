
// GMVideoSynPlayWithSDL2_V2Dlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <queue>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
};
#endif

#include "SDL.h"
#include "afxcmn.h"

#define MAX_AUDIO_FRAME_SIZE 192000

class PacketQueue
{
public:
	PacketQueue()
	{
		nb_packets_ = 0;
		size_ = 0;

		mutex_ = SDL_CreateMutex();
		cond_ = SDL_CreateCond();
	}

	~PacketQueue()
	{
		SDL_DestroyMutex(mutex_);
		SDL_DestroyCond(cond_);

		mutex_ = NULL;
		cond_ = NULL;

		nb_packets_ = 0;
		size_ = 0;
	}

public:
	std::queue<AVPacket> queue_;

	unsigned int nb_packets_;
	unsigned int size_;
	SDL_mutex *mutex_;
	SDL_cond *cond_;

public:
	bool push(const AVPacket *packet)
	{
		AVPacket pkt;
		int errCode = av_packet_ref(&pkt, packet);
		if (errCode < 0)
		{
			char msg[AV_ERROR_MAX_STRING_SIZE] = {0};
			av_make_error_string(msg, AV_ERROR_MAX_STRING_SIZE, errCode);
			return false;
		}

		SDL_LockMutex(mutex_);
		queue_.push(pkt);

		size_ += pkt.size;
		++nb_packets_;

		SDL_CondSignal(cond_);
		SDL_UnlockMutex(mutex_);

		return true;
	}

	bool pop(AVPacket **packet, bool block)
	{
		bool ret = false;

		SDL_LockMutex(mutex_);

		while (true)
		{
			// 这里有一个全局控制器，标记是否退出
			if (!queue_.empty())
			{
				*packet = av_packet_clone(&queue_.front());
				//int errCode = av_packet_ref(*packet, &queue_.front());
				if (*packet == NULL)
				{
					//char msg[AV_ERROR_MAX_STRING_SIZE] = {0};
					//av_make_error_string(msg, AV_ERROR_MAX_STRING_SIZE, errCode);
					ret = false;
					break;
				}

				queue_.pop();
				--nb_packets_;
				size_ -= (*packet)->size;

				ret = true;
				break;
			}
			else if (!block)
			{
				ret = false;
				break;
			}
			else
				SDL_CondWait(cond_, mutex_);
		}

		SDL_UnlockMutex(mutex_);
		return ret;
	}

	void cleanup()
	{
		// 这里简单粗暴一点，直接清空，不管所谓的锁什么的了
		// 恩...还是要管锁的，万一有哪个混蛋线程来取队列了呢？

		SDL_LockMutex(mutex_);

		while (!queue_.empty())
		{
			queue_.pop();
		}

		SDL_UnlockMutex(mutex_);
	}

};


// CGMVideoSynPlayWithSDL2_V2Dlg 对话框
class CGMVideoSynPlayWithSDL2_V2Dlg : public CDialog
{
// 构造
public:
	CGMVideoSynPlayWithSDL2_V2Dlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_GMVIDEOSYNPLAYWITHSDL2_V2_DIALOG };

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
	int progress_pos_;	// 播放进度
	int total_played_duration_;

	int volume_val_;	// 播放音量
	bool sound_switch_;	// 声音开关
	int volume_store_;	// 关闭声音的时候缓存下来的音量，到时候恢复的时候取出来用

public:
	AVFormatContext *input_fmtctx_;

	// 媒体流索引
	int video_stream_index_;
	int audio_stream_index_;
	int subtitle_stream_index_;

	// 媒体流
	AVStream *video_stream_;
	AVStream *audio_stream_;
	AVStream *subtitle_stream_;

	// 编解码上下文
	AVCodecContext *video_codec_ctx_;
	AVCodecContext *audio_codec_ctx_;
	AVCodecContext *subtitle_codec_ctx_;

	// 解码器
	AVCodec *video_codec_;
	AVCodec *audio_codec_;
	AVCodec *subtitle_codec_;

	// 视音频、文本编码帧队列
	PacketQueue video_packet_queue_;
	PacketQueue audio_packet_queue_;
	PacketQueue subtitle_packet_queue_;

public:
	static DWORD WINAPI VideoFrameRateControlThread(LPVOID lpParam);
	static DWORD WINAPI VideoDecodeThread(LPVOID lpParam);
	static DWORD WINAPI AudioDecodeThread(LPVOID lpParam);
	static DWORD WINAPI WorkingThread(LPVOID lpParam);
	static DWORD WINAPI StopManagerThread(LPVOID lpParam);

public:
	float framerate_;
	AVFrame *video_frame_yuv_;
	struct SwsContext *image_convert_context_;

	SwrContext *audio_convert_context_;
	unsigned char *audio_output_buffer_;
	int audio_output_buffer_size_;

public:
	SDL_Window *screen_;
	SDL_Renderer *screen_renderer_;
	SDL_Texture *screen_texture_;

	int screen_width_;
	int screen_height_;

	__int64 current_video_pts_;
	HANDLE framerate_event_;
	HANDLE pause_event_;
	bool is_paused_;

	static void SDLCALL audio_fill_callback(void *userdata, Uint8 * stream, int len);

public:
	void SetInfo(const char *info);

public:
	HANDLE Handle_VideoFrameRateControlThread_;
	HANDLE Handle_VideoDecodeThread_;
	HANDLE Handle_AudioDecodeThread_;
	HANDLE Handle_WorkingThread_;

public:
	CEdit m_cUrl;
	CEdit m_cInfo;

public:
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnDemuxing();
	afx_msg void OnBnClickedBtnPlay();
	afx_msg void OnBnClickedBtnPause();
	afx_msg void OnBnClickedBtnResume();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnBnClickedCheckVolume();
	
	CButton m_cVolumeSwitch;
	CSliderCtrl m_cVolumeValue;
	
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	CProgressCtrl m_cProgressTime;
};
