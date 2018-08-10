// GMVideoCodecConvert.cpp : 定义控制台应用程序的入口点。
//
// 视频转码：将原视频的视音频编码转换为其他的编码格式；
//           此过程中可以保持封装格式不变，也可以封装为新的格式；

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
	char errmsg[4096] = {0};

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
	// 输入视频解复用
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

	// 用于输出的解码器上下文
	int input_video_stream_index = -1;
	int input_audio_stream_index = -1;
	AVStream *input_video_stream = NULL;
	AVStream *input_audio_stream = NULL;
	AVCodec *input_video_codec = NULL;
	AVCodec *input_audio_codec = NULL;
	AVCodecContext *input_video_codec_context = NULL;
	AVCodecContext *input_audio_codec_context = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			// 流ID，后面复用时会用到
			input_video_stream_index = index;
			//input_video_stream = input_format_context->streams[index];

			input_video_codec = avcodec_find_decoder(input_format_context->streams[index]->codec->codec_id);
			if (input_video_codec == NULL)
			{
				printf("没有找到视频解码器");
				return -1;
			}

			input_video_codec_context = avcodec_alloc_context3(input_video_codec);
			if (input_video_codec_context == NULL)
			{
				printf("申请");
				return -2;
			}

			errCode = avcodec_copy_context(input_video_codec_context, input_format_context->streams[index]->codec);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				return errCode;
			}

			errCode = avcodec_open2(input_video_codec_context, input_video_codec, NULL);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				return errCode;
			}
		}
		else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			// 流ID，后面复用时会用到
			input_audio_stream_index = index;
			//input_audio_stream = input_format_context->streams[index];

			input_audio_codec = avcodec_find_decoder(input_format_context->streams[index]->codec->codec_id);
			if (input_audio_codec == NULL)
			{
				printf("没有找到视频解码器");
				return -1;
			}

			input_audio_codec_context = avcodec_alloc_context3(input_audio_codec);
			if (input_audio_codec_context == NULL)
			{
				printf("申请");
				return -2;
			}

			errCode = avcodec_copy_context(input_audio_codec_context, input_format_context->streams[index]->codec);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				return errCode;
			}

			errCode = avcodec_open2(input_audio_codec_context, input_audio_codec, NULL);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				return errCode;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备输出
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *output_format_context = NULL;
	errCode = avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(output_media));
	if (errCode < 0)
	{
		av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
		return errCode;
	}

	if (!(output_format_context->oformat->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_format_context->pb, T2A(output_media), AVIO_FLAG_WRITE);
		if (errCode < 0)
		{
			printf("Could not open output file '%s'", T2A(output_media));
			return errCode;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	//  准备编码上下文
	//
	//////////////////////////////////////////////////////////////////////////

	int output_video_stream_index = -1;
	int output_audio_stream_index = -1;
	AVStream *output_video_stream = NULL;
	AVStream *output_audio_stream = NULL;
	AVCodec *output_video_codec = NULL;
	AVCodec *output_audio_codec = NULL;
	AVCodecContext *output_video_codec_context = NULL;
	AVCodecContext *output_audio_codec_context = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (index == input_video_stream_index)
		{
			output_video_stream_index = index;

			output_video_codec = avcodec_find_encoder(output_format_context->oformat->video_codec);
			if (output_video_codec == NULL)
			{
				printf("没有找到视频编码器");
				return -1;
			}

			// 创建输出视频流
			output_video_stream = avformat_new_stream(output_format_context, output_video_codec);
			if (output_video_stream == NULL)
			{
				printf("申请输出视频流失败！");
				return -3;
			}

			output_video_codec_context = avcodec_alloc_context3(output_video_codec);
			if (output_video_codec_context == NULL)
			{
				printf("申请输出视频编码上下文失败！");
				return -2;
			}

			output_video_codec_context->height = input_video_codec_context->height;
			output_video_codec_context->width = input_video_codec_context->width;
			output_video_codec_context->sample_aspect_ratio = input_video_codec_context->sample_aspect_ratio;
			/* take first format from list of supported formats */
			if (output_video_codec->pix_fmts)
				output_video_codec_context->pix_fmt = output_video_codec->pix_fmts[0];
			else
				output_video_codec_context->pix_fmt = input_video_codec_context->pix_fmt;
			/* video time_base can be set to whatever is handy and supported by encoder */
			output_video_codec_context->time_base = av_inv_q(input_video_codec_context->framerate);
			//output_video_codec_context->framerate = 30;

			//AVDictionary *param = 0;
			//if (output_format_context->oformat->video_codec == AV_CODEC_ID_H264)
			//{
			//	av_opt_set(output_video_codec_context->priv_data, "preset", "slow", 0);
			//	av_opt_set(output_video_codec_context->priv_data, "tune", "zerolatency", 0);
			//}
			//if (output_format_context->oformat->video_codec == AV_CODEC_ID_H265)
			//{
			//	av_opt_set(output_video_codec_context->priv_data, "preset", "ultrafast", 0);
			//	av_opt_set(output_video_codec_context->priv_data, "tune", "zero-latency", 0);
			//}

			// 打开视频编码器
			errCode = avcodec_open2(output_video_codec_context, output_video_codec, NULL);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				return errCode;
			}

			//
			errCode = avcodec_parameters_from_context(output_video_stream->codecpar, output_video_codec_context);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				return errCode;
			}

			if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
				output_video_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			output_video_stream->time_base = output_video_codec_context->time_base;
			errCode = avcodec_copy_context(output_video_stream->codec, output_video_codec_context);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				return errCode;
			}
		}
		else if (index == input_audio_stream_index)
		{
			// 创建输出视频流
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 写文件头
	//
	//////////////////////////////////////////////////////////////////////////

	/* init muxer, write output file header */
	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0) {
		av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
		return errCode;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 读取帧，并进行转码
	//
	//////////////////////////////////////////////////////////////////////////

	while (true)
	{
		AVPacket orignal_pkt;
		errCode = av_read_frame(input_format_context, &orignal_pkt);
		if (errCode < 0)
		{
			av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
			printf("av_read_frame. err : %s\n", errmsg);
			break;
		}

		if (orignal_pkt.stream_index == input_video_stream_index)
		{
			// 视频帧，解码
			AVFrame *orignal_frame = av_frame_alloc();
			int got_pic = 0;
			errCode = avcodec_decode_video2(input_video_codec_context, orignal_frame, &got_pic, &orignal_pkt);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				av_frame_free(&orignal_frame);
				break;
			}

			if (!got_pic)
			{
				av_frame_free(&orignal_frame);
				continue;
			}

			// 拿到解码后的原始图片了，重新编码
			AVPacket encoded_pkt;
			av_init_packet(&encoded_pkt);
			encoded_pkt.data = NULL;
			encoded_pkt.size = 0;

			encoded_pkt.pts = orignal_pkt.pts;
			encoded_pkt.dts = orignal_pkt.dts;
			encoded_pkt.duration = orignal_pkt.duration;
			encoded_pkt.pos = orignal_pkt.pos;

			int got_pkt = 0;
			errCode = avcodec_encode_video2(output_video_codec_context, &encoded_pkt, orignal_frame, &got_pkt);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				av_frame_free(&orignal_frame);
				break;
			}

			if (!got_pkt)
			{
				av_frame_free(&orignal_frame);
				continue;
			}

			// 编码完成，调整一波时间戳？

			// 写入文件
			errCode = av_write_frame(output_format_context, &encoded_pkt);
			if (errCode < 0)
			{
				av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
				av_frame_free(&orignal_frame);
				break;
			}

			av_free_packet(&orignal_pkt);
			av_free_packet(&encoded_pkt);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 写文件尾部
	//
	//////////////////////////////////////////////////////////////////////////

	errCode = av_write_trailer(output_format_context);
	if (errCode < 0) {
		av_make_error_string(errmsg, AV_ERROR_MAX_STRING_SIZE, errCode);
		return errCode;
	}

	return 0;
}

