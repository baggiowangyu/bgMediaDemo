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
#include "libavutil/rational.h"
#include "libavutil/pixdesc.h"
#include "libavutil/samplefmt.h"
#ifdef __cplusplus
};
#endif

#include <iostream>


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
	printf("  Frame pts : "<<av_frame->pts);
	printf("  Frame pkt_dts : "<<av_frame->pkt_dts);
	printf("  Frame width : "<<av_frame->width);
	printf("  Frame height : "<<av_frame->height);
	printf("  Frame number of audio samples : "<<av_frame->nb_samples);
	printf("  Frame is key frame : "<<av_frame->key_frame);
	printf("  Frame pic type : "<<av_get_picture_type_char(av_frame->pict_type));
	printf("  Frame picture number in bitstream order : "<<av_frame->coded_picture_number);
	printf("  Frame picture number in display order : "<<av_frame->display_picture_number);
	printf("  Frame quality (1-best, bigger worse) : "<<av_frame->quality);
	printf("  Frame sample rate : "<<av_frame->sample_rate);
	printf("  Frame channel layout : "<<av_frame->channel_layout);
	printf("  Frame MPEG vs JPEG YUV range : "<<av_color_range_name(av_frame->color_range));
	printf("  Frame YUV colorspace : "<<av_color_space_name(av_frame->colorspace));
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
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
	FILE *audio_output_file = fopen(T2A(audio_decode_path), "w");
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

		fwrite(av_frame->data, av_frame->linesize[0], 1, audio_output_file);
	}

	av_frame_free(&av_frame);
	fclose(audio_output_file);

	system("pause");
	return 0;
}

