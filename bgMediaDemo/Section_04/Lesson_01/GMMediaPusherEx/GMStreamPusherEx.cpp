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
		return errCode;

	output_format = output_format_context->oformat;

	return errCode;
}

int GMStreamPusherEx::SetVideoParam(int video_codec, int width, int height, int frame_rate)
{
	return 0;
}

int GMStreamPusherEx::SetAudioParam(int audio_codec, int sample_rate, int bit, int channels)
{
	return 0;
}

int GMStreamPusherEx::InputMediaData(const unsigned char *data, int data_len)
{
	return 0;
}