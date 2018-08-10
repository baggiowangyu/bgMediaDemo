// GMVideoCollectionV2.cpp : 定义控制台应用程序的入口点。
//

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

#include "SDL.h"
#include <iostream>

#include <windows.h>  
#include <vector>  
#include <dshow.h>  

#ifndef MACRO_GROUP_DEVICENAME  
#define MACRO_GROUP_DEVICENAME  

#define MAX_FRIENDLY_NAME_LENGTH    128  
#define MAX_MONIKER_NAME_LENGTH     256  

typedef struct _TDeviceName  
{  
	WCHAR FriendlyName[MAX_FRIENDLY_NAME_LENGTH];   // 设备友好名  
	WCHAR MonikerName[MAX_MONIKER_NAME_LENGTH];     // 设备Moniker名  
} TDeviceName;  
#endif

#pragma comment(lib, "Strmiids.lib")  

HRESULT DS_GetAudioVideoInputDevices( std::vector<TDeviceName> &vectorDevices, REFGUID guidValue )  
{  
	TDeviceName name;   
	HRESULT hr;  

	// 初始化  
	vectorDevices.clear();  

	// 初始化COM  
	hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );  
	if (FAILED(hr))  
	{  
		return hr;  
	}  

	// 创建系统设备枚举器实例  
	ICreateDevEnum *pSysDevEnum = NULL;  
	hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum );  
	if (FAILED(hr))  
	{  
		CoUninitialize();  
		return hr;  
	}  

	// 获取设备类枚举器  
	IEnumMoniker *pEnumCat = NULL;  
	hr = pSysDevEnum->CreateClassEnumerator( guidValue, &pEnumCat, 0 );  
	if (hr == S_OK)   
	{  
		// 枚举设备名称  
		IMoniker *pMoniker = NULL;  
		ULONG cFetched;  
		while(pEnumCat->Next( 1, &pMoniker, &cFetched ) == S_OK)  
		{  
			IPropertyBag *pPropBag;  
			hr = pMoniker->BindToStorage( NULL, NULL, IID_IPropertyBag, (void **)&pPropBag );  
			if (SUCCEEDED(hr))  
			{  
				// 获取设备友好名  
				VARIANT varName;  
				VariantInit( &varName );  

				hr = pPropBag->Read( L"FriendlyName", &varName, NULL );  
				if (SUCCEEDED(hr))  
				{  
					StringCchCopy( name.FriendlyName, MAX_FRIENDLY_NAME_LENGTH, varName.bstrVal );  

					// 获取设备Moniker名  
					LPOLESTR pOleDisplayName = reinterpret_cast<LPOLESTR>(CoTaskMemAlloc(MAX_MONIKER_NAME_LENGTH * 2));  
					if (pOleDisplayName != NULL)  
					{  
						hr = pMoniker->GetDisplayName( NULL, NULL, &pOleDisplayName );  
						if (SUCCEEDED(hr))  
						{  
							StringCchCopy( name.MonikerName, MAX_MONIKER_NAME_LENGTH, pOleDisplayName );  
							vectorDevices.push_back( name );  
						}  

						CoTaskMemFree( pOleDisplayName );  
					}  
				}  

				VariantClear( &varName );  
				pPropBag->Release();                       
			}  

			pMoniker->Release();  
		} // End for While  

		pEnumCat->Release();  
	}  

	pSysDevEnum->Release();  
	CoUninitialize();  

	return hr;  
}  


