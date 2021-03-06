#include "stdafx.h"
#include "GMStreamPusherEx.h"

extern "C" 
{
#include "libavformat/avformat.h"
};

GMStreamPusherEx::GMStreamPusherEx()
: output_format_context(NULL)
, output_format(NULL)
{
	av_register_all();
	avformat_network_init();
}

GMStreamPusherEx::~GMStreamPusherEx()
{

}

int GMStreamPusherEx::OpenTargetUrl(const char *target_url)
{
	int errCode = 0;

	// 这里的输出格式，最好是ts，这样可以支持rtmp、rtp、hls
	//AVFormatContext *output_format_context = NULL;
	errCode = avformat_alloc_output_context2(&output_format_context, NULL, NULL, target_url);
	if (errCode < 0)
	{
		char errmsg[4096] = {0};
		printf("Open push url failed...%s\n", av_make_error_string(errmsg, 4096, errCode));
		return errCode;
	}

	output_format = output_format_context->oformat;
	target_url_ = target_url;

	return errCode;
}

int GMStreamPusherEx::SetVideoParam(int width, int height, int frame_rate, int video_codec /* = MEDIA_CODEC_H264 */, int color_fmt /* = VIDEO_COLOR_FMT_YUV420 */)
{
	int errCode = 0;
	enum AVCodecID av_video_codec_id = AV_CODEC_ID_NONE;
	enum AVPixelFormat av_pixel_fmt = AV_PIX_FMT_NONE;

	switch (video_codec)
	{
	case MEDIA_CODEC_H264:
		av_video_codec_id = AV_CODEC_ID_H264;
		break;
	}

	switch (color_fmt)
	{
	case VIDEO_COLOR_FMT_YUV420:
		av_pixel_fmt = AV_PIX_FMT_YUV420P;
		break;
	}

	output_video_codec = avcodec_find_encoder(av_video_codec_id);
	if (!output_video_codec)
		return -2;

	//// 这里要准备好输出的AVCodecContext
	//output_video_codec_context = avcodec_alloc_context3(output_video_codec);
	//if (!output_video_codec_context)
	//	return -1;

	// 创建视频流
	output_video_stream = avformat_new_stream(output_format_context, output_video_codec);
	output_video_codec_context = output_video_stream->codec;

	output_video_codec_context->width			= width;
	output_video_codec_context->height			= height;
	output_video_codec_context->time_base.num	= 1;
	output_video_codec_context->time_base.den	= frame_rate;
	output_video_codec_context->pix_fmt			= av_pixel_fmt;
	output_video_codec_context->bit_rate		= 60000;
	output_video_codec_context->codec_type		= AVMEDIA_TYPE_VIDEO;
	output_video_codec_context->gop_size		= 10;
	output_video_codec_context->max_b_frames	= 1;

	output_video_stream->codec->codec_tag = 0;

	if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
		output_video_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return 0;
}

