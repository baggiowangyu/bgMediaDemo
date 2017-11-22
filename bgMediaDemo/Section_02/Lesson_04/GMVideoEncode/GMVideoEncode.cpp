// GMVideoEncode.cpp : 定义控制台应用程序的入口点。
//
// 本范例将flv文件的视频编码转换为H264的MP4文件

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
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
};
#endif

#include <atlconv.h>


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		printf("GMVideoEncode.exe <yuv_url> <codec_name>\n");
		return 0;
	}

	TCHAR yuv_url[4096] = {0};
	_tcscpy_s(yuv_url, 4096, argv[1]);

	TCHAR codec_name[4096] = {0};
	_tcscpy_s(codec_name, 4096, argv[2]);

	av_register_all();
	USES_CONVERSION;

	//////////////////////////////////////////////////////////////////////////
	//
	// 首先寻找编码器
	//
	//////////////////////////////////////////////////////////////////////////

	AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_HEVC);
	if (!encoder)
		return -1;

	AVCodecContext *encode_codec_context = avcodec_alloc_context3(encoder);
	if (!encode_codec_context)
		return -2;

	AVPacket *av_packet = av_packet_alloc();
	if (!av_packet)
		return -3;

	// 设置参数
	encode_codec_context->bit_rate = 400000;

	encode_codec_context->width = 352;
	encode_codec_context->height = 288;

	encode_codec_context->time_base.num = 1;
	encode_codec_context->time_base.den = 25;

	encode_codec_context->framerate.num = 25;
	encode_codec_context->framerate.den = 1;

	encode_codec_context->gop_size = 10;
	encode_codec_context->max_b_frames = 1;
	encode_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

	if (encoder->id == AV_CODEC_ID_H264)
		av_opt_set(encode_codec_context->priv_data, "preset", "slow", 0);

	int errCode = avcodec_open2(encode_codec_context, encoder, NULL);
	if (errCode < 0)
		return -4;
	
	FILE *file = fopen(T2A(yuv_url), "wb");
	if (!file)
		return -5;

	AVFrame *av_frame = av_frame_alloc();
	if (!av_frame)
		return -6;

	av_frame->format = encode_codec_context->pix_fmt;
	av_frame->width = encode_codec_context->width;
	av_frame->height = encode_codec_context->height;

	errCode = av_frame_get_buffer(av_frame, 32);
	if (errCode < 0)
		return -7;

	for (int index = 0; index < 25; ++index)
	{
		fflush(stdout);

		errCode = av_frame_make_writable(av_frame);
		if (errCode < 0)
			return -8;

		// Y
		for (int y = 0; y < encode_codec_context->height; ++y)
		{
			for (int x = 0; x < encode_codec_context->width; ++x)
			{
				av_frame->data[0][y * av_frame->linesize[0] + x] = x + y + index * 3;
			}
		}

		// Cb and Cr
		for (int y = 0; y < encode_codec_context->height / 2; ++y)
		{
			for (int x = 0; x < encode_codec_context->width / 2; ++x)
			{
				av_frame->data[1][y * av_frame->linesize[1] + x] = 128 + y + index * 2;
				av_frame->data[2][y * av_frame->linesize[2] + x] = 64 + y + index * 5;
			}
		}

		av_frame->pts = index;

		// 编码
	}

	return 0;
}

