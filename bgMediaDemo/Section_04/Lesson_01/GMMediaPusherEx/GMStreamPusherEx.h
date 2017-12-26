#ifndef _GM_STREAM_PUSHER_EX_H_
#define _GM_STREAM_PUSHER_EX_H_

#define MEDIA_CODEC_H264
#define MEDIA_CODEC_AAC

class GMStreamPusherEx
{
public:
	GMStreamPusherEx();
	~GMStreamPusherEx();

public:
	int OpenTargetUrl(const char *target_url);

public:
	int SetVideoParam(int video_codec, int width, int height, int frame_rate);
	int SetAudioParam(int audio_codec, int sample_rate, int bit, int channels);
	int InputMediaData(const unsigned char *data, int data_len);

private:
	AVFormatContext *output_format_context;
	AVOutputFormat *output_format;
	AVCodecContext *output_video_codec_context;
};

#endif//_GM_STREAM_PUSHER_EX_H_