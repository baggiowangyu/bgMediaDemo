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

	int streams = pFormatCtx->nb_streams;
	for (int index = 0; index < streams; ++index)
	{
		if (pFormatCtx->streams[index]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			// 视频流信息
		}
		else if (pFormatCtx->streams[index]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			// 音频流信息
		}
	}

	return 0;
}

