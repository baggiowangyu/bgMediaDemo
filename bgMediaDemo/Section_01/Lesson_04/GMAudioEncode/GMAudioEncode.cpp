// GMAudioEncode.cpp : 定义控制台应用程序的入口点。
//
// 本范例，目标是将MP3文件解码为PCM采集数据，然后重新编码位AAC音频文件

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
	#include "libswresample/swresample.h"
#ifdef __cplusplus
};
#endif

#include <iostream>

#define MAX_AUDIO_FRAME_SIZE 192000

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

	USES_CONVERSION;

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开要输出的音频文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFormatContext *output_format_context = avformat_alloc_context();
	AVOutputFormat *output_format = av_guess_format(NULL, T2A(audio_out_path), NULL);
	output_format_context->oformat = output_format;

	int errCode = avio_open(&output_format_context->pb, T2A(audio_out_path), AVIO_FLAG_READ_WRITE);
	if (errCode < 0)
	{
		printf("Open output audio file failed...\n");
		return errCode;
	}

	AVStream *output_audio_stream = avformat_new_stream(output_format_context, NULL);
	if (output_audio_stream == NULL)
	{
		printf("Create output audio stream failed...\n");
		return -2;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入的音频文件
	//
	//////////////////////////////////////////////////////////////////////////
	AVFormatContext *format_context = NULL;
	errCode = avformat_open_input(&format_context, T2A(audio_in_path), NULL, NULL);
	if (errCode != 0)
	{
		printf("avformat_open_input failed.\n");
		return errCode;
	}

	// 初步查找音频信息
	avformat_find_stream_info(format_context, NULL);

	// 查找音频流索引
	int audio_stream_index = -1;
	for (int index = 0; index < format_context->nb_streams; ++index)
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

	//av_dump_format(format_context, 0, T2A(audio_path), 0);

	// 得到音频流，以及编码器上下文
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

	//////////////////////////////////////////////////////////////////////////
	//
	// MP3解码转换为PCM的配置参数
	//
	//////////////////////////////////////////////////////////////////////////
	uint64_t output_channel_layout		= AV_CH_LAYOUT_STEREO;
	int output_frame_size				= audio_codec_context->frame_size;	// AAC是1024；MP3是1152；
	AVSampleFormat output_sample_format	= AV_SAMPLE_FMT_S16;
	int output_sample_rate				= audio_codec_context->sample_rate;	// 44100
	int output_channels					= av_get_channel_layout_nb_channels(output_channel_layout);
	int output_buffer_size				= av_samples_get_buffer_size(NULL, output_channels, output_frame_size, output_sample_format, 1);

	unsigned char *output_buffer = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	// 
	int input_channel_layout = av_get_default_channel_layout(audio_codec_context->channels);

	// 
	SwrContext *audio_convert_context = swr_alloc();
	audio_convert_context = swr_alloc_set_opts(audio_convert_context, output_channel_layout, output_sample_format, output_sample_rate,
		input_channel_layout, audio_codec_context->sample_fmt, audio_codec_context->sample_rate, 0, NULL);
	swr_init(audio_convert_context);

	//////////////////////////////////////////////////////////////////////////
	//
	// 根据输入文件，设置对应的输出文件上下文
	//
	//////////////////////////////////////////////////////////////////////////
	AVCodecContext *output_codec_context = output_audio_stream->codec;
	output_codec_context->codec_id = output_format->audio_codec;
	output_codec_context->codec_type = AVMEDIA_TYPE_AUDIO;
	output_codec_context->sample_fmt = AV_SAMPLE_FMT_S16;
	output_codec_context->sample_rate = audio_codec_context->sample_rate;
	output_codec_context->channel_layout = AV_CH_LAYOUT_STEREO;
	output_codec_context->channels = av_get_channel_layout_nb_channels(output_codec_context->channel_layout);
	output_codec_context->bit_rate = 64000;		// 这里的码率要固定死？

	//////////////////////////////////////////////////////////////////////////
	//
	// 准备好音频编码上下文
	//
	//////////////////////////////////////////////////////////////////////////
	AVCodec *output_codec_encoder = avcodec_find_encoder(output_codec_context->codec_id);
	if (output_codec_encoder == NULL)
	{
		printf("Can't find encoder.\n");
		return -2;
	}

	errCode = avcodec_open2(output_codec_context, output_codec_encoder, NULL);
	if (errCode < 0)
	{
		printf("Open encoder failed..\n");
		return errCode;
	}

	AVFrame *av_encode_frame = av_frame_alloc();
	av_encode_frame->nb_samples = output_codec_context->frame_size;
	av_encode_frame->format = output_codec_context->sample_fmt;

	int encoder_buffer_size = av_samples_get_buffer_size(NULL, output_codec_context->channels,
		output_codec_context->frame_size, output_codec_context->sample_fmt, 1);
	unsigned char *encode_frame_buffer = (unsigned char *)av_malloc(encoder_buffer_size);
	avcodec_fill_audio_frame(av_encode_frame, output_codec_context->channels, output_codec_context->sample_fmt,
		(const unsigned char *)encode_frame_buffer, encoder_buffer_size, 1);

	AVPacket encode_packet;
	av_new_packet(&encode_packet, encoder_buffer_size);

	// 写文件头
	errCode = avformat_write_header(output_format_context, NULL);

	//////////////////////////////////////////////////////////////////////////
	//
	// 读取编码包，解码转换到PCM，再编码到AAC
	//
	//////////////////////////////////////////////////////////////////////////
	AVPacket av_packet;
	AVFrame *av_decode_frame = av_frame_alloc();

	bool is_print_info = false;
	while (av_read_frame(format_context, &av_packet) == 0)
	{
		if (av_packet.stream_index != audio_stream_index)
		{
			// 不是音频数据流中的包，我们放过
			continue;
		}

		// 解码音频编码包
		int got_frame_ptr = 0;
		errCode = avcodec_decode_audio4(audio_codec_context, av_decode_frame, &got_frame_ptr, &av_packet);

		if (!got_frame_ptr)
			continue;

		// 执行转换
		swr_convert(audio_convert_context, &output_buffer, MAX_AUDIO_FRAME_SIZE, (const unsigned char **)av_decode_frame->data, av_decode_frame->nb_samples);

		av_encode_frame->data[0] = output_buffer;
		av_encode_frame->pts = av_decode_frame->pts;

		
		int got_encode_frame = 0;
		errCode = avcodec_encode_audio2(output_codec_context, &encode_packet, av_encode_frame, &got_encode_frame);
		if (errCode < 0)
		{
		}
	}

	avcodec_close(audio_codec_context);
	av_free(output_buffer);
	swr_free(&audio_convert_context);
	av_frame_free(&av_decode_frame);
	avformat_close_input(&format_context);

	system("pause");

	return 0;
}

