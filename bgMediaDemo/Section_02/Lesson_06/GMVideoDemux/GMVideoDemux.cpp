// GMVideoDemux.cpp : 定义控制台应用程序的入口点。
//
// 将输入的flv文件拆分成h264和aac

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

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 4)
	{
		printf("GMVideoDemux.exe <source_url> <video_url> <audio_url>\n");
		return 0;
	}

	TCHAR source_url[4096] = {0};
	_tcscpy_s(source_url, 4096, argv[1]);

	TCHAR video_url[4096] = {0};
	_tcscpy_s(video_url, 4096, argv[2]);

	TCHAR audio_url[4096] = {0};
	_tcscpy_s(audio_url, 4096, argv[3]);

	USES_CONVERSION;

	av_register_all();

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入文件
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *input_format_context = NULL;
	int errCode = avformat_open_input(&input_format_context, T2A(source_url), NULL, NULL);
	if (errCode < 0)
		return errCode;

	errCode = avformat_find_stream_info(input_format_context, NULL);
	if (errCode < 0)
		return errCode;

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备输出格式信息
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *output_video_format_context = NULL;
	AVFormatContext *output_audio_format_context = NULL;

	AVOutputFormat *output_video_format = NULL;
	AVOutputFormat *output_audio_format = NULL;

	avformat_alloc_output_context2(&output_video_format_context, NULL, NULL, T2A(video_url));
	if (!output_video_format_context)
		return AVERROR_UNKNOWN;

	output_video_format = output_video_format_context->oformat;

	avformat_alloc_output_context2(&output_audio_format_context, NULL, NULL, T2A(audio_url));
	if (!output_audio_format_context)
		return AVERROR_UNKNOWN;

	output_audio_format = output_audio_format_context->oformat;

	//////////////////////////////////////////////////////////////////////////
	//
	// 处理输出输出流
	//
	//////////////////////////////////////////////////////////////////////////

	int input_video_stream_index = -1;
	int input_audio_stream_index = -1;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		AVFormatContext *ofmt_ctx = NULL;
		AVStream *out_stream = NULL;

		AVStream *in_stream = input_format_context->streams[index];

		switch (in_stream->codec->codec_type)
		{
		case AVMEDIA_TYPE_VIDEO:
			input_video_stream_index = index;
			out_stream = avformat_new_stream(output_video_format_context, in_stream->codec->codec);
			ofmt_ctx = output_video_format_context;
			break;
		case AVMEDIA_TYPE_AUDIO:
			input_audio_stream_index = index;
			out_stream = avformat_new_stream(output_audio_format_context, in_stream->codec->codec);
			ofmt_ctx = output_audio_format_context;
			break;
		default:
			break;
		}

		if (!out_stream)
			return AVERROR_UNKNOWN;

		errCode = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (errCode < 0)
			return AVERROR_UNKNOWN;

		out_stream->codec->codec_tag = 0;

		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件，写文件头
	//
	//////////////////////////////////////////////////////////////////////////

	if (!(output_video_format->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_video_format_context->pb, T2A(video_url), AVIO_FLAG_WRITE);
		if (errCode < 0)
			return -5;
	}

	if (!(output_audio_format->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_audio_format_context->pb, T2A(audio_url), AVIO_FLAG_WRITE);
		if (errCode < 0)
			return -5;
	}

	errCode = avformat_write_header(output_video_format_context, NULL);
	if (errCode < 0)
		return errCode;

	errCode = avformat_write_header(output_audio_format_context, NULL);
	if (errCode < 0)
		return errCode;

	//////////////////////////////////////////////////////////////////////////
	//
	// 编码包解复用
	//
	//////////////////////////////////////////////////////////////////////////

#ifdef USE_H264BSF
	AVBitStreamFilterContext *h264_bit_stream_filter_context = av_bitstream_filter_init("h264_mp4toannexb");
#endif

	AVPacket av_packet;
	while (true)
	{
		AVFormatContext *ofmt_ctx = NULL;
		AVStream *in_stream = NULL;
		AVStream *out_stream = NULL;

		errCode = av_read_frame(input_format_context, &av_packet);
		if (errCode < 0)
			break;

		in_stream = input_format_context->streams[av_packet.stream_index];

		if (av_packet.stream_index == input_video_stream_index)
		{
			out_stream = output_video_format_context->streams[0];
			ofmt_ctx = output_video_format_context;
#ifdef USE_H264BSF
			av_bitstream_filter_filter(h264_bit_stream_filter_context, in_stream->codec, NULL, &av_packet.data, &av_packet.size, av_packet.data, av_packet.size, 0);
#endif
		}
		else if (av_packet.stream_index == input_audio_stream_index)
		{
			out_stream = output_audio_format_context->streams[0];
			ofmt_ctx = output_audio_format_context;
		}
		else
			continue;

		// 视音频编码包属性转换
		av_packet.pts = av_rescale_q_rnd(av_packet.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_packet.dts = av_rescale_q_rnd(av_packet.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_packet.duration = av_rescale_q(av_packet.duration, in_stream->time_base, out_stream->time_base);
		av_packet.pos = -1;
		av_packet.stream_index = 0;

		// 写入各自的容器中
		errCode = av_interleaved_write_frame(ofmt_ctx, &av_packet);
		if (errCode < 0)
			return errCode;

		av_free_packet(&av_packet);
	}

#ifdef USE_H264BSF
	av_bitstream_filter_close(h264_bit_stream_filter_context);
#endif

	//////////////////////////////////////////////////////////////////////////
	//
	// 写容器文件尾
	//
	//////////////////////////////////////////////////////////////////////////

	av_write_trailer(output_audio_format_context);
	av_write_trailer(output_video_format_context);

	//////////////////////////////////////////////////////////////////////////
	//
	// 清理资源
	//
	//////////////////////////////////////////////////////////////////////////

	avformat_close_input(&input_format_context);

	if (output_audio_format_context && !(output_audio_format->flags & AVFMT_NOFILE))
		avio_close(output_audio_format_context->pb);

	if (output_video_format_context && !(output_video_format->flags & AVFMT_NOFILE))
		avio_close(output_video_format_context->pb);

	avformat_free_context(output_audio_format_context);
	avformat_free_context(output_video_format_context);

	return 0;
}

