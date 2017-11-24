// GMVideoMux.cpp : 定义控制台应用程序的入口点。
//
// 本范例将h264文件和aac文件封装为带音轨的MP4文件

#include "stdafx.h"

#include <atlconv.h>

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavutil/avutil.h"
#include "libavutil/rational.h"
#include "libavutil/pixdesc.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
};
#endif

#include <iostream>


int _tmain(int argc, _TCHAR* argv[])
{
	//if (argc < 4)
	//{
	//	printf("GMVideoMux.exe <video_url> <audio_url> <target_url>\n");
	//	return 0;
	//}

	//TCHAR video_url[4096] = {0};
	//_tcscpy_s(video_url, 4096, argv[1]);

	//TCHAR audio_url[4096] = {0};
	//_tcscpy_s(audio_url, 4096, argv[2]);

	//TCHAR target_url[4096] = {0};
	//_tcscpy_s(target_url, 4096, argv[3]);

	USES_CONVERSION;

	av_register_all();

	//////////////////////////////////////////////////////////////////////////
	//
	// 打开输入文件
	//
	//////////////////////////////////////////////////////////////////////////

	for (int index = 0; index <10; ++index)
	{
		unsigned char *data = new unsigned char[10];
	}

	_CrtDumpMemoryLeaks();
	return 0;
}

