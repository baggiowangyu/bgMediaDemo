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
	// 这段代码片段需要拿到视音频解码上下文（上下文已经打开了解码器）
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

// 	StreamContext *stream_ctx = av_mallocz_array(input_format_context->nb_streams, sizeof(*stream_ctx));
// 	if (!stream_ctx)
// 	{
// 		return -1;
// 	}

	// 用于输出的解码器上下文
	int input_video_stream_index = -1;
	int input_audio_stream_index = -1;
	AVCodecContext *input_video_codec_context = NULL;
	AVCodecContext *input_audio_codec_context = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		AVStream *stream = input_format_context->streams[index];
		AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
		if (!decoder)
			return AVERROR_DECODER_NOT_FOUND;

		AVCodecContext *decode_codec_context = avcodec_alloc_context3(decoder);
		if (!decode_codec_context)
			return AVERROR(ENOMEM);

		errCode = avcodec_parameters_to_context(decode_codec_context, stream->codecpar);
		if (errCode < 0)
			return errCode;

		if (decode_codec_context->codec_type == AVMEDIA_TYPE_VIDEO || decode_codec_context->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			// 如果是视频流，需要猜测帧率
			if (decode_codec_context->codec_type == AVMEDIA_TYPE_VIDEO)
				decode_codec_context->framerate = av_guess_frame_rate(input_format_context, stream, NULL);
			
			// 打开解码器
			errCode = avcodec_open2(decode_codec_context, decoder, NULL);
			if (errCode < 0)
				return errCode;
		}

		if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			input_video_stream_index = index;
			input_video_codec_context = decode_codec_context;
		}
		else if (stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			input_audio_stream_index = index;
			input_audio_codec_context = decode_codec_context;
		}
	}

	av_dump_format(input_format_context, 0, T2A(input_media), 0);

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件
	// 此代码片段需要拿到输出格式上下文，视音频编码上下文（编码器已经被打开）
	//
	//////////////////////////////////////////////////////////////////////////
	AVCodecContext *encode_video_codec_context = NULL;
	AVCodecContext *encode_audio_codec_context = NULL;

	AVFormatContext *output_format_context = NULL;
	avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(output_media));

	if (!output_format_context)
		return AVERROR_UNKNOWN;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		AVStream *out_stream = avformat_new_stream(output_format_context, NULL);
		if (!out_stream)
			return AVERROR_UNKNOWN;

		AVStream *in_stream = input_format_context->streams[index];
		AVCodecContext *decode_codec_context = NULL;
		AVCodecID codec_id = AV_CODEC_ID_NONE;

		if (index == input_audio_stream_index)
		{
			decode_codec_context = input_audio_codec_context;
			codec_id = output_format_context->oformat->audio_codec;
		}
		else if (index == input_video_stream_index)
		{
			decode_codec_context = input_video_codec_context;
			codec_id = output_format_context->oformat->video_codec;
		}

		AVCodec *encoder = avcodec_find_encoder(codec_id);
		if (!encoder)
			return AVERROR_INVALIDDATA;

		AVCodecContext *encode_codec_context = avcodec_alloc_context3(encoder);
		if (!encode_codec_context)
			return AVERROR(ENOMEM);

		out_stream->codec = encode_codec_context;

		if (index == input_video_stream_index)
		{
			// 视频编码
			encode_codec_context->height = decode_codec_context->height;
			encode_codec_context->width = decode_codec_context->width;

			encode_codec_context->sample_aspect_ratio = decode_codec_context->sample_aspect_ratio;

			if (encoder->pix_fmts)
				encode_codec_context->pix_fmt = encoder->pix_fmts[0];
			else
				encode_codec_context->pix_fmt = decode_codec_context->pix_fmt;

			encode_codec_context->time_base = av_inv_q(decode_codec_context->framerate);
		}
		else if (index == input_audio_stream_index)
		{
			// 音频编码
			encode_codec_context->sample_rate = decode_codec_context->sample_rate;
			encode_codec_context->channel_layout = decode_codec_context->channel_layout;
			encode_codec_context->channels = av_get_channel_layout_nb_channels(encode_codec_context->channel_layout);

			encode_codec_context->sample_fmt = encoder->sample_fmts[0];
			encode_codec_context->time_base.num = 1;
			encode_codec_context->time_base.den = encode_codec_context->sample_rate;
		}

		errCode = avcodec_open2(decode_codec_context, encoder, NULL);
		if (errCode < 0)
		{
			return errCode;
		}

		errCode = avcodec_parameters_from_context(out_stream->codecpar, encode_codec_context);
		if (errCode < 0)
		{
			return errCode;
		}

		if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
			encode_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		out_stream->time_base = encode_codec_context->time_base;

		if (out_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			encode_video_codec_context = encode_codec_context;
		else if (out_stream->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			encode_audio_codec_context = encode_codec_context;
	}

	av_dump_format(output_format_context, 0, T2A(output_media), 1);

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件
	//
	//////////////////////////////////////////////////////////////////////////

	if (!(output_format_context->oformat->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_format_context->pb, T2A(output_media), AVIO_FLAG_WRITE);
		if (errCode < 0)
			return errCode;
	}

	// 初始化复用器，写入文件头
	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
	{
		return errCode;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 读取每一个编码包
	//
	//////////////////////////////////////////////////////////////////////////
	AVPacket av_packet;

	while (true)
	{
		errCode = av_read_frame(input_format_context, &av_packet);
		if (errCode < 0)
			break;

		AVFrame *av_frame = av_frame_alloc();
		int got_data = 0;

		// 先解码数据
		if (input_video_stream_index == av_packet.stream_index)
			errCode = avcodec_decode_video2(input_video_codec_context, av_frame, &got_data, &av_packet);
		else if (input_audio_stream_index == av_packet.stream_index)
			errCode = avcodec_decode_audio4(input_audio_codec_context, av_frame, &got_data, &av_packet);

		if (errCode < 0)
			return errCode;

		if (got_data < 1)
			continue;

		// 重新编码组包
		AVPacket encode_packet;
		encode_packet.data = NULL;
		encode_packet.size = 0;
		av_init_packet(&encode_packet);
		av_new_packet(&encode_packet, 4096);

		if (input_video_stream_index == av_packet.stream_index)
			errCode = avcodec_encode_video2(encode_video_codec_context, &encode_packet, av_frame, &got_data);
		else if (input_audio_stream_index == av_packet.stream_index)
			errCode = avcodec_encode_audio2(encode_audio_codec_context, &encode_packet, av_frame, &got_data);

		if (errCode < 0)
			return errCode;

		if (got_data < 1)
			return -6;

		encode_packet.stream_index = av_packet.stream_index;
		
		if (input_video_stream_index == av_packet.stream_index)
			av_packet_rescale_ts(&encode_packet, encode_video_codec_context->time_base, output_format_context->streams[input_video_stream_index]->time_base);
		else if (input_audio_stream_index == av_packet.stream_index)
			av_packet_rescale_ts(&encode_packet, encode_audio_codec_context->time_base, output_format_context->streams[input_audio_stream_index]->time_base);

		// 写入包
		errCode = av_interleaved_write_frame(output_format_context, &encode_packet);
		if (errCode < 0)
			return errCode;
	}

	// 刷掉上下文中剩余的包,分别刷视频和音频
	//for (int index = 0; index < input_format_context->nb_streams; ++index)
	//{
	//	AVCodecContext *encode_codec_context = NULL;

	//	if (index == input_video_stream_index)
	//		encode_codec_context = encode_video_codec_context;
	//	else if (index == input_audio_stream_index)
	//		encode_codec_context = encode_audio_codec_context;

	//	if (!(encode_codec_context->codec->capabilities & AV_CODEC_CAP_DELAY))
	//		continue;

	//	while (true)
	//	{
	//		errCode = 
	//	}
	//}

	// 写入文件尾
	av_write_trailer(output_format_context);

	// 清理资源

	return 0;
}

