// GMAudioDecode.cpp : 定义控制台应用程序的入口点。
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
#ifdef __cplusplus
};
#endif

void ShowFormatInfo(AVInputFormat *input_format)
{
	
}

void ShowAudioInfo()
{

}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		printf("GMAudioDecode.exe [audio_path]\n");
		return 0;
	}

	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	TCHAR audio_path[4096] = {0};
	_tcscpy_s(audio_path, 4096, argv[1]);

	USES_CONVERSION;
	AVFormatContext *format_context = NULL;
	int errCode = avformat_open_input(&format_context, T2A(audio_path), NULL, NULL);
	if (errCode != 0)
	{
		printf("avformat_open_input failed.\n");
		return errCode;
	}

	avformat_find_stream_info(format_context, NULL);

	// 查找音频流
	int audio_stream_index = -1;
	for (int index = 0; index = format_context->nb_streams; ++index)
	{
		if (format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index = index;
			break;
		}
	}

	if (audio_stream_index < 0)
	{
		printf("There isn't a audio stream in this file ...\n");
		return -2;
	}

	AVStream *audio_stream = format_context->streams[audio_stream_index];
	AVCodecContext *audio_codec_context = audio_stream->codec;

	// 根据解码器ID查找解码器，并打开
	AVCodec *audio_codec = avcodec_find_decoder(audio_stream->codec->codec_id);
	if (audio_codec == NULL)
	{
		printf("Not found decoder ...\n");
		return -2;
	}

	errCode = avcodec_open2(audio_codec_context, audio_codec, NULL);
	if (errCode != 0)
	{
		printf("Open decoder failed ...\n");
		return -5;
	}

	// 读取编码包，执行解码
	AVPacket av_packet;
	AVFrame *av_frame = av_frame_alloc();

	bool is_print_info = false;
	while (av_read_frame(format_context, &av_packet) == 0)
	{
		if (av_packet.stream_index != audio_stream_index)
		{
			// 不是音频数据流中的包
			continue;
		}

		// 输出编码包相关信息

		// 解码
		int got_frame_ptr = 0;
		errCode = avcodec_decode_audio4(audio_codec_context, av_frame, &got_frame_ptr, &av_packet);

		if (!got_frame_ptr)
			continue;

		// 解码完成，这里可以输出音频数据了
		if (!is_print_info)
		{
			is_print_info = true;
		}
	}

	av_frame_free(&av_frame);
	return 0;
}