int _tmain(int argc, _TCHAR* argv[])
{
	av_register_all();
	avformat_network_init();
	avdevice_register_all();
	avcodec_register_all();

	//////////////////////////////////////////////////////////////////////////
	// 枚举摄像头
	//AVFormatContext *pFormatCtx = avformat_alloc_context();  
	//AVDictionary* options = NULL;  
	//av_dict_set(&options, "list_devices", "true", 0);  
	//AVInputFormat *iformat = av_find_input_format("dshow");  
	//printf("Device Info=============\n");
	//AVInputFormat *input_fmt = iformat;
	//while (input_fmt != NULL)
	//{
	//	printf("简称：%s\n全称：%s\n**********************\n", input_fmt->name, input_fmt->long_name);
	//	input_fmt = input_fmt->next;
	//}
	//avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);  
	//printf("========================\n");
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	//
	std::vector<TDeviceName> video_devices;
	DS_GetAudioVideoInputDevices(video_devices, CLSID_VideoInputDeviceCategory);

	std::vector<TDeviceName> audio_devices;
	DS_GetAudioVideoInputDevices(audio_devices, CLSID_AudioInputDeviceCategory);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 使用ffmpeg打开指定的摄像头设备
	AVFormatContext *pFormatCtx = avformat_alloc_context();  
	//AVDictionary* options = NULL;  
	//av_dict_set(&options, "list_options", "true", 0);  
	AVInputFormat *iformat = av_find_input_format("dshow");  
	char buffer[128] = {0};

	char err[AV_ERROR_MAX_STRING_SIZE] = {0};
	char *strerr = NULL;
	USES_CONVERSION;
	sprintf(buffer, "video=%s", W2A(video_devices[0].FriendlyName));  
	int errCode = avformat_open_input(&pFormatCtx, buffer, iformat, NULL); 
	if (errCode < 0)
	{
		strerr = av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, errCode);
	}
	//////////////////////////////////////////////////////////////////////////

	errCode = avformat_find_stream_info(pFormatCtx, NULL);

	int video_index = -1;
	AVStream *video_stream = NULL;
	AVCodec *video_codec = NULL;
	AVCodecContext *video_codec_ctx = NULL;

	int streams = pFormatCtx->nb_streams;
	for (int index = 0; index < streams; ++index)
	{
		if (pFormatCtx->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			// 视频流信息
			video_index = index;
			video_stream = pFormatCtx->streams[index];

			video_codec = avcodec_find_decoder(video_stream->codec->codec_id);
			if (video_codec == NULL)
			{
				printf("没有找到解码器\n");
				break;
			}

			video_codec_ctx = avcodec_alloc_context3(video_codec);
			if (video_codec_ctx == NULL)
			{
				printf("申请解码器上下文失败！");
				break;
			}

			errCode = avcodec_copy_context(video_codec_ctx, video_stream->codec);
			if (errCode < 0)
			{
				printf("复制解码上下文失败！错误码：%d\n", errCode);
				break;
			}

			errCode = avcodec_open2(video_codec_ctx, video_codec, NULL);
			if (errCode < 0)
			{
				printf("打开解码器失败！错误码：%d\n", errCode);
				break;
			}
		}
		else if (pFormatCtx->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			// 音频流信息
		}
	}

	if (errCode < 0)
	{
		return errCode;
	}

	// 我们这里要强制转为H.264或者H.265编码或者MPEG-4
	AVCodec *video_encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (video_encoder == NULL)
	{
		printf("未找到编码器\n");
		return -1;
	}

	AVCodecContext *video_encode_ctx = avcodec_alloc_context3(video_encoder);
	if (video_encode_ctx == NULL)
	{
		printf("申请编码上下文失败！\n");
		return -2;
	}

	// 补充其他元素
	video_encode_ctx->codec_type = AVMEDIA_TYPE_VIDEO;		// 媒体类型
	//video_encode_ctx->bit_rate = 600000;					// 视频码率
	video_encode_ctx->compression_level = 5;				// 压缩等级
	video_encode_ctx->width = video_codec_ctx->width;		// 视频宽
	video_encode_ctx->height = video_codec_ctx->height;		// 视频高
	video_encode_ctx->pix_fmt = video_codec_ctx->pix_fmt;	// 图像格式
	//video_encode_ctx->time_base = av_inv_q(video_codec_ctx->framerate);

	// 接下来读取
	while (true)
	{
		AVPacket pkt;
		errCode = av_read_frame(pFormatCtx, &pkt);
		if (errCode < 0)
			break;

		if (pkt.stream_index == video_index)
		{
			// 是视频流，先解码，再编码
			AVFrame *frm = av_frame_alloc();
			int got = 0;
			errCode = avcodec_decode_video2(video_codec_ctx, frm, &got, &pkt);
			if (!got)
			{
				printf("没有解码出图片");
				av_frame_free(&frm);
				continue;
			}
		
			// 重新编码
			int new_got = 0;
			AVPacket new_pkt;
			new_pkt.data = NULL;
			new_pkt.size = 0;
			av_init_packet(&new_pkt);
			errCode = avcodec_encode_video2(video_encode_ctx, &new_pkt, frm, &new_got);
			if (!new_got)
			{
				printf("编码失败！");
				av_frame_free(&frm);
				continue;
			}

			// 如果要输出到文件，需要设置流ID
			// 需要设置时间基
		}
	}

	return 0;
}

