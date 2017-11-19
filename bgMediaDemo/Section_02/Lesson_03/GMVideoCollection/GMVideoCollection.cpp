// GMVideoCollection.cpp : 定义控制台应用程序的入口点。
//
// 本范例从接入系统的USB摄像头采集视频数据
//
// 1.遍历当前系统中的所有摄像头设备
// 2.从摄像头中取出数据
// 3.解码显示到SDL2上
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

#include "SDL.h"
#include <iostream>


HANDLE event_handle = NULL;
DWORD WINAPI video_play_control(LPVOID lpParam)
{
	while (true)
	{
		SetEvent(event_handle);
		//Sleep(40);		// 这里由帧率来控制
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	av_register_all();
	avformat_network_init();
	avdevice_register_all();
	avcodec_register_all();

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开摄像头
	//
	//////////////////////////////////////////////////////////////////////////

	AVFormatContext *device_format_context = avformat_alloc_context();
	AVInputFormat *device_input_format = av_find_input_format("vfwcap");

	int errCode = avformat_open_input(&device_format_context, "0", device_input_format, NULL);
	if (errCode != 0)
	{
		std::cout<<"打开摄像头失败！"<<std::endl;
		return -1;
	}

	errCode = avformat_find_stream_info(device_format_context, NULL);
	if (errCode < 0)
	{
		std::cout<<"没有找到流信息！"<<std::endl;
		return -1;
	}

	int video_stream_index = -1;
	int audio_stream_index = -1;

	for (int index = 0; index < device_format_context->nb_streams; ++index)
	{
		switch (device_format_context->streams[index]->codec->codec_type)
		{
		case AVMEDIA_TYPE_VIDEO:
			video_stream_index = index;
			break;
		case AVMEDIA_TYPE_AUDIO:
			audio_stream_index = index;
			break;
		default:
			break;
		}
	}

	AVCodecContext *video_codec_context = device_format_context->streams[video_stream_index]->codec;
	AVCodecContext *audio_codec_context = device_format_context->streams[audio_stream_index]->codec;

	AVCodec *video_codec = avcodec_find_decoder(video_codec_context->codec_id);
	if (video_codec == NULL)
	{
		std::cout<<"没有找到视频解码器！"<<std::endl;
		return -2;
	}

	errCode = avcodec_open2(video_codec_context, video_codec, NULL);
	if (errCode < 0)
	{
		std::cout<<"打开视频解码器失败！"<<std::endl;
		return -3;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 视频图像转换的前期操作，准备转换为YUV文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFrame *video_frame_yuv = av_frame_alloc();

	int video_frame_yuv_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, video_codec_context->width, video_codec_context->height, 1);
	unsigned char *video_frame_yuv_buffer = (unsigned char *)av_malloc(video_frame_yuv_buffer_size);
	av_image_fill_arrays(video_frame_yuv->data, video_frame_yuv->linesize, video_frame_yuv_buffer, AV_PIX_FMT_YUV420P,
		video_codec_context->width, video_codec_context->height, 1);

	struct SwsContext *image_convert_context = sws_getContext(video_codec_context->width, video_codec_context->height, video_codec_context->pix_fmt,
		video_codec_context->width, video_codec_context->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	//////////////////////////////////////////////////////////////////////////
	//
	// 前面视频解码的东西都准备好了，这里准备SDL的东西
	//
	//////////////////////////////////////////////////////////////////////////
	int source_width	= video_codec_context->width;
	int source_height	= video_codec_context->height;
	int screen_width	= source_width;
	int screen_height	= source_height;

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

	event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThread(NULL, 0, video_play_control, NULL, 0, NULL);

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_width;
	rect.h = screen_height;

	//////////////////////////////////////////////////////////////////////////
	//
	// 开始读取摄像头的视频数据
	//
	//////////////////////////////////////////////////////////////////////////
	AVPacket av_packet;
	while (true)
	{
		errCode = av_read_frame(device_format_context, &av_packet);
		if (errCode < 0)
			break;

		int got_pic = 0;
		AVFrame *av_frame = av_frame_alloc();
		errCode = avcodec_decode_video2(video_codec_context, av_frame, &got_pic, &av_packet);
		if (errCode < 0)
			continue;

		if (!got_pic)
			continue;

		// 图像格式转换
		sws_scale(image_convert_context, (const unsigned char* const*)av_frame->data, av_frame->linesize, 0,
			video_codec_context->height, video_frame_yuv->data, video_frame_yuv->linesize);

		SDL_UpdateTexture(sdl_texture_, NULL, video_frame_yuv->data[0], video_frame_yuv->linesize[0]);
		SDL_RenderCopy(sdl_renderer_, sdl_texture_, NULL, &rect);
		SDL_RenderPresent(sdl_renderer_);

		av_frame_free(&av_frame);
	}

	// 释放掉资源

	return 0;
}

