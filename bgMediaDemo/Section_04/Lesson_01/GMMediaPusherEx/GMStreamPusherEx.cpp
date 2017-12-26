#include "stdafx.h"
#include "GMStreamPusherEx.h"

GMStreamPusherEx::GMStreamPusherEx()
: output_format_context(NULL)
, output_format(NULL)
{

}

GMStreamPusherEx::~GMStreamPusherEx()
{

}

int GMStreamPusherEx::OpenTargetUrl(const char *target_url)
{
	int errCode = 0;

	AVFormatContext *output_format_context = NULL;
	errCode = avformat_alloc_output_context2(&output_format_context, NULL, "flv", target_url);
	if (errCode < 0)
	{
		char errmsg[4096] = {0};
		printf("Open push url failed...%s\n", av_make_error_string(errmsg, 4096, errCode));
		return errCode;
	}

	output_format = output_format_context->oformat;

	return errCode;
}

int GMStreamPusherEx::SetVideoParam(int video_codec, int width, int height, int frame_rate)
{
	int errCode = 0;

	// 这里要准备好输出的AVCodecContext
	output_video_codec_context = avcodec_alloc_context3(AV_CODEC_ID_H264);
	if (!output_video_codec_context)
		return -1;

	output_video_codec_context->width = width;
	output_video_codec_context->height = height;
	output_video_codec_context->time_base.num = 1;
	output_video_codec_context->time_base.den = frame_rate;

	return 0;
}

int GMStreamPusherEx::SetAudioParam(int audio_codec, int sample_rate, int bit, int channels)
{
	return 0;
}

int GMStreamPusherEx::InputMediaData(const unsigned char *data, int data_len)
{
	AVPacket packet;

	return 0;
}