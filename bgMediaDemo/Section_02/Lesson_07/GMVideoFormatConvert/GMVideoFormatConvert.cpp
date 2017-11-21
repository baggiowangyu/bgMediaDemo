// GMVideoFormatConvert.cpp : 定义控制台应用程序的入口点。
//
// 本范例将flv文件转为MP4文件

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
#include "libavutil/error.h"
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

	USES_CONVERSION;

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFormatContext *input_format_context = NULL;
	int errCode = avformat_open_input(&input_format_context, T2A(input_media), NULL, NULL);
	if (errCode != 0)
	{
		return errCode;
	}

	errCode = avformat_find_stream_info(input_format_context, NULL);
	if (errCode < 0)
	{
		return errCode;
	}

	int stream_mapping_size = input_format_context->nb_streams;
	int *stream_mapping = (int*)av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
	if (!stream_mapping)
	{
		return -2;
	}

	av_dump_format(input_format_context, 0, T2A(input_media), 0);

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFormatContext * output_format_context = NULL;
	avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(output_media));
	if (!output_format_context)
	{
		return -1;
	}

	AVOutputFormat *output_format = output_format_context->oformat;

	//////////////////////////////////////////////////////////////////////////
	//
	//
	//
	//////////////////////////////////////////////////////////////////////////

	int stream_index = 0;
	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		AVStream *out_stream = NULL;
		AVStream *in_stream = input_format_context->streams[index];
		AVCodecParameters *in_codecpar = in_stream->codecpar;

		if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)
		{
			stream_mapping[index] = -1;
			continue;
		}

		stream_mapping[index] = stream_index++;

		out_stream = avformat_new_stream(output_format_context, NULL);
		if (!out_stream)
		{
			return -3;
		}

		errCode = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
		if (errCode < 0)
		{
			return -4;
		}

		out_stream->codecpar->codec_tag = 0;
	}

	av_dump_format(output_format_context, 0, T2A(output_media), 1);


	//////////////////////////////////////////////////////////////////////////
	//
	// 
	//
	//////////////////////////////////////////////////////////////////////////

	if (!(output_format->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_format_context->pb, T2A(output_media), AVIO_FLAG_WRITE);
		if (errCode < 0)
		{
			return errCode;
		}
	}

	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
	{
		return errCode;
	}

	AVPacket av_packet;
	while (true)
	{
		AVStream *in_stream = NULL, *out_stream = NULL;

		errCode = av_read_frame(input_format_context, &av_packet);
		if (errCode < 0)
			break;

		in_stream = input_format_context->streams[av_packet.stream_index];
		if (av_packet.stream_index >= stream_mapping_size || stream_mapping[av_packet.stream_index] < 0)
		{
			av_packet_unref(&av_packet);
			continue;
		}

		//av_packet.stream_index = stream_mapping[av_packet.stream_index];
		out_stream = output_format_context->streams[av_packet.stream_index];

		av_packet.pts = av_rescale_q_rnd(av_packet.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_packet.dts = av_rescale_q_rnd(av_packet.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		av_packet.duration = av_rescale_q(av_packet.duration, in_stream->time_base, out_stream->time_base);
		av_packet.pos = -1;

		errCode = av_interleaved_write_frame(output_format_context, &av_packet);
		if (errCode < 0)
		{
			break;
		}

		av_packet_unref(&av_packet);
	}

	av_write_trailer(output_format_context);

	avformat_close_input(&input_format_context);

	/* close output */
	if (output_format_context && !(output_format->flags & AVFMT_NOFILE))
		avio_closep(&output_format_context->pb);
	avformat_free_context(output_format_context);

	av_freep(&stream_mapping);

	if (errCode < 0 && errCode != AVERROR_EOF) {
		printf("Error occurred\n");
		return 1;
	}

	return 0;
}

