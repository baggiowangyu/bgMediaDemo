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
		printf("GMVideoEncode.exe <yuv_url> <mp4_url>\n");
		return 0;
	}

	TCHAR flv_url[4096] = {0};
	_tcscpy_s(flv_url, 4096, argv[1]);

	TCHAR mp4_url[4096] = {0};
	_tcscpy_s(mp4_url, 4096, argv[2]);

	av_register_all();
	USES_CONVERSION;

	//////////////////////////////////////////////////////////////////////////
	//
	// 首先打开flv文件
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *flv_format_context = NULL;
	int errCode = avformat_open_input(&flv_format_context, T2A(flv_url), NULL, NULL);
	if (errCode != 0)
	{
		return errCode;
	}

	errCode = avformat_find_stream_info(flv_format_context, NULL);
	if (errCode < 0)
	{
		return errCode;
	}

	int flv_video_stream_index = -1;

	for (int index = 0; index < flv_format_context->nb_streams; ++index)
	{
		if (flv_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			flv_video_stream_index = index;
		}
	}

	AVStream *flv_video_stream = flv_format_context->streams[flv_video_stream_index];
	AVCodecContext *flv_video_codec_context = flv_video_stream->codec;

	AVCodec *flv_decode_codec = avcodec_find_decoder(flv_video_codec_context->codec_id);
	if (!flv_video_codec_context)
		return -1;

	errCode = avcodec_open2(flv_video_codec_context, flv_decode_codec, NULL);
	if (errCode < 0)
		return errCode;

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备一下要输出的mp4的上下文，以及文件信息
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *mp4_format_context = avformat_alloc_context();
	AVOutputFormat *mp4_output_format = av_guess_format(NULL, T2A(mp4_url), NULL);
	mp4_format_context->oformat = mp4_output_format;

	AVCodec *mp4_codec_encoder = avcodec_find_encoder(mp4_output_format->video_codec);
	if (!mp4_codec_encoder)
	{
		return -2;
	}

	AVStream *mp4_video_stream = avformat_new_stream(mp4_format_context, mp4_codec_encoder);
	if (!mp4_video_stream)
	{
		return -3;
	}

	AVCodecContext *mp4_video_codec_context = mp4_video_stream->codec;
	if (!mp4_video_codec_context)
	{
		return -4;
	}

	// 这里准备一些视频编码上下文相关的参数
	// 这里需要定义一下编码时必须要填写的参数
	// - bit_rate		码率
	// - width			视频宽度
	// - height			视频高度
	// - time_base		时间基准
	// - framerate		帧率
	// - gop_size		间隔大小
	// - pix_fmt		图像格式
	// - max_b_frames	
	mp4_video_codec_context->bit_rate		= flv_video_codec_context->bit_rate;
	mp4_video_codec_context->width			= flv_video_codec_context->width;
	mp4_video_codec_context->height			= flv_video_codec_context->height;
	mp4_video_codec_context->time_base		= flv_video_codec_context->time_base;
	//mp4_video_codec_context->framerate		= flv_video_codec_context->framerate;
	mp4_video_codec_context->gop_size		= flv_video_codec_context->gop_size;
	mp4_video_codec_context->max_b_frames	= flv_video_codec_context->max_b_frames;
	mp4_video_codec_context->pix_fmt		= flv_video_codec_context->pix_fmt;//AV_PIX_FMT_YUV420P;

	if (mp4_output_format->video_codec == AV_CODEC_ID_H264)
		av_opt_set(mp4_video_codec_context->priv_data, "preset", "slow", 0);

	errCode = avcodec_open2(mp4_video_codec_context, mp4_codec_encoder, NULL);
	if (errCode < 0)
	{
		return errCode;
	}

	// 打开输出视频文件
	errCode = avio_open(&mp4_format_context->pb, T2A(mp4_url), AVIO_FLAG_READ_WRITE);
	if (errCode < 0)
	{
		return errCode;
	}

	errCode = avformat_write_header(mp4_format_context, NULL);
	if (errCode < 0)
	{
		return errCode;
	}

	int video_frame_yuv_buffer_size = av_image_get_buffer_size(mp4_video_codec_context->pix_fmt, mp4_video_codec_context->width, mp4_video_codec_context->height, 1);

	//////////////////////////////////////////////////////////////////////////
	//
	// 开始解码，并重编码
	// 经过几轮测试，基本可以确认，这里的转码依然要通过图像转换的方式进行
	//
	//////////////////////////////////////////////////////////////////////////
	AVPacket av_packet;
	while (true)
	{
		errCode = av_read_frame(flv_format_context, &av_packet);
		if (errCode < 0)
			break;

		if (av_packet.stream_index != flv_video_stream_index)
			continue;

		AVFrame *av_frame = av_frame_alloc();
		int got_pic = 0;
		errCode = avcodec_decode_video2(flv_video_codec_context, av_frame, &got_pic, &av_packet);
		if (errCode < 0)
			continue;

		if (!got_pic)
			continue;

		// 这里要给这个packet分配编码内存
		AVPacket encoded_packet;
		av_new_packet(&encoded_packet, video_frame_yuv_buffer_size);
		int got_encoded_pic = 0;
		errCode = avcodec_encode_video2(mp4_video_codec_context, &encoded_packet, av_frame, &got_encoded_pic);
		if (errCode < 0)
			continue;

		if (!got_encoded_pic)
			continue;

		errCode = av_write_frame(mp4_format_context, &av_packet);
		if (errCode < 0)
			printf("write frame to file failed...%d\n", errCode);

		av_frame_free(&av_frame);
	}

	// 将编码上下文剩余的帧数据刷入

	av_write_trailer(mp4_format_context);

	avio_close(mp4_format_context->pb);
	avformat_close_input(&flv_format_context);
	avformat_free_context(mp4_format_context);

	return 0;
}

