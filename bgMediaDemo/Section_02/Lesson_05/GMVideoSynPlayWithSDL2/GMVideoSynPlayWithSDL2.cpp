// GMVideoSynPlayWithSDL2.cpp : 定义控制台应用程序的入口点。
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
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
};
#endif

#include "SDL.h"
#include <iostream>

#define MAX_AUDIO_FRAME_SIZE 192000


int audio_len = 0;
unsigned char *audio_pos = NULL;
unsigned char *audio_chunk = NULL;

void audio_fill_callback(void *udata, Uint8 *stream, int len)
{
	SDL_memset(stream, 0, len);

	if(audio_len == 0)
		return;

	len = (len > audio_len ? audio_len : len);

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}


int frame_rate = 25;
HANDLE event_handle = NULL;
DWORD WINAPI video_play_control(LPVOID lpParam)
{
	// 计算一下平均帧率下需要Delay的时间
	int delay_time = 1000 / frame_rate;
	while (true)
	{
		SetEvent(event_handle);
		Sleep(delay_time);		// 这里由帧率来控制
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
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

	int input_audio_stream_index = -1;
	AVStream *input_audio_stream = NULL;
	AVCodecContext *input_audio_codec_context = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			input_video_stream_index	= index;
			input_video_stream			= input_format_context->streams[index];
			input_video_codec_context	= input_video_stream->codec;
		}
		else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			input_audio_stream_index	= index;
			input_audio_stream			= input_format_context->streams[index];
			input_audio_codec_context	= input_audio_stream->codec;
		}
	}

	// 第一种帧率
	frame_rate = av_q2d(input_video_stream->r_frame_rate);
	if (frame_rate == 0)
	{
		// 计算第二种帧率
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备几种流的解码器
	//
	//////////////////////////////////////////////////////////////////////////

	// 视频解码器
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

	// 音频解码器
	AVCodec *input_audio_decoder = NULL;
	input_audio_decoder = avcodec_find_decoder(input_audio_codec_context->codec_id);
	if (input_audio_decoder == NULL)
	{
		printf("Not found audio decoder...\n");
		return -2;
	}

	errCode = avcodec_open2(input_audio_codec_context, input_audio_decoder, NULL);
	if (errCode != 0)
	{
		printf("Open audio decoder failed. %d\n", errCode);
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
	// 音频
	//
	//////////////////////////////////////////////////////////////////////////
	// 准备解码之前，先设定好PCM转参数
	uint64_t output_channel_layout = AV_CH_LAYOUT_STEREO;
	int output_frame_size = input_audio_codec_context->frame_size;	// AAC是1024；MP3是1152；
	AVSampleFormat output_sample_format = AV_SAMPLE_FMT_S16;
	int output_sample_rate = input_audio_codec_context->sample_rate;	// 44100
	int output_channels = av_get_channel_layout_nb_channels(output_channel_layout);
	int output_buffer_size = av_samples_get_buffer_size(NULL, output_channels, output_frame_size, output_sample_format, 1);

	unsigned char *output_buffer = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	// 
	int input_channel_layout = av_get_default_channel_layout(input_audio_codec_context->channels);

	// 
	SwrContext *audio_convert_context = swr_alloc();
	audio_convert_context = swr_alloc_set_opts(audio_convert_context, output_channel_layout, output_sample_format, output_sample_rate,
		input_channel_layout, input_audio_codec_context->sample_fmt, input_audio_codec_context->sample_rate, 0, NULL);
	swr_init(audio_convert_context);

	// SDL的音频播放参数，实际上跟解密有点关系
	SDL_AudioSpec audio_spec;
	audio_spec.freq = output_sample_rate;
	audio_spec.format = AUDIO_S16SYS;
	audio_spec.channels = output_channels;
	audio_spec.silence = 0;
	audio_spec.samples = output_frame_size;
	audio_spec.callback = audio_fill_callback;

	//////////////////////////////////////////////////////////////////////////
	//
	// 前面视频解码的东西都准备好了，这里准备SDL的东西
	//
	//////////////////////////////////////////////////////////////////////////
	int source_width	= input_video_codec_context->width;
	int source_height	= input_video_codec_context->height;
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

	// SDL打开音频通道
	errCode = SDL_OpenAudio(&audio_spec, NULL);
	if (errCode < 0)
	{
		printf("Can't open audio...\n");
		return errCode;
	}

	// Play
	SDL_PauseAudio(0);

	event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThread(NULL, 0, video_play_control, NULL, 0, NULL);

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_width;
	rect.h = screen_height;


	AVPacket av_packet;
	while (true)
	{
		

		errCode = av_read_frame(input_format_context, &av_packet);
		if (errCode < 0)
			break;

		AVFrame *av_frame = av_frame_alloc();
		if (av_packet.stream_index == input_video_stream_index)
		{
			WaitForSingleObject(event_handle, INFINITE);

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

			// 图像格式转换
			sws_scale(image_convert_context, (const unsigned char* const*)av_frame->data, av_frame->linesize, 0,
				input_video_codec_context->height, video_frame_yuv->data, video_frame_yuv->linesize);

			SDL_UpdateTexture(sdl_texture_, NULL, video_frame_yuv->data[0], video_frame_yuv->linesize[0]);
			SDL_RenderCopy(sdl_renderer_, sdl_texture_, NULL, &rect);
			SDL_RenderPresent(sdl_renderer_);
		}
		else if (av_packet.stream_index == input_audio_stream_index)
		{
			// 解码音频编码包
			int got_frame_ptr = 0;
			errCode = avcodec_decode_audio4(input_audio_codec_context, av_frame, &got_frame_ptr, &av_packet);

			if (!got_frame_ptr)
				continue;

			// 执行转换
			swr_convert(audio_convert_context, &output_buffer, MAX_AUDIO_FRAME_SIZE, (const unsigned char **)av_frame->data, av_frame->nb_samples);

			while (audio_len > 0)
				SDL_Delay(1);

			audio_chunk = (unsigned char *)output_buffer;
			audio_len = output_buffer_size;
			audio_pos = audio_chunk;
		}

		av_frame_free(&av_frame);
	}

	sws_freeContext(image_convert_context);
	av_frame_free(&video_frame_yuv);
	avcodec_close(input_video_codec_context);
	avformat_close_input(&input_format_context);

	return 0;
}

