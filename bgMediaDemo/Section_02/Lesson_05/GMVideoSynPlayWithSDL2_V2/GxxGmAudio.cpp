#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
};
#endif

#include "GxxGmAudio.h"

GxxGmAudio::GxxGmAudio()
: buffer_size_(192000)
, audio_stream_index_(-1)
, audio_stream_(NULL)
, audio_ctx_(NULL)
, audio_clock_(0)
, audio_buff_(new unsigned char[buffer_size_])
, audio_buff_size_(0)
, audio_buff_index_(0)
{

}

GxxGmAudio::GxxGmAudio(AVCodecContext *audio_ctx, int audio_stream_index)
: buffer_size_(192000)
, audio_stream_index_(audio_stream_index)
, audio_stream_(NULL)
, audio_ctx_(audio_ctx)
, audio_clock_(0)
, audio_buff_(new unsigned char[buffer_size_])
, audio_buff_size_(0)
, audio_buff_index_(0)
{

}

GxxGmAudio::~GxxGmAudio()
{
	if (audio_buff_)
		delete [] audio_buff_;
	audio_buff_ = NULL;
}

void GxxGmAudio::SetStream(AVStream *stream)
{
	audio_stream_ = stream;
}

bool GxxGmAudio::AudioPlay()
{
	//SDL_AudioSpec desired;
	//desired.freq = audio_ctx_->sample_rate;
	//desired.channels = audio_ctx_->channels;
	//desired.format = AUDIO_S16SYS;
	//desired.samples = 1024;
	//desired.silence = 0;
	//desired.userdata = this;
	//desired.callback = audio_callback;

	//if (SDL_OpenAudio(&desired, NULL) < 0)
	//{
	//	return false;
	//}

	//SDL_PauseAudio(0); // playing

	return true;
}