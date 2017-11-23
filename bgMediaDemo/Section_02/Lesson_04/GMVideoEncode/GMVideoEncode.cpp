// GMVideoEncode.cpp : 定义控制台应用程序的入口点。
//
// 本范例将yuv文件的视频编码转换为H264/HEVC(H265)的h264/hevc(h265)文件

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


int flush_encoder(AVFormatContext *output_format_context, unsigned int stream_index)
{
	if (!(output_format_context->streams[stream_index]->codec->codec->capabilities & CODEC_CAP_DELAY))
		return 0;

	int errCode = 0;
	AVPacket encode_packet;
	while (true)
	{
		encode_packet.data = NULL;
		encode_packet.size = 0;
		av_init_packet(&encode_packet);

		int got_data = 0;
		errCode = avcodec_encode_video2(output_format_context->streams[stream_index]->codec, &encode_packet, NULL, &got_data);

		av_frame_free(NULL);

		if (errCode < 0)
			break;

		if (!got_data)
		{
			errCode = 0;
			break;
		}

		printf("Flush encoder succeed...\n");

		errCode = av_write_frame(output_format_context, &encode_packet);
		if (errCode < 0)
			break;
	}

	return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 6)
	{
		printf("GMVideoEncode.exe <yuv_url> <yuv_width> <yuv_height> <codec_name> <output_url>\n\n");
		printf("\tcodec_name : [ H264 | H265 ]");
		return 0;
	}

	TCHAR yuv_url[4096] = {0};
	_tcscpy_s(yuv_url, 4096, argv[1]);

	TCHAR yuv_width_str[4096] = {0};
	_tcscpy_s(yuv_width_str, 4096, argv[2]);
	int yuv_width = _ttoi(yuv_width_str);

	TCHAR yuv_height_str[4096] = {0};
	_tcscpy_s(yuv_height_str, 4096, argv[3]);
	int yuv_height = _ttoi(yuv_height_str);

	TCHAR codec_name[4096] = {0};
	_tcscpy_s(codec_name, 4096, argv[4]);

	TCHAR output_url[4096] = {0};
	_tcscpy_s(output_url, 4096, argv[5]);

	av_register_all();
	USES_CONVERSION;

	//////////////////////////////////////////////////////////////////////////
	//
	// 首先打开yuv文件
	//
	//////////////////////////////////////////////////////////////////////////

	FILE *yuv_file = fopen(T2A(yuv_url), "rb");
	if (!yuv_file)
	{
		printf("Open yuv file failed...\n");
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输出文件
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *output_format_context = NULL;
	int errCode = avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(output_url));
	if (errCode < 0)
	{
		printf("Alloc output format context failed...%d\n", errCode);
		return errCode;
	}

	AVOutputFormat *output_format = output_format_context->oformat;

	errCode = avio_open(&output_format_context->pb, T2A(output_url), AVIO_FLAG_READ_WRITE);
	if (errCode < 0)
	{
		printf("Open output file failed...%d\n", errCode);
		return errCode;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 创建输出视频流
	//
	//////////////////////////////////////////////////////////////////////////

	AVStream *encode_video_stream = avformat_new_stream(output_format_context, NULL);
	if (encode_video_stream == NULL)
	{
		printf("Alloc encode video stream failed...\n");
		return -2;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 设置编码上下文参数
	//
	//////////////////////////////////////////////////////////////////////////

	// 首先确认编码器ID，如果指定的编码器ID未知，那么编码器ID使用默认的
	AVCodecID encoder_id = output_format->video_codec;

	if (_tcsicmp(codec_name, _T("H264")) == 0)
		encoder_id = AV_CODEC_ID_H264;
	else if (_tcsicmp(codec_name, _T("H265")) == 0)
		encoder_id = AV_CODEC_ID_H265;

	AVCodecContext *encode_video_codec_context = encode_video_stream->codec; 

	encode_video_codec_context->codec_id		= encoder_id;
	encode_video_codec_context->codec_type		= AVMEDIA_TYPE_VIDEO;
	encode_video_codec_context->pix_fmt			= AV_PIX_FMT_YUV420P;
	encode_video_codec_context->width			= yuv_width;
	encode_video_codec_context->height			= yuv_height;
	encode_video_codec_context->bit_rate		= 400000;		// 默认400000的码流？
	encode_video_codec_context->gop_size		= 250;			// 一般都是12，范例给的是250
	
	encode_video_codec_context->time_base.num	= 1;
	encode_video_codec_context->time_base.den	= 15;		// 这里决定了帧率是15

	encode_video_codec_context->qmin			= 10;
	encode_video_codec_context->qmax			= 51;

	encode_video_codec_context->max_b_frames	= 3;

	//if (encode_video_codec_context->codec_id == AV_CODEC_ID_H264)
	//{
	//	encode_video_codec_context->me_range	= 16;
	//	encode_video_codec_context->max_qdiff	= 4;
	//	encode_video_codec_context->qcompress	= 0.6;
	//}

	AVDictionary *param = NULL;
	if (encode_video_codec_context->codec_id == AV_CODEC_ID_H264)
	{
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
	}
	else if (encode_video_codec_context->codec_id == AV_CODEC_ID_H265)
	{
		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}

	av_dump_format(output_format_context, NULL, T2A(output_url), 1);

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开编码器
	//
	//////////////////////////////////////////////////////////////////////////

	AVCodec *video_encoder = avcodec_find_encoder(encode_video_codec_context->codec_id);
	if (!video_encoder)
	{
		printf("Not found video encoder...\n");
		return -3;
	}

	errCode = avcodec_open2(encode_video_codec_context, video_encoder, &param);
	if (errCode < 0)
	{
		printf("Open video encoder failed...%d\n", errCode);
		return errCode;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备图像转换相关的结构、以及相关初始化工作
	//
	//////////////////////////////////////////////////////////////////////////

	AVFrame *video_frame = av_frame_alloc();

	int picture_size = avpicture_get_size(encode_video_codec_context->pix_fmt, encode_video_codec_context->width, encode_video_codec_context->height);
	uint8_t *picture_buffer = (uint8_t *)av_malloc(picture_size);
	avpicture_fill((AVPicture *)video_frame->data, picture_buffer, encode_video_codec_context->pix_fmt, encode_video_codec_context->width, encode_video_codec_context->height);

	//////////////////////////////////////////////////////////////////////////
	//
	// 进行编码
	//
	//////////////////////////////////////////////////////////////////////////

	// 写文件头
	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
	{
		printf("Write file header failed...%d\n");
		return errCode;
	}

	AVPacket video_packet;
	av_new_packet(&video_packet, picture_size);

	int y_size = encode_video_codec_context->width * encode_video_codec_context->height;

	int index = 0;
	while (true)
	{
		// 从文件中读取YUV数据
		errCode = fread(picture_buffer, 1, y_size * 3 / 2, yuv_file);
		if (errCode <= 0)
		{
			printf("Read yuv file failed...%d\n", errCode);
			break;
		}
		else if (feof(yuv_file))
			break;

		video_frame->data[0] = picture_buffer;					// Y
		video_frame->data[1] = picture_buffer + y_size;			// U
		video_frame->data[2] = picture_buffer + y_size * 5 / 4;	// V

		// 设置pts
		video_frame->pts = index * (encode_video_stream->time_base.den) / ((encode_video_stream->time_base.num) * 25);

		// 编码
		int got_pic = 0;
		errCode = avcodec_encode_video2(encode_video_codec_context, &video_packet, video_frame, &got_pic);
		if (errCode < 0)
		{
			printf("Encode video failed...%d\n", errCode);
			return errCode;
		}

		if (got_pic)
		{
			printf("Encode video frame %d success...\n", index);
			video_packet.stream_index = encode_video_stream->index;

			errCode = av_write_frame(output_format_context, &video_packet);
			av_free_packet(&video_packet);
		}

		++index;
	}

	// 刷新编码器
	errCode = flush_encoder(output_format_context, 0);
	if (errCode < 0)
	{
		printf("Flush encoder failed...%d\n", errCode);
		return errCode;
	}

	av_write_trailer(output_format_context);

	if (encode_video_stream)
	{
		avcodec_close(encode_video_stream->codec);
		av_free(video_frame);
		av_free(picture_buffer);
	}

	avio_close(output_format_context->pb);
	avformat_free_context(output_format_context);

	fclose(yuv_file);

	return 0;
}

