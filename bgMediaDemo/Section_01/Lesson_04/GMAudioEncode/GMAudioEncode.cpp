// GMAudioEncode.cpp : 定义控制台应用程序的入口点。
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
#include "libavutil/avstring.h"
#ifdef __cplusplus
};
#endif

#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		printf("GMAudioEncode.exe <audio_in_path> <audio_out_path> \n");
		return 0;
	}

	// 完成初始化注册
	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	TCHAR audio_in_path[4096] = {0};
	_tcscpy_s(audio_in_path, 4096, argv[1]);

	TCHAR audio_out_path[4096] = {0};
	_tcscpy_s(audio_out_path, 4096, argv[1]);

	//
	// 打开音频输入文件
	//
	USES_CONVERSION;
	AVFormatContext *in_format_context = NULL;
	int errCode = avformat_open_input(&in_format_context, T2A(audio_in_path), NULL, NULL);
	if (errCode != 0)
	{
		printf("avformat_open_input failed.\n");
		return errCode;
	}

	// 初步查找音频信息
	errCode = avformat_find_stream_info(in_format_context, NULL);
	if (errCode < 0)
	{
		printf("avformat_find_stream_info failed. %s", av_err2str(errCode));
		avformat_close_input(&in_format_context);
		return errCode;
	}

	// 找到音频流
	int audio_stream_index = -1;
	for (int index = 0; index < in_format_context->nb_streams; ++index)
	{
		if (in_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index = index;
			break;
		}
	}

	AVStream *audio_stream = in_format_context->streams[audio_stream_index];

	// 查找音频解码器
	AVCodec *audio_decoder = avcodec_find_decoder(audio_stream->codec->codec_id);
	if (audio_decoder == NULL)
	{
		printf("Could not allocate a decoding context\n");
		avformat_close_input(&in_format_context);
		return -1;
	}

	// 声明一个新的解码上下文
	AVCodecContext *audio_decode_context = avcodec_alloc_context3(audio_decoder);
	if (audio_decode_context == NULL)
	{
		printf("Could not allocate a decoding context\n");
		avformat_close_input(&in_format_context);
		return AVERROR(ENOMEM);
	}

	errCode = avcodec_parameters_to_context(audio_decode_context, audio_stream->codecpar);
	if (errCode < 0)
	{
		avformat_close_input(&in_format_context);
		avcodec_free_context(&audio_decode_context);
		return errCode;
	}

	errCode = avcodec_open2(audio_decode_context, audio_decoder, NULL);
	if (errCode < 0)
	{
		printf("Could not open input codec (error '%s')\n", av_err2str(error));
		avformat_close_input(&in_format_context);
		avcodec_free_context(&audio_decode_context);
		return errCode;
	}

	//
	// 创建音频输出文件
	//
	AVIOContext *output_io_context = NULL;
	errCode = avio_open(&output_io_context, T2A(audio_out_path), AVIO_FLAG_WRITE);
	if (errCode < 0)
	{
		printf("Could not open output file '%s' (error '%s')\n", T2A(audio_out_path), av_err2str(error));
		return errCode;
	}

	// 申请输出上下文
	AVFormatContext *output_format_context = avformat_alloc_context();
	if (output_format_context != NULL)
	{
		printf("Could not allocate output format context\n");
		return AVERROR(ENOMEM);
	}

	output_format_context->pb = output_io_context;

	output_format_context->oformat = av_guess_format(NULL, T2A(audio_out_path), NULL);
	if (output_format_context->oformat == NULL)
	{
		printf("Could not find output file format\n");
		return AVERROR(ENOMEM);
	}

	av_strlcpy(output_format_context->filename, T2A(audio_out_path), sizeof(output_format_context->filename));

	// 查找编码器
	AVCodec *output_audio_encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (output_audio_encoder == NULL)
	{
		printf("Could not find an AAC encoder.\n");
		return -2;
	}

	AVStream *output_audio_stream = avformat_new_stream(output_format_context, output_audio_encoder);
	if (output_audio_stream == NULL)
	{
		printf("Could not create new stream\n");
		return AVERROR(ENOMEM);
	}

	AVCodecContext *encode_codec_context = avcodec_alloc_context3(output_audio_encoder);
	if (encode_codec_context == NULL)
	{
		printf("Could not allocate an encoding context\n");
		return AVERROR(ENOMEM);
	}

	// 设置基本的编码参数
	// 
	encode_codec_context->channels			= 2;
	encode_codec_context->channel_layout	= av_get_default_channel_layout(2);
	encode_codec_context->sample_rate		= audio_decode_context->sample_rate;
	encode_codec_context->sample_fmt		= output_audio_encoder->sample_fmts[0];
	encode_codec_context->bit_rate			= 96000;

	//
	encode_codec_context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	// 设置采样率
	output_audio_stream->time_base.den = audio_decode_context->sample_rate;
	output_audio_stream->time_base.num = 1;

	// 有些格式（例如MP4）需要全局头部
	if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
	{
		encode_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	errCode = avcodec_open2(encode_codec_context, output_audio_encoder, NULL);
	if (errCode < 0)
	{
		printf("Could not open output codec (error '%s')\n", av_err2str(errCode));
		return errCode;
	}

	errCode = avcodec_parameters_to_context(output_audio_stream->codecpar, encode_codec_context);
	if (errCode < 0)
	{
		printf("Could not initialize stream parameters\n");
		return errCode;
	}

	//
	// 初始化重采样模块
	// 

	return 0;
}

