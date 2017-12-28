#include "stdafx.h"
#include "GMStreamGenerator.h"

#include <Windows.h>

extern "C" 
{
#include "libavformat/avformat.h"
};

DWORD WINAPI StreamGeneratorThread(LPVOID lpParam)
{
	GMStreamGenerator *generator = (GMStreamGenerator *)lpParam;
	generator->is_generator_working_ = true;

	AVFormatContext *input_format_context = NULL;
	int errCode = avformat_open_input(&input_format_context, generator->GetSourceUrl().c_str(), NULL, NULL);
	if (errCode < 0)
		return errCode;

	avformat_find_stream_info(input_format_context, NULL);

	int input_video_stream_index = -1;
	int input_audio_stream_index = -1;

	AVStream *input_video_stream = NULL;
	AVStream *input_audio_stream = NULL;

	AVCodecContext *input_video_codec_context = NULL;
	AVCodecContext *input_audio_codec_context = NULL;

	AVCodec *input_video_codec = NULL;
	AVCodec *input_audio_codec = NULL;

	for (int index = 0; index < input_format_context->nb_streams; ++index)
	{
		if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			input_video_stream_index = index;
			input_video_stream = input_format_context->streams[index];
			input_video_codec_context = input_video_stream->codec;
			input_video_codec = avcodec_find_decoder(input_video_codec_context->codec_id);

			avcodec_open2(input_video_codec_context, input_video_codec, NULL);
		}
		else if (input_format_context->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			input_audio_stream_index = index;
			input_audio_stream = input_format_context->streams[index];
			input_audio_codec_context = input_audio_stream->codec;
			input_audio_codec = avcodec_find_decoder(input_audio_codec_context->codec_id);

			avcodec_open2(input_audio_codec_context, input_audio_codec, NULL);
		}
	}

	AVPacket packet;

	do 
	{
		if (!generator->is_generator_working_)
			break;

		errCode = av_read_frame(input_format_context, &packet);
		if (errCode < 0)
			break;

		int media_type = 0;
		if (packet.stream_index == input_video_stream_index)
			media_type = MEDIA_VIDEO;
		if (packet.stream_index == input_audio_stream_index)
			media_type = MEDIA_AUDIO;

		if (!(generator->GetMediaType() & media_type))
		{
			av_free_packet(&packet);
			continue;
		}

		if (generator->GetStreamType() & STREAM_ES)
		{
			// ES送
			generator->GetNotifer()->StreamDataNotify(packet.data, packet.size, media_type, STREAM_ES);
		}
		else if (generator->GetStreamType() & STREAM_PS)
		{
			// PS送
		}
		else if (generator->GetStreamType() & STREAM_YUV)
		{
			// YUV方象送
		}

		av_free_packet(&packet);

	} while (true);

	avcodec_close(input_video_codec_context);
	avcodec_close(input_audio_codec_context);
	avformat_close_input(&input_format_context);
}

GMStreamGenerator::GMStreamGenerator(GMStreamGenteratorNotifer *notier /* = NULL */)
: notifer_(notier)
, media_type_(0)
, stream_type_(0)
, is_generator_working_(false)
{
	av_register_all();
	avformat_network_init();
}

GMStreamGenerator::~GMStreamGenerator()
{

}

int GMStreamGenerator::OpenSource(const char *source_url, int media_type /* = STREAM_VIDEO */, int stream_type /* = STREAM_ES */)
{
	int errCode = 0;

	source_url_ = source_url;
	media_type_ = media_type;
	stream_type_ = stream_type;

	CreateThread(NULL, 0, StreamGeneratorThread, this, 0, NULL);

	return errCode;
}