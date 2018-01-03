// GMRtspPusher.cpp : 定义控制台应用程序的入口点。
//

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

#define PUSH_PROTOCOL	"rtsp"

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		printf("usage : GMMediaPusher.exe <source_url> <stream_server_url>\n");
		return 0;
	}

	//BOOL b = SetConsoleCtrlHandler(HandlerRoutine, TRUE);

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

	//////////////////////////////////////////////////////////////////////////
	//
	// 创建输出上下文
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *output_format_context = avformat_alloc_context();
	output_format_context->oformat = av_guess_format(PUSH_PROTOCOL, NULL, NULL);
	sprintf_s(output_format_context->filename, sizeof(output_format_context->filename), "%s://%s:%d/123.sdp", PUSH_PROTOCOL, "127.0.0.1", 554);

	AVOutputFormat *output_format = output_format_context->oformat;

	//////////////////////////////////////////////////////////////////////////
	//
	// 分析流，拿到视音频信息
	//
	//////////////////////////////////////////////////////////////////////////

	int input_video_stream_index = -1;
	int input_audio_stream_index = -1;

	int output_video_stream_index = -1;
	int output_audio_stream_index = -1;

	AVStream *input_video_stream = NULL;
	AVStream *output_video_stream = NULL;

	AVStream *input_audio_stream = NULL;
	AVStream *output_audio_stream = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			input_video_stream_index = index;
			input_video_stream = input_format_context->streams[index];

			output_video_stream = avformat_new_stream(output_format_context, input_video_stream->codec->codec);

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

			output_video_stream->codec->extradata = (unsigned char*)av_malloc(input_video_stream->codec->extradata_size);
			memcpy(output_video_stream->codec->extradata, input_video_stream->codec->extradata, input_video_stream->codec->extradata_size);
			output_video_stream->codec->extradata_size = input_video_stream->codec->extradata_size;
			errCode = avcodec_open2(output_video_stream->codec, input_video_stream->codec->codec, NULL);

			char sdp[2048] = {0};
			strcpy_s(sdp, 2048, "123.sdp");
			errCode = av_sdp_create(&output_format_context, 1, sdp, sizeof(sdp));

			Sleep(1);
		}
		//else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		//{
		//	input_audio_stream_index = index;
		//	input_audio_stream = input_format_context->streams[index];

		//	output_audio_stream = avformat_new_stream(output_format_context, input_audio_stream->codec->codec);

		//	if (!output_audio_stream)
		//		return AVERROR_UNKNOWN;

		//	output_audio_stream_index = output_audio_stream->index;

		//	// 复制编码上下文
		//	errCode = avcodec_copy_context(output_audio_stream->codec, input_audio_stream->codec);
		//	if (errCode < 0)
		//		return errCode;

		//	output_audio_stream->codec->codec_tag = 0;

		//	if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
		//		output_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		//}
	}



	av_dump_format(input_format_context, 0, T2A(source_url), 0);

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件，写入文件头
	//
	//////////////////////////////////////////////////////////////////////////

	if (!(output_format_context->flags & AVFMT_NOFILE))
	{
		AVDictionary* options = NULL;
		av_dict_set(&options, "rtsp_transport", "tcp", 0);

		errCode = avio_open2(&output_format_context->pb, output_format_context->filename, AVIO_FLAG_WRITE, NULL, &options);
		if (errCode < 0)
		{
			char errmsg[4096] = {0};
			printf("Open push url failed...%s\n", av_make_error_string(errmsg, 4096, errCode));
			return errCode;
		}
	}

	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
		return errCode;

	AVPacket av_packet;
	int frame_index = 0;
	int total = 0;
	int size = 0;
	int stream_index = -1;

	while (true)
	{
		errCode = av_read_frame(input_format_context, &av_packet);
		if (errCode < 0)
			break;

		if (av_packet.stream_index != input_video_stream_index)
		{
			av_free_packet(&av_packet);
			continue;
		}

		av_packet.pts = av_rescale_q_rnd(av_packet.pts, input_video_stream->time_base, output_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_packet.dts = av_rescale_q_rnd(av_packet.dts, input_video_stream->time_base, output_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_packet.duration = av_rescale_q(av_packet.duration, input_video_stream->time_base, output_video_stream->time_base);
		av_packet.pos = -1;
		av_packet.stream_index = output_video_stream_index;

		size = av_packet.size;

		errCode = av_write_frame(output_format_context, &av_packet);

		av_free_packet(&av_packet);
	}

	return 0;
}

