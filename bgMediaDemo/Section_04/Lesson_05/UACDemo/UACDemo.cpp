// UACDemo.cpp : 定义控制台应用程序的入口点。
//
// 参考资料：http://blog.csdn.net/hiwubihe/article/details/44339125
//
// 在这个例子中，UAC和UAS通信，source是自己的地址，目标地址就是UAS

#include "stdafx.h"
#include "eXosip2/eXosip.h"

#include <iostream>
#include <WinSock2.h>


int _tmain(int argc, _TCHAR* argv[])
{
	const char *identity = "sip:140@127.0.0.1";
	const char *uas = "sip:133@127.0.0.1:15061";
	const char *source_call = "sip:140@127.0.0.1";
	const char *dest_call = "sip:133@127.0.0.1:15061";

	std::cout<<"SIP 客户端测试程序"<<std::endl;
	std::cout<<"r - 向SIP服务器发起注册"<<std::endl;
	std::cout<<"c - 取消注册(注销?)"<<std::endl;
	std::cout<<"i - 发起呼叫请求"<<std::endl;
	std::cout<<"h - 挂断呼叫"<<std::endl;
	std::cout<<"q - 退出程序"<<std::endl;
	std::cout<<"s - 执行 INFO 方法"<<std::endl;
	std::cout<<"m - 执行 MESSAGE 方法"<<std::endl;

	// 初始化eXosip
	struct eXosip_t *context_eXosip = NULL;
	int errCode = eXosip_init(context_eXosip);

	if (errCode != 0)
		return errCode;

	std::cout<<"注册 eXosip 成功！"<<std::endl;

	return 0;
}

