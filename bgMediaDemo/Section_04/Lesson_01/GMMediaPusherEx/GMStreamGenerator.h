#ifndef _GM_STREAM_GENERATOR_H_
#define _GM_STREAM_GENERATOR_H_

#include <string>

#define __STDC_CONSTANT_MACROS

#define MEDIA_VIDEO	0x001							// 视频流
#define MEDIA_AUDIO	0x010							// 音频流
//#define STREAM_TEXT		0x100						// 文字流
#define MEDIA_ALL		(MEDIA_VIDEO|MEDIA_AUDIO)		// 视音频流

#define STREAM_ES		0x0001
#define STREAM_PS		0x0010
#define STREAM_YUV		0x0100
#define STREAM_PCM		0x1000

class GMStreamGenteratorNotifer
{
public:
	virtual int StreamDataNotify(unsigned char *data, int data_len, int stream_type, int media_type) = 0;
};

class GMStreamGenerator
{
public:
	GMStreamGenerator(GMStreamGenteratorNotifer *notier = NULL);
	~GMStreamGenerator();

public:
	int OpenSource(const char *source_url, int media_type = MEDIA_VIDEO, int stream_type = STREAM_ES);
	void CloseSource();

public:
	std::string GetSourceUrl() { return source_url_; }
	int GetMediaType() { return media_type_; }
	int GetStreamType() { return stream_type_; }
	GMStreamGenteratorNotifer* GetNotifer() { return notifer_; }

public:
	bool is_generator_working_;

private:
	std::string source_url_;
	int media_type_;
	int stream_type_;
	GMStreamGenteratorNotifer *notifer_;
};

#endif//_GM_STREAM_GENERATOR_H_