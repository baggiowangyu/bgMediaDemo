// StreamRecorder.cpp : 定义控制台应用程序的入口点。
//

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
#define USE_AACBSF

bool need_stop = false;
bool need_exit = false;

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		// Ctrl + C
		need_stop = true;
		need_exit = true;
		break;
	case CTRL_BREAK_EVENT:
		// Ctrl + Break
		need_stop = true;
		need_exit = true;
		break;
	case CTRL_CLOSE_EVENT:
		// Close or End Task
		need_stop = true;
		need_exit = true;
		break;
	case CTRL_LOGOFF_EVENT:
		// System user logoff
		need_stop = true;
		need_exit = true;
		break;
	case CTRL_SHUTDOWN_EVENT:
		// System will shutdown
		need_stop = true;
		need_exit = true;
		break;
	default:
		break;
	}
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		printf("usage : StreamRecorder.exe <name> <stream_url>\n");
		return 0;
	}

	SetConsoleCtrlHandler(HandlerRoutine, TRUE);

	int errCode = 0;

	TCHAR name[4096] = {0};
	_tcscpy_s(name, 4096, argv[1]);

	TCHAR stream_url[4096] = {0};
	_tcscpy_s(stream_url, 4096, argv[2]);

	USES_CONVERSION;

	av_register_all();
	avformat_network_init();

	while (true)
	{
		if (need_exit)
			break;

		//////////////////////////////////////////////////////////////////////////
		//
		// 打开流
		//
		//////////////////////////////////////////////////////////////////////////

		AVDictionary *options = NULL;
		AVFormatContext *stream_format_context = NULL;
		errCode = avformat_open_input(&stream_format_context, T2A(stream_url), NULL, &options);
		if (errCode < 0)
		{
			Sleep(1000);
			break;
		}

		errCode = avformat_find_stream_info(stream_format_context, NULL);
		if (errCode < 0)
		{
			avformat_close_input(&stream_format_context);
			Sleep(1000);
			break;
		}

		//////////////////////////////////////////////////////////////////////////
		//
		// 创建输出上下文
		//
		//////////////////////////////////////////////////////////////////////////

		// 获得当前目录
		TCHAR current_module_path[4096] = {0};
		GetModuleFileName(NULL, current_module_path, 4096);

		std::wstring cmp = current_module_path;
		int pos = cmp.find_last_of(L"\\");
		std::wstring base_dir = cmp.substr(0, pos + 1);

		// 获得当前时间
		SYSTEMTIME st;
		GetLocalTime(&st);

		TCHAR local_url[4096] = {0};
		_stprintf_s(local_url, 4096, _T("%s%s-%d%02d%02d-%02d%02d%02d.flv"), base_dir.c_str(), name, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);


		AVFormatContext *output_format_context = NULL;
		errCode = avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(local_url));
		if (errCode < 0)
		{
			avformat_close_input(&stream_format_context);

			Sleep(1000);
			break;
		}

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

		bool analyze_failed = false;
		for (int index = 0; index < stream_format_context->nb_streams; ++index)
		{
			if (stream_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				input_video_stream_index = index;
				input_video_stream = stream_format_context->streams[index];

				output_video_stream = avformat_new_stream(output_format_context, input_video_stream->codec->codec);

				if (!output_video_stream)
				{
					analyze_failed = true;
					break;
				}

				output_video_stream_index = output_video_stream->index;

				// 复制编码上下文
				errCode = avcodec_copy_context(output_video_stream->codec, input_video_stream->codec);
				if (errCode < 0)
				{
					analyze_failed = true;
					break;
				}

				output_video_stream->codec->codec_tag = 0;

				if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
					output_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
			else if (stream_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				input_audio_stream_index = index;
				input_audio_stream = stream_format_context->streams[index];

				output_audio_stream = avformat_new_stream(output_format_context, input_audio_stream->codec->codec);

				if (!output_audio_stream)
				{
					analyze_failed = true;
					break;
				}

				output_audio_stream_index = output_audio_stream->index;

				// 复制编码上下文
				errCode = avcodec_copy_context(output_audio_stream->codec, input_audio_stream->codec);
				if (errCode < 0)
				{
					analyze_failed = true;
					break;
				}

				output_audio_stream->codec->codec_tag = 0;

				if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
					output_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
		}

		if (analyze_failed)
		{
			avformat_close_input(&stream_format_context);
			avformat_free_context(output_format_context);

			Sleep(1000);
			break;
		}

		//av_dump_format(stream_format_context, 0, T2A(stream_url), 0);

		//////////////////////////////////////////////////////////////////////////
		//
		// 打开输出文件，写入文件头
		//
		//////////////////////////////////////////////////////////////////////////

		if (!(output_format_context->flags & AVFMT_NOFILE))
		{
			errCode = avio_open(&output_format_context->pb, T2A(local_url), AVIO_FLAG_WRITE);
			if (errCode < 0)
			{
				avformat_close_input(&stream_format_context);
				avformat_free_context(output_format_context);

				Sleep(1000);
				break;
			}
		}

		errCode = avformat_write_header(output_format_context, NULL);
		if (errCode < 0)
		{
			avformat_close_input(&stream_format_context);
			if (output_format_context && !(output_format->flags & AVFMT_NOFILE))
				avio_close(output_format_context->pb);
			avformat_free_context(output_format_context);

			Sleep(1000);
			break;
		}

#ifdef USE_H264BSF
		AVBitStreamFilterContext *h264_bit_stream_filter_context = av_bitstream_filter_init("h264_mp4toannexb");
#endif

#ifdef USE_AACBSF
		AVBitStreamFilterContext *aac_bit_stream_filter_context = av_bitstream_filter_init("aac_adtstoasc");
#endif

		AVPacket av_packet;
		int frame_index = 0;
		int total = 0;
		int size = 0;
		int stream_index = -1;

		while (true)
		{
			if (need_stop)
				break;

			errCode = av_read_frame(stream_format_context, &av_packet);
			if (errCode < 0)
				break;

			if (av_packet.stream_index == input_video_stream_index)
			{
#ifdef USE_H264BSF
				av_bitstream_filter_filter(h264_bit_stream_filter_context, input_video_stream->codec, NULL, &av_packet.data, &av_packet.size, av_packet.data, av_packet.size, 0);
#endif
				stream_index = output_video_stream_index;
			}
			else if (av_packet.stream_index == input_audio_stream_index)
			{
#ifdef USE_AACBSF
				av_bitstream_filter_filter(aac_bit_stream_filter_context, input_audio_stream->codec, NULL, &av_packet.data, &av_packet.size, av_packet.data, av_packet.size, 0);
#endif
				stream_index = output_audio_stream_index;
			}

			// 转换PTS/DTS
			av_packet.pts = av_rescale_q_rnd(av_packet.pts, input_video_stream->time_base, output_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			av_packet.dts = av_rescale_q_rnd(av_packet.dts, input_video_stream->time_base, output_video_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			av_packet.duration = av_rescale_q(av_packet.duration, input_video_stream->time_base, output_video_stream->time_base);
			av_packet.pos = -1;
			av_packet.stream_index = stream_index;

			size = av_packet.size;

			// 写入编码包
			errCode = av_interleaved_write_frame(output_format_context, &av_packet);
			if (errCode < 0)
				break;

			printf("Write a packet.\t size : %d,\t total : %d\n", size, total);
			total += size;

			av_free_packet(&av_packet);
		}

		// 写文件尾
		av_write_trailer(output_format_context);
		printf("Write file tailer...\n");

		//////////////////////////////////////////////////////////////////////////
		//
		// 清理资源
		//
		//////////////////////////////////////////////////////////////////////////

		avformat_close_input(&stream_format_context);

		// 关闭输出文件
		if (output_format_context && !(output_format->flags & AVFMT_NOFILE))
			avio_close(output_format_context->pb);

		avformat_free_context(output_format_context);

		Sleep(1000);
	}

	return 0;
}

