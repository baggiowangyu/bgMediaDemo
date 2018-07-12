#ifndef _GMMediaCollection_H_
#define _GMMediaCollection_H_

/**
 * 本类定义了一个媒体采集器对象，用于采集计算机摄像头、麦克风的视音频数据
 * 根据用户设定，以指定编码的形式将数据包输出
 *
 * 其中：
 * 视频数据不改变分辨率，颜色模式，只做编码转换
 * 音频数据从麦克风采集出来以后以最优的采样率进行重编码
 */

class GmGxxStreamNotifier
{
public:
	virtual int StreamData(enum AVMediaType type, AVFrame *) = 0;
};

class GMMediaCollection
{
public:
	struct GmGxxDeviceInfo
	{
		std::string name_;
		std::string long_name_;
	};

public:
	GMMediaCollection();
	~GMMediaCollection();

public:
	int EnumDeviceInfo(std::vector<GmGxxDeviceInfo> &video_devs, std::vector<GmGxxDeviceInfo> &audio_devs);

public:
	int StartVideo(enum AVCodecID encode_id);
	int StartAudio(enum AVCodecID encode_id);

	int StopVideo();
	int StopAudio();
};

#endif//_GMMediaCollection_H_
