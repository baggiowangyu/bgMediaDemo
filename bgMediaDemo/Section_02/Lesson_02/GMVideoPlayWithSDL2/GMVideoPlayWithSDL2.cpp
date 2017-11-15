// GMVideoPlayWithSDL2.cpp : 定义控制台应用程序的入口点。
//
// 本范例用于解码播放视频文件中的视频流

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


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 4)
	{
		printf("GMVideoPlayWithSDL2.exe <video_in_path>\n");
		return 0;
	}

	// 完成初始化注册
	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	TCHAR video_in_path[4096] = {0};
	_tcscpy_s(video_in_path, 4096, argv[1]);

	USES_CONVERSION;
	AVFormatContext *input_format_context = NULL;
	int errCode = avformat_open_input(&input_format_context, T2A(video_in_path), NULL, NULL);
	if (errCode != 0)
	{
		printf("Open video file failed...\n");
		return errCode;
	}

	errCode = avformat_find_stream_info(input_format_context, NULL);
	if (errCode < 0)
	{
		printf("Find video's stream information failed...\n");
		avformat_close_input(&input_format_context);
		return errCode;
	}

	int input_video_stream_index = -1;
	AVStream *input_video_stream = NULL;
	AVCodecContext *input_video_codec_context = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			input_video_stream_index	= index;
			input_video_stream			= input_format_context->streams[index];
			input_video_codec_context	= input_video_stream->codec;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备几种流的解码器
	//
	//////////////////////////////////////////////////////////////////////////
	AVCodec *input_video_decoder = NULL;
	input_video_decoder = avcodec_find_decoder(input_video_codec_context->codec_id);
	if (input_video_decoder == NULL)
	{
		printf("Not found video decoder...\n");
		return -2;
	}

	errCode = avcodec_open2(input_video_codec_context, input_video_decoder, NULL);
	if (errCode != 0)
	{
		printf("Open video decoder failed. %d\n", errCode);
		return errCode;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 视频图像转换的前期操作，准备转换为YUV文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFrame *video_frame_yuv = av_frame_alloc();

	int video_frame_yuv_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, input_video_codec_context->width, input_video_codec_context->height, 1);
	unsigned char *video_frame_yuv_buffer = (unsigned char *)av_malloc(video_frame_yuv_buffer_size);
	av_image_fill_arrays(video_frame_yuv->data, video_frame_yuv->linesize, video_frame_yuv_buffer, AV_PIX_FMT_YUV420P,
		input_video_codec_context->width, input_video_codec_context->height, 1);

	struct SwsContext *image_convert_context = sws_getContext(input_video_codec_context->width, input_video_codec_context->height, input_video_codec_context->pix_fmt,
		input_video_codec_context->width, input_video_codec_context->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	//////////////////////////////////////////////////////////////////////////
	//
	// 前面视频解码的东西都准备好了，这里准备SDL的东西
	//
	//////////////////////////////////////////////////////////////////////////
	int source_width = input_video_codec_context->width;
	int source_height = input_video_codec_context->height;
	int screen_width = source_width;
	int screen_height = source_height;

	errCode = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	if (errCode != 0)
		return errCode;

	SDL_Window *sdl_window_ = SDL_CreateWindow("GMVideoPlayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, source_width, source_height, SDL_WINDOW_OPENGL);
	if (!sdl_window_)
	{
		printf("\n");
		return -1;
	}

	SDL_Renderer *sdl_renderer_ = SDL_CreateRenderer(sdl_window_, -1, 0);
	if (!sdl_renderer_)
	{
		printf("\n");
		return -2;
	}

	SDL_Texture *sdl_texture_ = SDL_CreateTexture(sdl_renderer_, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, source_width, source_height);
	if (!sdl_texture_)
	{
		printf("\n");
		return -2;
	}


	AVPacket av_packet;
	while (true)
	{
		errCode = av_read_frame(input_format_context, &av_packet);
		if (errCode < 0)
			break;

		AVFrame *av_frame = av_frame_alloc();
		if (av_packet.stream_index == input_video_stream_index)
		{
			// 视频数据，解码后转换图像格式，写入文件
			int got_pic = 0;
			errCode = avcodec_decode_video2(input_video_codec_context, av_frame, &got_pic, &av_packet);
			if (errCode < 0)
			{
				printf("Decode video packet failed...%d\n", errCode);
				av_frame_free(&av_frame);
				return errCode;
			}

			if (!got_pic)
			{
				av_frame_free(&av_frame);
				continue;
			}

			sws_scale(image_convert_context, (const unsigned char* const*)av_frame->data, av_frame->linesize, 0,
				input_video_codec_context->height, video_frame_yuv->data, video_frame_yuv->linesize);
		}

		av_frame_free(&av_frame);
	}

	sws_freeContext(image_convert_context);
	av_frame_free(&video_frame_yuv);
	avcodec_close(input_video_codec_context);
	avformat_close_input(&input_format_context);

	return 0;
}

