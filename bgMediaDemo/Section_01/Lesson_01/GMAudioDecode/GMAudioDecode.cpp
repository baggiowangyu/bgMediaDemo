// GMAudioDecode.cpp : 定义控制台应用程序的入口点。
//
// 这个程序，我们将音频文件或视频中的音频流解码后转换成PCM原始采样数据

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
#ifdef __cplusplus
};
#endif

#include <iostream>

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio


void ShowFormatInfo(AVFormatContext *input_format)
{
	printf("====================== Format Information =======================\n");
	printf("  Filename : %s\n", input_format->filename);
	printf("  Duration : %f\n", (double)input_format->duration / AV_TIME_BASE );
	printf("  Bit rate : %d\n", input_format->bit_rate);
	printf("  Format name : %s\n", input_format->iformat->name);
	printf("  Format long name : %s\n", input_format->iformat->long_name);
	printf("  Format long name : %s\n", input_format->iformat->long_name);
	printf("  Format raw codec id : %d\n", input_format->iformat->raw_codec_id);
}

void ShowCodecContextInfo(AVCodecContext *codec_context)
{
	printf("====================== Codec Context Information =======================");
	printf("  Codec id : %d\n", codec_context->codec_id);
	printf("  Codec name : %s\n", avcodec_get_name(codec_context->codec_id));
	printf("  Codec time_base : %d\n", av_q2d(codec_context->time_base));
	printf("  Codec color primaries : %s\n", av_color_primaries_name(codec_context->color_primaries));
	printf("  Codec color transfer characteristic : %s\n", av_color_transfer_name(codec_context->color_trc));
	printf("  Codec YUV colorspace : %s\n", av_color_space_name(codec_context->colorspace));
	printf("  Codec MPEG vs JPEG YUV range : %s\n", av_color_range_name(codec_context->color_range));
	printf("  Codec location of chroma samples : %s\n", av_chroma_location_name(codec_context->chroma_sample_location));

	printf("  Codec sample rate : %d\n", codec_context->sample_rate);
	printf("  Codec channels : %d\n", codec_context->channels);
	printf("  Codec sample format : %s\n", av_get_sample_fmt_name(codec_context->sample_fmt));

	printf("  Codec frame size : %d\n", codec_context->frame_size);
	printf("  Codec frame number : %d\n", codec_context->frame_number);
}

void ShowPacketInfo(AVPacket *av_packet)
{
	printf("====================== Packet Information =======================");
	printf("  Packet pos in stream : %d\n", av_packet->pos);
	printf("  Packet pts : %d\n", av_packet->pts);
	printf("  Packet dts : %d\n", av_packet->dts);
	printf("  Packet stream index : %d\n", av_packet->stream_index);
	printf("  Packet data len : %\n", av_packet->size);

}

void ShowFrameInfo(AVFrame *av_frame)
{
	printf("====================== Frame Information =======================");
	printf("  Frame pts : %d\n", av_frame->pts);
	printf("  Frame pkt_dts : %d\n", av_frame->pkt_dts);
	printf("  Frame width : %d\n", av_frame->width);
	printf("  Frame height : %d\n", av_frame->height);
	printf("  Frame number of audio samples : %d\n", av_frame->nb_samples);
	printf("  Frame is key frame : %d\n", av_frame->key_frame);
	//printf("  Frame pic type : %s\n", av_get_picture_type_char(av_frame->pict_type));
	printf("  Frame picture number in bitstream order : %d\n", av_frame->coded_picture_number);
	printf("  Frame picture number in display order : %d\n", av_frame->display_picture_number);
	printf("  Frame quality (1-best, bigger worse) : %d\n", av_frame->quality);
	printf("  Frame sample rate : %d\n", av_frame->sample_rate);
	printf("  Frame channel layout : %I64d\n", av_frame->channel_layout);
	printf("  Frame MPEG vs JPEG YUV range : %s\n", av_color_range_name(av_frame->color_range));
	printf("  Frame YUV colorspace : %s\n", av_color_space_name(av_frame->colorspace));
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		printf("GMAudioDecode.exe <audio_path> <audio_decode_path> \n");
		return 0;
	}

	// 完成初始化注册
	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	TCHAR audio_path[4096] = {0};
	_tcscpy_s(audio_path, 4096, argv[1]);

	TCHAR audio_decode_path[4096] = {0};
	_tcscpy_s(audio_decode_path, 4096, argv[2]);

	USES_CONVERSION;
	FILE *audio_output_file = fopen(T2A(audio_decode_path), "wb");
	if (audio_decode_path == NULL)
	{
		printf("open output file failed.\n");
		return -1;
	}

	// 打开音频文件
	AVFormatContext *format_context = NULL;
	int errCode = avformat_open_input(&format_context, T2A(audio_path), NULL, NULL);
	if (errCode != 0)
	{
		printf("avformat_open_input failed.\n");
		return errCode;
	}

	// 初步查找音频信息
	avformat_find_stream_info(format_context, NULL);

	// 显示格式信息
	ShowFormatInfo(format_context);

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

	// 准备解码之前，先设定好PCM转参数
	uint64_t output_channel_layout = AV_CH_LAYOUT_STEREO;
	int output_frame_size = audio_codec_context->frame_size;	// AAC是1024；MP3是1152；
	AVSampleFormat output_sample_format = AV_SAMPLE_FMT_S16;
	int output_sample_rate = audio_codec_context->sample_rate;	// 44100
	int output_channels = av_get_channel_layout_nb_channels(output_channel_layout);
	int output_buffer_size = av_samples_get_buffer_size(NULL, output_channels, output_frame_size, output_sample_format, 1);

	unsigned char *output_buffer = (unsigned char *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	// 
	int input_channel_layout = av_get_default_channel_layout(audio_codec_context->channels);

	// 
	SwrContext *audio_convert_context = swr_alloc();
	audio_convert_context = swr_alloc_set_opts(audio_convert_context, output_channel_layout, output_sample_format, output_sample_rate,
		input_channel_layout, audio_codec_context->sample_fmt, audio_codec_context->sample_rate, 0, NULL);
	swr_init(audio_convert_context);

	// 读取编码包，执行解码
	AVPacket av_packet;
	AVFrame *av_frame = av_frame_alloc();

	bool is_print_info = false;
	while (av_read_frame(format_context, &av_packet) == 0)
	{
		if (av_packet.stream_index != audio_stream_index)
		{
			// 不是音频数据流中的包，我们放过
			continue;
		}

		// 输出编码包相关信息
		ShowPacketInfo(&av_packet);

		// 解码音频编码包
		int got_frame_ptr = 0;
		errCode = avcodec_decode_audio4(audio_codec_context, av_frame, &got_frame_ptr, &av_packet);

		if (!got_frame_ptr)
			continue;

		// 解码完成，这里可以输出音频数据了
		if (!is_print_info)
		{
			is_print_info = true;

			// 显示解码上下文
			ShowCodecContextInfo(audio_codec_context);
		}

		ShowFrameInfo(av_frame);

		// 执行转换
		swr_convert(audio_convert_context, &output_buffer, MAX_AUDIO_FRAME_SIZE, (const unsigned char **)av_frame->data, av_frame->nb_samples);

		fwrite(output_buffer, 1, output_buffer_size, audio_output_file);
	}

	avcodec_close(audio_codec_context);
	av_free(output_buffer);
	swr_free(&audio_convert_context);
	av_frame_free(&av_frame);
	fclose(audio_output_file);
	avformat_close_input(&format_context);

	system("pause");
	return 0;
}

