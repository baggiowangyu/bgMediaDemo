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

class GMMediaTranscoder
{
public:
	GMMediaTranscoder();
	~GMMediaTranscoder();

public:
	/**
	 * 安装回调接口
	 * 目前只实现了异常回调，进度回调暂不实现
	 */
	void SetupStateCallbackFunc(_GMAudioTranscodeStateCallback func);
	void SetupExceptionCallbackFunc(_GMAudioTranscodeExceptionCallback func);

public:
	/**
	 * 接口说明：开始一个音频转码工作
	 * 参数说明：
	 *	@source_audio		原始音频文件全路径
	 *	@target_audio		转换后的音频文件全路径
	 */
	int __stdcall Transcode(std::string source_audio, std::string target_audio);

private:
	FFmpegAudioTranscode *audio_transcoder_;

private:
	_GMAudioTranscodeStateCallback state_callback_;
	_GMAudioTranscodeExceptionCallback except_callback_;

};



#endif//_GM_AUDIO_TRANSCODE_H_
