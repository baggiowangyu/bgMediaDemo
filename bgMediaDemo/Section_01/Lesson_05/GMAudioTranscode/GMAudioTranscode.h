#ifndef _GM_AUDIO_TRANSCODE_H_
#define _GM_AUDIO_TRANSCODE_H_

//////////////////////////////////////////////////////////////////////////
//
// 文件说明：国迈音频转换接口
// 时间：2017-12-11
// 作者：wangyu
// 版本：v1.0.1
//
//////////////////////////////////////////////////////////////////////////

#include <string>

typedef void (__stdcall * _GMAudioTranscodeStateCallback)(int progress);
typedef void (__stdcall * _GMAudioTranscodeExceptionCallback)(int errcode, std::string errinfo);

class FFmpegAudioTranscode;

/**
 * 接口说明：开始一个音频转码工作
 * 参数说明：
 *	@source_audio		原始音频文件全路径
 *	@target_audio		转换后的音频文件全路径
 *	@state_callback		转换状态回调通知函数
 *	@except_callback	转换异常回调通知函数
 */
class GMMediaTranscoder
{
public:
	GMMediaTranscoder();
	~GMMediaTranscoder();

public:
	void SetupStateCallbackFunc(_GMAudioTranscodeStateCallback func);
	void SetupExceptionCallbackFunc(_GMAudioTranscodeExceptionCallback func);

public:
	int __stdcall Transcode(std::string source_audio, std::string target_audio);

private:
	FFmpegAudioTranscode *audio_transcoder_;

private:
	_GMAudioTranscodeStateCallback state_callback_;
	_GMAudioTranscodeExceptionCallback except_callback_;

};



#endif//_GM_AUDIO_TRANSCODE_H_
