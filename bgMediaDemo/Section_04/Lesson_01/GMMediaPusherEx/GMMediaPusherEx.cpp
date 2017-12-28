// GMMediaPusherEx.cpp : 定义控制台应用程序的入口点。
//
// 源地址：rtmp://live.hkstv.hk.lxdns.com/live/hks
//

#include "stdafx.h"
#include "GMStreamGenerator.h"
#include "GMStreamPusherEx.h"

#include <iostream>
#include <Windows.h>
#include <atlconv.h>

class GMWorking : public GMStreamGenteratorNotifer
{
public:
	GMWorking() {}
	~GMWorking() {}

public:
	int OnInit() { return 0; }
	void OnClose() { }

	int OnOpen(const char *target_url)
	{
		return pusher_.OpenTargetUrl(target_url);
	}

	int OnSetVideoParam(int width, int height, int frame_rate, int video_codec, int color_fmt)
	{
		return pusher_.SetVideoParam(width, height, frame_rate, video_codec, color_fmt);
	}

	int OnSetAudioParam(int bit_rate, int sample_rate, int sample_fmt, int channel_layout, int audio_codec)
	{
		return pusher_.SetAudioParam(bit_rate, sample_rate, sample_fmt, channel_layout, audio_codec);
	}

public:
	int OnStart()
	{
		return pusher_.StartPush();
	}

	void OnStop()
	{
		return pusher_.StopPush();
	}

public:
	virtual int StreamDataNotify(unsigned char *data, int data_len, int stream_type, int media_type)
	{
		std::string stream_name = "未知数据";

		switch (stream_type)
		{
		case MEDIA_VIDEO:
			stream_name = "视频数据";
			break;
		case MEDIA_AUDIO:
			stream_name = "音频数据";
			break;
		}

		pusher_.InputMediaData(data, data_len, stream_type);

		std::cout<<stream_name.c_str()<<std::endl;
		return 0;
	}

private:
	GMStreamPusherEx pusher_;
};

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		std::cout<<"usage : GMMediaPusherEx.exe <input_url> <output_url>"<<std::endl;
		return 0;
	}

	USES_CONVERSION;

	TCHAR input_url[4096] = {0};
	_tcscpy_s(input_url, 4096, argv[1]);

	TCHAR output_url[4096] = {0};
	_tcscpy_s(output_url, 4096, argv[2]);

	GMWorking working;
	working.OnOpen(T2A(output_url));
	working.OnSetVideoParam(1920, 1080, 25, MEDIA_CODEC_H264, VIDEO_COLOR_FMT_YUV420);
	working.OnSetAudioParam(64000, 44100, AUDIO_BIT_S16, AUDIO_CHENNAL_LAYOUT_STEREO, MEDIA_CODEC_MP3);

	working.OnStart();

	// 测试流发生器
	GMStreamGenerator stream_gen(&working);
	stream_gen.OpenSource(T2A(input_url), MEDIA_ALL);

	Sleep(10000);

	working.OnStop();

	return 0;
}

