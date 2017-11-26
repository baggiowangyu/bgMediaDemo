// GMVideoMux.cpp : 定义控制台应用程序的入口点。
//
// 本范例将h264文件和aac文件封装为带音轨的MP4文件

#include "stdafx.h"

#include <atlconv.h>

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
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

//#define USE_H264BSF
//#define USE_AACBSF


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 4)
	{
		printf("GMVideoMux.exe <video_url> <audio_url> <target_url>\n");
		return 0;
	}

	TCHAR video_url[4096] = {0};
	_tcscpy_s(video_url, 4096, argv[1]);

	TCHAR audio_url[4096] = {0};
	_tcscpy_s(audio_url, 4096, argv[2]);

	TCHAR target_url[4096] = {0};
	_tcscpy_s(target_url, 4096, argv[3]);

	USES_CONVERSION;

	av_register_all();

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入文件
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *input_video_format_context = NULL;
	AVFormatContext *input_audio_format_context = NULL;

	int errCode = avformat_open_input(&input_video_format_context, T2A(video_url), NULL, NULL);
	if (errCode < 0)
		return errCode;

	errCode = avformat_find_stream_info(input_video_format_context, NULL);
	if (errCode < 0)
		return errCode;

	errCode = avformat_open_input(&input_audio_format_context, T2A(audio_url), NULL, NULL);
	if (errCode < 0)
		return errCode;

	errCode = avformat_find_stream_info(input_video_format_context, NULL);
	if (errCode < 0)
		return errCode;

	//////////////////////////////////////////////////////////////////////////
	//
	// 创建输出上下文
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *output_format_context = NULL;
	errCode = avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(target_url));
	if (!output_format_context)
		return errCode;

	AVOutputFormat *output_format = output_format_context->oformat;

	//////////////////////////////////////////////////////////////////////////
	//
	// 分析输入的视音频流
	//
	//////////////////////////////////////////////////////////////////////////

	int input_video_stream_index = -1;
	int output_video_stream_index = -1;

	for (int index = 0; index < input_video_format_context->nb_streams; ++index)
	{
		if (input_video_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVStream *in_stream = input_video_format_context->streams[index];
			AVStream *out_stream = avformat_new_stream(output_format_context, in_stream->codec->codec);

			input_video_stream_index = index;

			if (!out_stream)
				return AVERROR_UNKNOWN;

			output_video_stream_index = out_stream->index;

			// 复制编码上下文
			errCode = avcodec_copy_context(out_stream->codec, in_stream->codec);
			if (errCode < 0)
				return errCode;

			out_stream->codec->codec_tag = 0;

			if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	int input_audio_stream_index = -1;
	int output_audio_stream_index = -1;

	for (int index = 0; index < input_audio_format_context->nb_streams; ++index)
	{
		if (input_audio_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			AVStream *in_stream = input_audio_format_context->streams[index];
			AVStream *out_stream = avformat_new_stream(output_format_context, in_stream->codec->codec);

			input_audio_stream_index = index;

			if (!out_stream)
				return AVERROR_UNKNOWN;

			output_audio_stream_index = out_stream->index;

			// 复制编码上下文
			errCode = avcodec_copy_context(out_stream->codec, in_stream->codec);
			if (errCode < 0)
				return errCode;

			out_stream->codec->codec_tag = 0;

			if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件，写入文件头
	//
	//////////////////////////////////////////////////////////////////////////

	if (!(output_format_context->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_format_context->pb, T2A(target_url), AVIO_FLAG_WRITE);
		if (errCode < 0)
			return errCode;
	}

	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
		return errCode;

	//////////////////////////////////////////////////////////////////////////
	//
	// 
	//
	//////////////////////////////////////////////////////////////////////////

#ifdef USE_H264BSF
	AVBitStreamFilterContext *h264_bit_stream_filter_context = av_bitstream_filter_init("h264_mp4toannexb");
#endif

#ifdef USE_AACBSF
	AVBitStreamFilterContext *aac_bit_stream_filter_context = av_bitstream_filter_init("aac_adtstoasc");
#endif

	while (true)
	{

	}

	_CrtDumpMemoryLeaks();
	return 0;
}


