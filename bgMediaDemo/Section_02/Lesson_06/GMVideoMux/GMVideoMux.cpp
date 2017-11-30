// GMVideoMux.cpp : 定义控制台应用程序的入口点。
//
// 本范例将h264文件和aac文件封装为带音轨的MP4文件
// $(OutDir)\BOB.h264 $(OutDir)\BOB.mp3 $(OutDir)\BOB.mp4
// 粗暴测试了一下，似乎效果还可以

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

	errCode = avformat_find_stream_info(input_audio_format_context, NULL);
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
	AVStream *input_video_stream = NULL;
	AVStream *output_video_stream = NULL;

	for (int index = 0; index < input_video_format_context->nb_streams; ++index)
	{
		if (input_video_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			input_video_stream = input_video_format_context->streams[index];
			output_video_stream = avformat_new_stream(output_format_context, input_video_stream->codec->codec);

			input_video_stream_index = index;

			if (!output_video_stream)
				return AVERROR_UNKNOWN;

			output_video_stream_index = output_video_stream->index;

			// 复制编码上下文
			errCode = avcodec_copy_context(output_video_stream->codec, input_video_stream->codec);
			if (errCode < 0)
				return errCode;

			output_video_stream->codec->codec_tag = 0;

			if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
				output_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	int input_audio_stream_index = -1;
	int output_audio_stream_index = -1;
	AVStream *input_audio_stream = NULL;
	AVStream *output_audio_stream = NULL;

	for (int index = 0; index < input_audio_format_context->nb_streams; ++index)
	{
		if (input_audio_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			input_audio_stream = input_audio_format_context->streams[index];
			output_audio_stream = avformat_new_stream(output_format_context, input_audio_stream->codec->codec);

			input_audio_stream_index = index;

			if (!output_audio_stream)
				return AVERROR_UNKNOWN;

			output_audio_stream_index = output_audio_stream->index;

			// 复制编码上下文
			errCode = avcodec_copy_context(output_audio_stream->codec, input_audio_stream->codec);
			if (errCode < 0)
				return errCode;

			output_audio_stream->codec->codec_tag = 0;

			if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
				output_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
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

	AVPacket av_video_packet;
	AVPacket av_audio_packet;
	int frame_index = 0;

	int video_frames = input_video_stream->nb_frames;
	int audio_frames = input_audio_stream->nb_frames;

	// 按照一个视频帧带五个音频帧的方式进行复用，尝试一下
	for (int index = 0; index < video_frames; ++index)
	{
		errCode = av_read_frame(input_video_format_context, &av_video_packet);
		if (errCode < 0)
			break;

		// 写入
#ifdef USE_H264BSF
		av_bitstream_filter_filter(h264_bit_stream_filter_context, input_video_stream->codec, NULL, &av_video_packet.data, &av_video_packet.size, av_video_packet.data, av_video_packet.size, 0);
#endif

		// 转换PTS/DTS
		av_video_packet.pts = av_rescale_q_rnd(av_video_packet.pts, input_video_stream->time_base, output_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_video_packet.dts = av_rescale_q_rnd(av_video_packet.dts, input_video_stream->time_base, output_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_video_packet.duration = av_rescale_q(av_video_packet.duration, input_video_stream->time_base, output_video_stream->time_base);
		av_video_packet.pos = -1;
		av_video_packet.stream_index = output_video_stream_index;

		// 写入编码包
		errCode = av_interleaved_write_frame(output_format_context, &av_video_packet);
		if (errCode < 0)
			break;

		// 5帧音频
		for (int index2 = 0; index2 < 5; ++index2)
		{
			errCode = av_read_frame(input_audio_format_context, &av_audio_packet);
			if (errCode < 0)
				break;

			// 写入
#ifdef USE_AACBSF
			av_bitstream_filter_filter(aac_bit_stream_filter_context, input_audio_stream->codec, NULL, &av_audio_packet.data, &av_audio_packet.size, av_audio_packet.data, av_audio_packet.size, 0);
#endif

			// 转换PTS/DTS
			av_audio_packet.pts = av_rescale_q_rnd(av_audio_packet.pts, input_audio_stream->time_base, output_audio_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			av_audio_packet.dts = av_rescale_q_rnd(av_audio_packet.dts, input_audio_stream->time_base, output_audio_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			av_audio_packet.duration = av_rescale_q(av_audio_packet.duration, input_audio_stream->time_base, output_audio_stream->time_base);
			av_audio_packet.pos = -1;
			av_audio_packet.stream_index = output_audio_stream_index;

			// 写入编码包
			errCode = av_interleaved_write_frame(output_format_context, &av_audio_packet);
			if (errCode < 0)
				break;

			av_free_packet(&av_audio_packet);
		}

		av_free_packet(&av_video_packet);
	}

	// 写文件尾
	av_write_trailer(output_format_context);

#ifdef USE_H264BSF
	av_bitstream_filter_close(h264_bit_stream_filter_context);
#endif

#ifdef USE_AACBSF
	av_bitstream_filter_close(aac_bit_stream_filter_context);
#endif

	avformat_close_input(&input_video_format_context);
	avformat_close_input(&input_audio_format_context);

	// 关闭输出文件
	if (output_format_context && !(output_format->flags & AVFMT_NOFILE))
		avio_close(output_format_context->pb);

	avformat_free_context(output_format_context);

	_CrtDumpMemoryLeaks();
	return 0;
}


