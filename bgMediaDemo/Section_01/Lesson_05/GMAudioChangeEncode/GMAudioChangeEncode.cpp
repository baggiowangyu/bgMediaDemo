// GMAudioChangeEncode.cpp : 定义控制台应用程序的入口点。
//
// 本范例用于将wav录音文件转码为mp3或aac录音文件

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
#ifdef __cplusplus
};
#endif

#include <iostream>
#include <atlconv.h>

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		printf("GMAudioChangeEncode.exe <source_url> <target_url> \n");
		return 0;
	}

	int errCode = 0;

	// 完成初始化注册
	av_register_all();
	avformat_network_init();

	USES_CONVERSION;

	TCHAR source_url[4096] = {0};
	_tcscpy_s(source_url, 4096, argv[1]);

	TCHAR target_url[4096] = {0};
	_tcscpy_s(target_url, 4096, argv[2]);

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入文件
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *input_format_context = NULL;
	errCode = avformat_open_input(&input_format_context, T2A(source_url), NULL, NULL);
	if (errCode < 0)
		return errCode;

	errCode = avformat_find_stream_info(input_format_context, NULL);
	if (errCode < 0)
		return errCode;

	return 0;
}

