// GMMediaPusher.cpp : 定义控制台应用程序的入口点。
//
// 此范例用于将文件以rtsp协议推送到流媒体服务器

#include "stdafx.h"

#include <atlconv.h>

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/rational.h"
#include "libavutil/pixdesc.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
};
#endif

#include <iostream>


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		printf("usage : GMMediaPusher.exe <source_url> <stream_server_url>\n");
		return 0;
	}

	TCHAR source_url[4096] = {0};
	_tcscpy_s(source_url, 4096, argv[1]);

	TCHAR stream_server_url[4096] = {0};
	_tcscpy_s(stream_server_url, 4096, argv[2]);

	USES_CONVERSION;

	av_register_all();
	avformat_network_init();

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入的文件
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *input_format_context = NULL;
	int errCode = avformat_open_input(&input_format_context, T2A(source_url), NULL, NULL);
	if (errCode < 0)
	{
		printf("Open input media file failed...%d\n", errCode);
		return errCode;
	}

	errCode = avformat_find_stream_info(input_format_context, NULL);
	if (errCode < 0)
	{
		printf("input media has no media info...\n");
		return errCode;
	}

	int input_video_index = -1;
	int input_audio_index = -1;
	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			input_video_index = index;
		}
		else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			input_audio_index = index;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备输出上下文，格式采用输入格式
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *output_format_context = NULL;
	errCode = avformat_alloc_output_context2(&output_format_context, NULL, "flv", T2A(stream_server_url));
	if (errCode < 0)
	{
		printf("push")
	}

	return 0;
}

