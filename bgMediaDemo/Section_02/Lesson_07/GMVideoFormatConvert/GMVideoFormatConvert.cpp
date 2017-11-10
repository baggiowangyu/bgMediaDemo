// GMVideoFormatConvert.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libpostproc/postprocess.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
};
#endif

#include <iostream>
#include <atlconv.h>


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		std::cout<<"usage: GMVideoFormatConvert.exe <input_media> <output_media>"<<std::endl;
		return 0;
	}

	TCHAR input_media[4096] = {0};
	TCHAR output_media[4096] = {0};

	_tcscpy_s(input_media, 4096, argv[1]);
	_tcscpy_s(output_media, 4096, argv[2]);

	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	USES_CONVERSION;
	AVFormatContext *input_format_context = NULL;
	int errCode = avformat_open_input(&input_format_context, T2A(input_media), NULL, NULL);
	if (errCode != 0)
	{
		return errCode;
	}

	avformat_find_stream_info(input_format_context, NULL);

	int video_stream_index = -1;
	int audio_stream_index = -1;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = index;
		}
		else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index = index;
		}
	}

	AVFormatContext *output_format_context = NULL;
	avformat_alloc_output_context2(&output_format_context, NULL, NULL, T2A(output_media));

	if (!output_format_context)
	{
		return -1;
	}

	AVStream *input_video_stream = input_format_context->streams[video_stream_index];
	AVStream *input_audio_stream = input_format_context->streams[audio_stream_index];

	AVCodec *audio_codec = avcodec_find_encoder(AV_CODEC_ID_PCM_MULAW);
	if (audio_codec == NULL)
	{
		return -3;
	}

	// 根据已有的流序号，谁小先创建谁
	AVStream *output_video_stream = NULL;
	AVStream *output_audio_stream = NULL;
	if (video_stream_index < audio_stream_index)
	{
		output_video_stream = avformat_new_stream(output_format_context, input_video_stream->codec->codec);
		output_audio_stream = avformat_new_stream(output_format_context, audio_codec);

		output_video_stream->index = 0;
		output_audio_stream->index = 1;
	}
	else
	{
		output_audio_stream = avformat_new_stream(output_format_context, audio_codec);
		output_video_stream = avformat_new_stream(output_format_context, input_video_stream->codec->codec);

		output_audio_stream->index = 0;
		output_video_stream->index = 1;
	}

	// 需要编码的就在第二个参数写入编码器，不需要就置空
	// 完成流创建后，需要手动完成具体成员的初始化
	
	// 这里仅仅只是复制了解码器的一些上下文，流里面的其他参数应该还需要配置
	errCode = avcodec_copy_context(output_video_stream->codec, input_video_stream->codec);
	if (errCode < 0)
	{
		return -2;
	}

	output_video_stream->time_base						= input_video_stream->time_base;
	output_video_stream->start_time						= input_video_stream->start_time;
	output_video_stream->duration						= input_video_stream->duration;
	output_video_stream->pts_wrap_bits					= input_video_stream->pts_wrap_bits;
	output_video_stream->first_dts						= input_video_stream->first_dts;
	output_video_stream->cur_dts						= input_video_stream->cur_dts;
	output_video_stream->probe_packets					= input_video_stream->probe_packets;
	output_video_stream->codec_info_nb_frames			= input_video_stream->codec_info_nb_frames;
	output_video_stream->need_parsing					= input_video_stream->need_parsing;
	output_video_stream->index_entries					= input_video_stream->index_entries;
	output_video_stream->nb_index_entries				= input_video_stream->nb_index_entries;
	output_video_stream->index_entries_allocated_size	= input_video_stream->index_entries_allocated_size;
	output_video_stream->r_frame_rate					= input_video_stream->r_frame_rate;
	output_video_stream->pts_wrap_reference				= input_video_stream->pts_wrap_reference;
	output_video_stream->pts_wrap_behavior				= input_video_stream->pts_wrap_behavior;

	// 这里仅仅只是复制了解码器的一些上下文，流里面的其他参数应该还需要配置
	output_audio_stream->codec = avcodec_alloc_context3(audio_codec);

	output_audio_stream->time_base						= input_video_stream->time_base;
	output_audio_stream->start_time						= input_video_stream->start_time;
	output_audio_stream->duration						= input_video_stream->duration;
	output_audio_stream->pts_wrap_bits					= input_video_stream->pts_wrap_bits;
	output_audio_stream->first_dts						= input_video_stream->first_dts;
	output_audio_stream->cur_dts						= input_video_stream->cur_dts;
	output_audio_stream->probe_packets					= input_video_stream->probe_packets;
	output_audio_stream->codec_info_nb_frames			= input_video_stream->codec_info_nb_frames;
	output_audio_stream->need_parsing					= input_video_stream->need_parsing;
	output_audio_stream->index_entries					= input_video_stream->index_entries;
	output_audio_stream->nb_index_entries				= input_video_stream->nb_index_entries;
	output_audio_stream->index_entries_allocated_size	= input_video_stream->index_entries_allocated_size;
	output_audio_stream->r_frame_rate					= input_video_stream->r_frame_rate;
	output_audio_stream->pts_wrap_reference				= input_video_stream->pts_wrap_reference;
	output_audio_stream->pts_wrap_behavior				= input_video_stream->pts_wrap_behavior;

	output_audio_stream->codec->bit_rate_tolerance		= input_audio_stream->codec->bit_rate_tolerance;
	output_audio_stream->codec->ticks_per_frame			= input_audio_stream->codec->ticks_per_frame;
	output_audio_stream->codec->gop_size				= input_audio_stream->codec->gop_size;
	output_audio_stream->codec->b_quant_factor			= input_audio_stream->codec->b_quant_factor;
	output_audio_stream->codec->b_quant_offset			= input_audio_stream->codec->b_quant_offset;
	output_audio_stream->codec->ildct_cmp				= input_audio_stream->codec->ildct_cmp;
	output_audio_stream->codec->me_subpel_quality		= input_audio_stream->codec->me_subpel_quality;
	output_audio_stream->codec->intra_quant_bias		= input_audio_stream->codec->intra_quant_bias;
	output_audio_stream->codec->inter_quant_bias		= input_audio_stream->codec->inter_quant_bias;
	output_audio_stream->codec->mb_lmin					= input_audio_stream->codec->mb_lmin;
	output_audio_stream->codec->me_penalty_compensation	= input_audio_stream->codec->me_penalty_compensation;
	output_audio_stream->codec->bidir_refine			= input_audio_stream->codec->bidir_refine;
	output_audio_stream->codec->keyint_min				= input_audio_stream->codec->keyint_min;
	output_audio_stream->codec->scenechange_factor		= input_audio_stream->codec->scenechange_factor;
	output_audio_stream->codec->mv0_threshold			= input_audio_stream->codec->mv0_threshold;
	output_audio_stream->codec->b_sensitivity			= input_audio_stream->codec->b_sensitivity;
	output_audio_stream->codec->sample_rate				= 8000;//input_audio_stream->codec->sample_rate;
	output_audio_stream->codec->channels				= 2;//input_audio_stream->codec->channels;
	output_audio_stream->codec->sample_fmt				= input_audio_stream->codec->sample_fmt;
	output_audio_stream->codec->audio_service_type		= input_audio_stream->codec->audio_service_type;
	output_audio_stream->codec->qcompress				= input_audio_stream->codec->qcompress;
	output_audio_stream->codec->qblur					= input_audio_stream->codec->qblur;
	output_audio_stream->codec->qmin					= input_audio_stream->codec->qmin;
	output_audio_stream->codec->qmax					= input_audio_stream->codec->qmax;
	output_audio_stream->codec->max_qdiff				= input_audio_stream->codec->max_qdiff;
	output_audio_stream->codec->rc_buffer_aggressivity	= input_audio_stream->codec->rc_buffer_aggressivity;
	output_audio_stream->codec->rc_min_vbv_overflow_use	= input_audio_stream->codec->rc_min_vbv_overflow_use;
	output_audio_stream->codec->frame_skip_cmp			= input_audio_stream->codec->frame_skip_cmp;
	output_audio_stream->codec->workaround_bugs			= input_audio_stream->codec->workaround_bugs;
	output_audio_stream->codec->error_concealment		= input_audio_stream->codec->error_concealment;

	output_video_stream->codec->codec_tag = 0;
	output_audio_stream->codec->codec_tag = 0;

	if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
	{
		output_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		output_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	av_dump_format(output_format_context, 0, T2A(output_media), 1);

	if (!(output_format_context->flags & AVFMT_NOFILE))
	{
		errCode = avio_open(&output_format_context->pb, T2A(output_media), AVIO_FLAG_WRITE);
		if (errCode < 0)
		{
			return -4;
		}
	}

	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
	{
		return -5;
	}

	while (true)
	{
		AVPacket packet;

		errCode = av_read_frame(input_format_context, &packet);
		if (errCode < 0)
		{
			break;
		}

		AVStream *input_stream = input_format_context->streams[packet.stream_index];
		AVStream *output_stream = output_format_context->streams[packet.stream_index];

		// 转换PTS/DTS
		packet.pts = av_rescale_q_rnd(packet.pts, input_stream->time_base, output_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		packet.dts = av_rescale_q_rnd(packet.dts, input_stream->time_base, output_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		packet.duration = av_rescale_q(packet.duration, input_stream->time_base, output_stream->time_base);
		packet.pos = -1;

		// 写入编码帧
		errCode = av_interleaved_write_frame(output_format_context, &packet);
		if (errCode < 0)
		{
			break;
		}
	}

	av_write_trailer(output_format_context);

	avformat_close_input(&input_format_context);

	if (output_format_context && !(output_format_context->flags & AVFMT_NOFILE))
	{
		avio_close(output_format_context->pb);
	}

	return 0;
}

