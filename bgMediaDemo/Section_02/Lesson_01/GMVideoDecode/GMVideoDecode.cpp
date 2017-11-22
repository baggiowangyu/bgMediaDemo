// GMVideoDecode.cpp : 定义控制台应用程序的入口点。
//
// 本范例将FLV视频文件解码为YUV文件和PCM文件

//
// 2017-11-15 测试
// 1. yuv文件使用yuv播放器勉强能播，颜色不太对，帧率控制的也不太好
// 2. pcm文件播放时一塌糊涂
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

#include <iostream>

#define MAX_AUDIO_FRAME_SIZE 192000


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 4)
	{
		printf("GMVideoDecode.exe <video_in_path> <yuv_path> <pcm_path>\n");
		return 0;
	}

	// 完成初始化注册
	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	TCHAR video_in_path[4096] = {0};
	_tcscpy_s(video_in_path, 4096, argv[1]);

	TCHAR yuv_out_path[4096] = {0};
	_tcscpy_s(yuv_out_path, 4096, argv[2]);

	TCHAR pcm_out_path[4096] = {0};
	_tcscpy_s(pcm_out_path, 4096, argv[3]);

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
	int input_audio_stream_index = -1;
	AVStream *input_video_stream = NULL;
	AVStream *input_audio_stream = NULL;
	AVCodecContext *input_video_codec_context = NULL;
	AVCodecContext *input_audio_codec_context = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			input_audio_stream_index	= index;
			input_audio_stream			= input_format_context->streams[index];
			input_audio_codec_context	= input_audio_stream->codec;
		}
		else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
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
	AVCodec *input_audio_decoder = NULL;
	if (input_video_codec_context >= 0)
	{
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
	}
	
	if (input_audio_stream_index >= 0)
	{
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
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 视频图像转换的前期操作，准备转换为YUV文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFrame *video_frame_yuv = av_frame_alloc();

	int video_frame_yuv_buffer_size = avpicture_get_size(AV_PIX_FMT_YUV420P, input_video_codec_context->width, input_video_codec_context->height);
	unsigned char *video_frame_yuv_buffer = (unsigned char *)av_malloc(video_frame_yuv_buffer_size);
	avpicture_fill((AVPicture *)video_frame_yuv, video_frame_yuv_buffer, AV_PIX_FMT_YUV420P, input_video_codec_context->width, input_video_codec_context->height);

	struct SwsContext *image_convert_context = sws_getContext(input_video_codec_context->width, input_video_codec_context->height, input_video_codec_context->pix_fmt,
		input_video_codec_context->width, input_video_codec_context->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	//////////////////////////////////////////////////////////////////////////
	//
	// 音频转换的前期操作，将音频转换为PCM文件
	//
	//////////////////////////////////////////////////////////////////////////
	uint64_t pcm_channel_layout = AV_CH_LAYOUT_STEREO;
	int pcm_frame_size = input_audio_codec_context->frame_size;	// AAC是1024；MP3是1152；
	AVSampleFormat pcm_sample_format = AV_SAMPLE_FMT_S16;
	int pcm_sample_rate = input_audio_codec_context->sample_rate;	// 44100
	int pcm_channels = av_get_channel_layout_nb_channels(pcm_channel_layout);
	int pcm_buffer_size = av_samples_get_buffer_size(NULL, pcm_channels, pcm_frame_size, pcm_sample_format, 1);

	unsigned char *pcm_frame_buffer = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	// 
	int input_audio_channel_layout = av_get_default_channel_layout(input_audio_codec_context->channels);

	// 
	SwrContext *audio_convert_context = swr_alloc();
	audio_convert_context = swr_alloc_set_opts(audio_convert_context, pcm_channel_layout, pcm_sample_format, pcm_sample_rate,
		input_audio_channel_layout, input_audio_codec_context->sample_fmt, input_audio_codec_context->sample_rate, 0, NULL);
	swr_init(audio_convert_context);

	//////////////////////////////////////////////////////////////////////////
	//
	// 读取编码包，解码，写入对应文件
	//
	//////////////////////////////////////////////////////////////////////////

	FILE *yuv_file_handle = fopen(T2A(yuv_out_path), "w");
	if (yuv_file_handle == NULL)
	{
		printf("Open \"%s\" failed...\n", T2A(yuv_out_path));
		return -5;
	}

	FILE *pcm_file_handle = fopen(T2A(pcm_out_path), "w");
	if (yuv_file_handle == NULL)
	{
		printf("Open \"%s\" failed...\n", T2A(pcm_out_path));
		return -5;
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

			printf("完成视频帧解码。帧pts：%d\n", av_frame->pts);

			sws_scale(image_convert_context, (const unsigned char* const*)av_frame->data, av_frame->linesize, 0,
				input_video_codec_context->height, video_frame_yuv->data, video_frame_yuv->linesize);

			int y_size = input_video_codec_context->width * input_video_codec_context->height;
			fwrite(video_frame_yuv->data[0], 1, y_size, yuv_file_handle);		// Y
			fwrite(video_frame_yuv->data[1], 1, y_size / 4, yuv_file_handle);	// U
			fwrite(video_frame_yuv->data[2], 1, y_size / 4, yuv_file_handle);	// V

			//// 一种新的yuv文件写入方式
			//// Y
			//for (int index = 0; index < av_frame->height; ++index)
			//	fwrite(video_frame_yuv->data[0] + av_frame->linesize[0] * index, 1, av_frame->width, yuv_file_handle);

			//// U
			//for (int index = 0; index < av_frame->height / 2; ++index)
			//	fwrite(video_frame_yuv->data[1] + av_frame->linesize[1] * index, 1, av_frame->width / 2, yuv_file_handle);

			//// V
			//for (int index = 0; index < av_frame->height / 2; ++index)
			//	fwrite(video_frame_yuv->data[2] + av_frame->linesize[2] * index, 1, av_frame->width / 2, yuv_file_handle);
		}
		else if (av_packet.stream_index == input_audio_stream_index)
		{
			// 音频数据，解码后转换声音格式，写入文件
			int got_frame_ptr = 0;
			errCode = avcodec_decode_audio4(input_audio_codec_context, av_frame, &got_frame_ptr, &av_packet);
			if (errCode < 0)
			{
				printf("Decode audio packet failed...%d\n", errCode);
				av_frame_free(&av_frame);
				return errCode;
			}

			if (!got_frame_ptr)
			{
				av_frame_free(&av_frame);
				continue;
			}

			swr_convert(audio_convert_context, &pcm_frame_buffer, MAX_AUDIO_FRAME_SIZE, (const unsigned char **)av_frame->data, av_frame->nb_samples);

			fwrite(pcm_frame_buffer, 1, pcm_buffer_size, pcm_file_handle);
		}

		av_frame_free(&av_frame);
	}

	// 解码完毕之后，将Codec中剩下的帧刷入文件
	while (true)
	{
		int got_pic = 0;
		AVFrame *av_frame = av_frame_alloc();

		errCode = avcodec_decode_video2(input_video_codec_context, av_frame, &got_pic, &av_packet);

		if (errCode < 0)
			break;
		
		if (!got_pic)
			break;

		int y_size = input_video_codec_context->width * input_video_codec_context->height;
		fwrite(video_frame_yuv->data[0], 1, y_size, yuv_file_handle);		// Y
		fwrite(video_frame_yuv->data[1], 1, y_size / 4, yuv_file_handle);	// U
		fwrite(video_frame_yuv->data[2], 1, y_size / 4, yuv_file_handle);	// V

		//// 一种新的yuv文件写入方式
		//// Y
		//for (int index = 0; index < av_frame->height; ++index)
		//	fwrite(video_frame_yuv->data[0] + av_frame->linesize[0] * index, 1, av_frame->width, yuv_file_handle);

		//// U
		//for (int index = 0; index < av_frame->height / 2; ++index)
		//	fwrite(video_frame_yuv->data[1] + av_frame->linesize[1] * index, 1, av_frame->width / 2, yuv_file_handle);

		//// V
		//for (int index = 0; index < av_frame->height / 2; ++index)
		//	fwrite(video_frame_yuv->data[2] + av_frame->linesize[2] * index, 1, av_frame->width / 2, yuv_file_handle);
	}

	sws_freeContext(image_convert_context);
	swr_free(&audio_convert_context);

	fclose(yuv_file_handle);
	fclose(pcm_file_handle);
	
	av_frame_free(&video_frame_yuv);
	
	avcodec_close(input_video_codec_context);
	avcodec_close(input_audio_codec_context);

	avformat_close_input(&input_format_context);

	return 0;
}

