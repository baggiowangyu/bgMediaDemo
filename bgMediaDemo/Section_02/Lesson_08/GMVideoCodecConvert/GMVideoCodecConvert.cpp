// GMVideoCodecConvert.cpp : 定义控制台应用程序的入口点。
//
// 本范例将H264-flv文件重编码为H265-MP4文件

#include "stdafx.h"

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libpostproc/postprocess.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
};
#endif

#include <iostream>
#include <atlconv.h>

typedef struct _StreamContext
{
	AVCodecContext *decode_context;
	AVCodecContext *encode_context;
} StreamContext;

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		std::cout<<"usage: GMVideoFormatConvert.exe <input_media> <output_media>"<<std::endl;
		return 0;
	}

	TCHAR input_media[4096] = {0};
	TCHAR output_media[4096] = {0};

	_tcscpy_s(input_media, 4096, argv[1]);
	_tcscpy_s(output_media, 4096, argv[2]);

	av_register_all();
	avformat_network_init();
	avcodec_register_all();
	avfilter_register_all();

	USES_CONVERSION;

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFormatContext *input_format_context = NULL;
	int errCode = avformat_open_input(&input_format_context, T2A(input_media), NULL, NULL);
	if (errCode < 0)
	{
		printf("Open source media file failed..%d\n", errCode);
		return errCode;
	}

	errCode = avformat_find_stream_info(input_format_context, NULL);
	if (errCode < 0)
	{
		printf("Find source media file's streams failed..%d\n", errCode);
		return errCode;
	}

	StreamContext *stream_ctx = av_mallocz_array(input_format_context->nb_streams, sizeof(*stream_ctx));
	if (!stream_ctx)
	{
		return -1;
	}

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件
	//
	//////////////////////////////////////////////////////////////////////////

	return 0;
}