int GMStreamPusherEx::SetAudioParam(int bit_rate, int sample_rate, int sample_fmt /* = AUDIO_BIT_S16 */, int channel_layout /* = AUDIO_CHENNAL_LAYOUT_STEREO */, int audio_codec /* = MEDIA_CODEC_AAC */)
{
	int errCode = 0;

	enum AVCodecID av_audio_codec_id = AV_CODEC_ID_NONE;
	uint64_t audio_channel_layout = AV_CH_LAYOUT_STEREO;
	AVSampleFormat av_audio_sample_fmt = AV_SAMPLE_FMT_NONE;

	switch (audio_codec)
	{
	case MEDIA_CODEC_AAC:
		av_audio_codec_id = AV_CODEC_ID_AAC;
		break;
	case MEDIA_CODEC_MP3:
		av_audio_codec_id = AV_CODEC_ID_MP3;
		break;
	case MEDIA_CODEC_G711u:
		av_audio_codec_id = AV_CODEC_ID_PCM_MULAW;
		break;
	}

	switch (channel_layout)
	{
	case AUDIO_CHENNAL_LAYOUT_MONO:
		audio_channel_layout = AV_CH_LAYOUT_MONO;
		break;
	case AUDIO_CHENNAL_LAYOUT_STEREO:
		audio_channel_layout = AV_CH_LAYOUT_STEREO;
		break;
	}

	switch (sample_fmt)
	{
	case AUDIO_BIT_U8:
		av_audio_sample_fmt = AV_SAMPLE_FMT_U8;
		break;
	case AUDIO_BIT_S16:
		av_audio_sample_fmt = AV_SAMPLE_FMT_S16;
		break;
	case AUDIO_BIT_S32:
		av_audio_sample_fmt = AV_SAMPLE_FMT_S32;
		break;
	case AUDIO_BIT_FLT:
		av_audio_sample_fmt = AV_SAMPLE_FMT_FLT;
		break;
	case AUDIO_BIT_DBL:
		av_audio_sample_fmt = AV_SAMPLE_FMT_DBL;
		break;
	case AUDIO_BIT_U8P:
		av_audio_sample_fmt = AV_SAMPLE_FMT_U8P;
		break;
	case AUDIO_BIT_S16P:
		av_audio_sample_fmt = AV_SAMPLE_FMT_S16P;
		break;
	case AUDIO_BIT_S32P:
		av_audio_sample_fmt = AV_SAMPLE_FMT_S32P;
		break;
	case AUDIO_BIT_FLTP:
		av_audio_sample_fmt = AV_SAMPLE_FMT_FLTP;
		break;
	case AUDIO_BIT_DBLP:
		av_audio_sample_fmt = AV_SAMPLE_FMT_DBLP;
		break;
	}

	output_audio_codec = avcodec_find_encoder(av_audio_codec_id);
	if (!output_audio_codec)
		return -2;

	// 这里要准备好输出的AVCodecContext
	//output_audio_codec_context = avcodec_alloc_context3(output_audio_codec);
	//if (!output_audio_codec_context)
	//	return -1;

	output_audio_stream = avformat_new_stream(output_format_context, output_audio_codec);
	output_audio_codec_context = output_audio_stream->codec;


	output_audio_codec_context->sample_rate		= sample_rate;
	output_audio_codec_context->bit_rate		= bit_rate;
	output_audio_codec_context->channel_layout	= audio_channel_layout;
	output_audio_codec_context->channels		= av_get_channel_layout_nb_channels(output_audio_codec_context->channel_layout);
	output_audio_codec_context->sample_fmt		= av_audio_sample_fmt;
	output_audio_codec_context->bit_rate		= 64000;
	output_audio_codec_context->codec_type		= AVMEDIA_TYPE_AUDIO;

	output_audio_stream->codec->codec_tag = 0;

	if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
		output_audio_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

	return 0;
}

int GMStreamPusherEx::StartPush()
{
	int errCode = 0;

	if (!(output_format_context->flags & AVFMT_NOFILE))
	{
		errCode = avio_open2(&output_format_context->pb, target_url_.c_str(), AVIO_FLAG_WRITE, NULL, NULL);
		if (errCode < 0)
		{
			char errmsg[4096] = {0};
			printf("Open push url failed...%s\n", av_make_error_string(errmsg, 4096, errCode));
			return errCode;
		}
	}

	errCode = avformat_write_header(output_format_context, NULL);
	if (errCode < 0)
		return errCode;


	return errCode;
}

int GMStreamPusherEx::InputMediaData(const unsigned char *data, int data_len, int data_type /* = MEDIA_TYPE_VIDEO */)
{
	int errCode = 0;
	AVPacket packet;

	av_init_packet(&packet);
	uint8_t *buf = (uint8_t*)av_malloc(data_len);
	memcpy(buf, data, data_len);
	av_packet_from_data(&packet, buf, data_len);

	if (data_type == MEDIA_TYPE_VIDEO)
		packet.stream_index = output_video_stream->index;
	else if (data_type == MEDIA_TYPE_AUDIO)
		packet.stream_index = output_audio_stream->index;

	// 写入编码包
	errCode = av_interleaved_write_frame(output_format_context, &packet);
	if (errCode < 0)
	{
		char errmsg[4096] = {0};
		printf("Open push url failed...%s\n", av_make_error_string(errmsg, 4096, errCode));
	}

	return errCode;
}

void GMStreamPusherEx::StopPush()
{
	av_write_trailer(output_format_context);
}