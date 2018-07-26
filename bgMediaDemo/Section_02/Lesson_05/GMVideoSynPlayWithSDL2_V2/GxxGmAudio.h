#ifndef _GxxGmAudio_H_
#define _GxxGmAudio_H_

#include "GxxGmQueue.h"

struct AVStream;
struct AVCodecContext;

/**
 * 处理音频相关业务
 */
class GxxGmAudio
{
public:
	GxxGmAudio();
	GxxGmAudio(AVCodecContext *audio_ctx, int audio_stream_index);
	~GxxGmAudio();

public:
	void SetStream(AVStream *stream);
	bool AudioPlay();

public:
	//int audio_decode_frame(AudioState *audio_state, uint8_t *audio_buf, int buf_size);
	//static void audio_callback(void* userdata, Uint8 *stream, int len);

private:
	const unsigned int buffer_size_;

	//GxxGmPacketQueue audioq_;			// 音频编码包队列

	int audio_stream_index_;
	AVStream *audio_stream_;			// 音频流
	AVCodecContext *audio_ctx_;

	double audio_clock_;				// 音频流时钟

	unsigned char *audio_buff_;			// 解码后数据的缓冲空间
	unsigned int audio_buff_size_;		// buffer中的字节数
	unsigned int audio_buff_index_;		// buffer中未发送数据的index
};

#endif//_GxxGmAudio_H_
