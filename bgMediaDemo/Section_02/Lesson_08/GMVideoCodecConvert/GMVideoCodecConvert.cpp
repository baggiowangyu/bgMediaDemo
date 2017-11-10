// GMVideoCodecConvert.cpp : 定义控制台应用程序的入口点。
//

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

	// 整个流程：
	// 打开输入文件，读取视频流，写入到输出文件
	// 读取音频流，解码后重编码为AAC或者MP3，写入到输出文件

	USES_CONVERSION;
	// 准备输出上下文
	AVFormatContext *output_format_context = NULL;
	int errCode = avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(output_media));
	if (errCode != 0)
	{
		return errCode;
	}

	AVFormatContext *input_format_context = NULL;
	errCode = avformat_open_input(&input_format_context, T2A(input_media), NULL, NULL);
	if (errCode != 0)
	{
		return errCode;
	}

	avformat_find_stream_info(input_format_context, NULL);

	int video_stream_index = -1;
	int audio_stream_index = -1;

	AVStream *input_video_stream = NULL;
	AVStream *input_audio_stream = NULL;

	AVStream *output_video_stream = NULL;
	AVStream *output_audio_stream = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = index;
			input_video_stream = input_format_context->streams[video_stream_index];

			output_video_stream = avformat_new_stream(output_format_context, input_video_stream->codec->codec);
			output_video_stream->index = index;

			errCode = avcodec_copy_context(output_video_stream->codec, input_video_stream->codec);
			if (errCode < 0)
			{
				return -1;
			}
		}
		else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{

		}
	}

	output_video_stream->codec->codec_tag = 0;
	//output_audio_stream->codec->codec_tag = 0;

	if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
	{
		output_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		//output_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	av_dump_format(output_format_context, 0, T2A(output_media), 1);

	if (!(output_format_context->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_format_context->pb, T2A(output_media), AVIO_FLAG_WRITE);
		if (errCode < 0)
		{
			return -4;
		}
	}

	// 写文件头
	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
	{
		return -5;
	}

	// 写数据帧
	AVPacket av_packet;
	while (av_read_frame(input_format_context, &av_packet) == 0)
	{
		// 
		AVStream *input_stream = input_format_context->streams[av_packet.stream_index];
		AVStream *output_stream = output_format_context->streams[av_packet.stream_index];

		// 转换PTS/DTS
		av_packet.pts = av_rescale_q_rnd(av_packet.pts, input_stream->time_base, output_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		av_packet.dts = av_rescale_q_rnd(av_packet.dts, input_stream->time_base, output_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		av_packet.duration = av_rescale_q(av_packet.duration, input_stream->time_base, output_stream->time_base);
		av_packet.pos = -1;

		// 写入编码帧
		errCode = av_interleaved_write_frame(output_format_context, &av_packet);
		if (errCode < 0)
		{
			break;
		}
	}

	// 写文件尾
	av_write_trailer(output_format_context);

	// 清理资源
	avformat_close_input(&input_format_context);

	if (output_format_context && !(output_format_context->flags & AVFMT_NOFILE))
	{
		avio_close(output_format_context->pb);
	}

	return 0;
}

