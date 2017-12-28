// GMAudioTranscodeDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "GMAudioTranscode.h"

#include <iostream>
#include <atlconv.h>


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 3)
	{
		printf("GMAudioTranscodeDemo.exe <source_url> <target_url> \n");
		return 0;
	}

	int errCode = 0;

	USES_CONVERSION;

	TCHAR source_url[4096] = {0};
	_tcscpy_s(source_url, 4096, argv[1]);

	TCHAR target_url[4096] = {0};
	_tcscpy_s(target_url, 4096, argv[2]);


	GMMediaTranscoder transcode;
	errCode = transcode.Transcode(T2A(source_url), T2A(target_url));
	std::cout<<"GMMediaTranscoder::Transcode(\""<<T2A(source_url)<<"\", \""<<T2A(target_url)<<"\"); errCode : "<<errCode<<std::endl;

	system("pause");
	return 0;
}

