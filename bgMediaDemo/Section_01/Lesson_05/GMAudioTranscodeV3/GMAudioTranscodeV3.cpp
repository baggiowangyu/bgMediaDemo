// GMAudioTranscodeV3.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#ifdef __cplusplus
	};
#endif

class GMAudioTranscode
{
public:
	enum GMAudioCodec
	{
		CODEC_DEFAULT,
		CODEC_AAC,
		CODEC_MP3,
		CODEC_G711u,
		CODEC_G711a
	};

public:
	GMAudioTranscode();
	~GMAudioTranscode();

public:
	int OpenAudioFile(const char *audio_url);

public:
	int Transcode(GMAudioCodec encoder = CODEC_DEFAULT);
};

/**
 * 检查指定的的采样格式是否被编码器所支持
 */
bool CheckIsSupportSampleFmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
	const enum AVSampleFormat *p = codec->sample_fmts;
	while (*p != AV_SAMPLE_FMT_NONE)
	{
		if (*p == sample_fmt)
			return true;

		p++;
	}

	return false;
}

/**
 * 找到编码器支持的最高采样率
 */
int SelectSampleRate(const AVCodec *encoder)
{
	const int *p;
	int best_samplerate = 0;

	if (!encoder->supported_samplerates)
		return 44100;

	p = encoder->supported_samplerates;
	while (*p)
	{
		if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
			best_samplerate = *p;

		p++;
	}

	return best_samplerate;
}

/**
 * 
 */
int SelectChannelLayout(const AVCodec *encoder)
{
	const uint64_t *p;
	uint64_t best_ch_layout = 0;
	int best_nb_channels   = 0;

	if (!encoder->channel_layouts)
		return AV_CH_LAYOUT_STEREO;

	p = encoder->channel_layouts;
	while (*p)
	{
		int nb_channels = av_get_channel_layout_nb_channels(*p);

		if (nb_channels > best_nb_channels)
		{
			best_ch_layout    = *p;
			best_nb_channels = nb_channels;
		}
		p++;
	}

	return best_ch_layout;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		std::cout<<""<<std::endl;
		return 0;
	}

	char source_file[4096] = {0};
	char target_file[4096] = {0};
	strcpy_s(source_file, 4096, argv[1]);
	strcpy_s(target_file, 4096, argv[2]);

	av_register_all();
	avformat_network_init();
	avcodec_register_all();

	//////////////////////////////////////////////////////////////////////////
	//
	// 处理输入参数
	AVFormatContext *source_fmtx = NULL;
	int errCode = avformat_open_input(&source_fmtx, source_file, NULL, NULL);
	if (errCode < 0)
	{
		return 0;
	}

	errCode = avformat_find_stream_info(source_fmtx, NULL);
	if (errCode < 0)
	{
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 处理输出参数
	AVFormatContext *target_fmtx = NULL;
	errCode = avformat_alloc_output_context2(&target_fmtx, NULL, NULL, target_file);
	if (errCode < 0)
	{
		return errCode;
	}

	AVCodec *encoder = avcodec_find_encoder(target_fmtx->oformat->audio_codec);
	if (encoder == NULL)
	{
		return -1;
	}

	AVCodecContext *encode_ctx = avcodec_alloc_context3(encoder);
	if (encode_ctx == NULL)
	{
		return -2;
	}

	encode_ctx->bit_rate = 64000;
	encode_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
	if (!CheckIsSupportSampleFmt(encoder, encode_ctx->sample_fmt))
	{
		return -3;
	}
	encode_ctx->sample_rate = SelectSampleRate(encoder);
	encode_ctx->channel_layout = SelectChannelLayout(encoder);
	encode_ctx->channels = av_get_channel_layout_nb_channels(encode_ctx->channel_layout);

	// 打开编码上下文
	errCode = avcodec_open2(encode_ctx, encoder, NULL);
	if (errCode < 0)
	{
		return errCode;
	}

	//////////////////////////////////////////////////////////////////////////
	//
	// 生成输出文件
	errCode = avio_open(target_fmtx->pb, target_file, AVIO_FLAG_WRITE);
	if (errCode < 0)
	{
		return errCode;
	}
	

	//////////////////////////////////////////////////////////////////////////
	//
	// 转码写入
	AVPacket *pkt = av_packet_alloc();
	if (pkt == NULL)
	{
		return -4;
	}

	AVFrame *frame = av_frame_alloc();
	if (frame == NULL)
	{
		return -5;
	}

	frame->nb_samples		= encode_ctx->frame_size;
	frame->format			= encode_ctx->sample_fmt;
	frame->channel_layout	= encode_ctx->channel_layout;
	
	errCode = av_frame_get_buffer(frame, 0);
	if (errCode < 0)
	{
		return -5;
	}

	

	//////////////////////////////////////////////////////////////////////////
	//
	// 清理关闭

	return 0;
}

