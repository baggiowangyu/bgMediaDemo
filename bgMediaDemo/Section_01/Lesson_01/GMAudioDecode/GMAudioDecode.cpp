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
	std::cout<<"====================== Format Information ======================="<<std::endl;
	std::cout<<"  Filename : "<<input_format->filename<<std::endl;
	std::cout<<"  Duration : "<<(double)input_format->duration / AV_TIME_BASE <<std::endl;
	std::cout<<"  Bit rate : "<<input_format->bit_rate<<std::endl;
	std::cout<<"  Format name : "<<input_format->iformat->name<<std::endl;
	std::cout<<"  Format long name : "<<input_format->iformat->long_name<<std::endl;
	std::cout<<"  Format long name : "<<input_format->iformat->long_name<<std::endl;
	std::cout<<"  Format raw codec id : "<<input_format->iformat->raw_codec_id<<std::endl;
}

void ShowCodecContextInfo(AVCodecContext *codec_context)
{
	std::cout<<"====================== Codec Context Information ======================="<<std::endl;
	std::cout<<"  Codec id : "<<codec_context->codec_id<<std::endl;
	std::cout<<"  Codec name : "<<avcodec_get_name(codec_context->codec_id)<<std::endl;
	std::cout<<"  Codec time_base : "<<av_q2d(codec_context->time_base)<<std::endl;
	std::cout<<"  Codec color primaries : "<<av_color_primaries_name(codec_context->color_primaries)<<std::endl;
	std::cout<<"  Codec color transfer characteristic : "<<av_color_transfer_name(codec_context->color_trc)<<std::endl;
	std::cout<<"  Codec YUV colorspace : "<<av_color_space_name(codec_context->colorspace)<<std::endl;
	std::cout<<"  Codec MPEG vs JPEG YUV range : "<<av_color_range_name(codec_context->color_range)<<std::endl;
	std::cout<<"  Codec location of chroma samples : "<<av_chroma_location_name(codec_context->chroma_sample_location)<<std::endl;

	std::cout<<"  Codec sample rate : "<<codec_context->sample_rate<<std::endl;
	std::cout<<"  Codec channels : "<<codec_context->channels<<std::endl;
	std::cout<<"  Codec sample format : "<<av_get_sample_fmt_name(codec_context->sample_fmt)<<std::endl;

	std::cout<<"  Codec frame size : "<<codec_context->frame_size<<std::endl;
	std::cout<<"  Codec frame number : "<<codec_context->frame_number<<std::endl;
}

void ShowPacketInfo(AVPacket *av_packet)
{
	std::cout<<"====================== Packet Information ======================="<<std::endl;
	std::cout<<"  Packet pos in stream : "<<av_packet->pos<<std::endl;
	std::cout<<"  Packet pts : "<<av_packet->pts<<std::endl;
	std::cout<<"  Packet dts : "<<av_packet->dts<<std::endl;
	std::cout<<"  Packet stream index : "<<av_packet->stream_index<<std::endl;
	std::cout<<"  Packet data len : "<<av_packet->size<<std::endl;

}

void ShowFrameInfo(AVFrame *av_frame)
{
	std::cout<<"====================== Frame Information ======================="<<std::endl;
	std::cout<<"  Frame pts : "<<av_frame->pts<<std::endl;
	std::cout<<"  Frame pkt_dts : "<<av_frame->pkt_dts<<std::endl;
	std::cout<<"  Frame width : "<<av_frame->width<<std::endl;
	std::cout<<"  Frame height : "<<av_frame->height<<std::endl;
	std::cout<<"  Frame number of audio samples : "<<av_frame->nb_samples<<std::endl;
	std::cout<<"  Frame is key frame : "<<av_frame->key_frame<<std::endl;
	std::cout<<"  Frame pic type : "<<av_get_picture_type_char(av_frame->pict_type)<<std::endl;
	std::cout<<"  Frame picture number in bitstream order : "<<av_frame->coded_picture_number<<std::endl;
	std::cout<<"  Frame picture number in display order : "<<av_frame->display_picture_number<<std::endl;
	std::cout<<"  Frame quality (1-best, bigger worse) : "<<av_frame->quality<<std::endl;
	std::cout<<"  Frame sample rate : "<<av_frame->sample_rate<<std::endl;
	std::cout<<"  Frame channel layout : "<<av_frame->channel_layout<<std::endl;
	std::cout<<"  Frame MPEG vs JPEG YUV range : "<<av_color_range_name(av_frame->color_range)<<std::endl;
	std::cout<<"  Frame YUV colorspace : "<<av_color_space_name(av_frame->colorspace)<<std::endl;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		printf("GMAudioDecode.exe <audio_path> \n");
		return 0;
	}

	// 完成初始化注册
	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	TCHAR audio_path[4096] = {0};
	_tcscpy_s(audio_path, 4096, argv[1]);

	// 打开音频文件
	USES_CONVERSION;
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
	}

	av_frame_free(&av_frame);

	system("pause");
	return 0;
}

