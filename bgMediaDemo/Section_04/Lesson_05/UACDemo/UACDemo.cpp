// UACDemo.cpp : 定义控制台应用程序的入口点。
//
// 参考资料：http://blog.csdn.net/hiwubihe/article/details/44339125
//
// 在这个例子中，UAC和UAS通信，source是自己的地址，目标地址就是UAS

#include "stdafx.h"
#include "eXosip2/eXosip.h"
#include "eXosip2.h"

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
	struct eXosip_t *context_eXosip = new struct eXosip_t;
	int errCode = eXosip_init(context_eXosip);

	if (errCode != 0)
	{
		std::cout<<"注册 eXosip 失败！"<<std::endl;
		return errCode;
	}
	else
		std::cout<<"注册 eXosip 成功！"<<std::endl;

	// 绑定UAC自己的端口15060，并监听
	errCode = eXosip_listen_addr(context_eXosip, IPPROTO_UDP, NULL, 15060, AF_INET, 0);
	if (errCode != 0)
	{
		std::cout<<"初始化传输层失败！"<<std::endl;
		eXosip_quit(context_eXosip);
		return errCode;
	}
	else
		std::cout<<"初始化传输层成功！"<<std::endl;

	int flag = 1;

	
	// 全局性的变量
	osip_message_t *invite = NULL;	// 发起呼叫请求之后的事情

	int call_id = -1;
	int dialog_id = -1;

	while (flag)
	{
		std::cout<<"请输入命令：";
		char command_[4096] = {0};
		std::cin.clear();
		std::cin>>command_;
		std::cout<<std::endl;

		std::string command = command_;

		if (command.compare("r") == 0)
		{
			std::cout<<"暂未实现"<<std::endl;
		}
		else if (command.compare("i") == 0)
		{
			// 发起呼叫请求，INVITE
			errCode = eXosip_call_build_initial_invite(context_eXosip, &invite, dest_call, source_call, NULL, "This is a call for conversation");
			if (errCode != 0)
				std::cout<<"初始化 INVITE 失败"<<std::endl;
			else
			{
				// 创建 SDP，按经验来说，ffmpeg的av_create_sdp()也是可以生产SDP信息的
				char body[4096] = {0};
				strcpy_s(body, 4096,
					"v=0\r\n"
					"0=anonymous 0 0 IN IP4 0.0.0.0\r\n"  
					"t=1 10\r\n"  
					"a=username:rainfish\r\n"  
					"a=password:aaaaaa\r\n");

				osip_message_set_body(invite, body, strlen(body));
				osip_message_set_content_type(invite, "application/sdp");

				eXosip_lock(context_eXosip);
				errCode = eXosip_call_send_initial_invite(context_eXosip, invite);
				eXosip_unlock(context_eXosip);

				// 消息已发送，等待应答
				int flag_1 = 1;
				while (flag_1)
				{
					eXosip_event_t *event = eXosip_event_wait(context_eXosip, 0, 200);

					if (event == NULL)
					{
						// 超时了
						std::cout<<"INVITE 应答等待超时"<<std::endl;
						break;
					}

					switch (event->type)
					{
					case EXOSIP_CALL_INVITE:
						std::cout<<"收到一个新的 INVITE 请求"<<std::endl;
						break;
					case EXOSIP_CALL_PROCEEDING:
						std::cout<<"收到 100 trying消息，表示请求正在处理中"<<std::endl;
						break;
					case EXOSIP_CALL_RINGING:
						std::cout<<"收到 180 Ringing应答，表示接收到INVITE请求的UAS正在向被叫用户振铃"<<std::endl;
						std::cout<<"call_id : "<<event->cid<<", dialog_id : "<<event->did<<std::endl;
						break;
					case EXOSIP_CALL_ANSWERED:
						{
							std::cout<<"收到 200 OK，表示请求已经被成功接受，用户应答"<<std::endl;
							call_id = event->cid;
							dialog_id = event->did;
							std::cout<<"call_id : "<<event->cid<<", dialog_id : "<<event->did<<std::endl;

							// 回应ack应答
							osip_message_t *ack = NULL;
							eXosip_call_build_ack(context_eXosip, dialog_id, &ack);
							eXosip_call_send_ack(context_eXosip, dialog_id, ack);

							// 退出这个消息循环
							flag_1 = 0;
						}
						break;
					case EXOSIP_CALL_CLOSED:
						std::cout<<"收到一个 BYE 消息"<<std::endl;
						break;
					case EXOSIP_CALL_ACK:
						std::cout<<"收到一个 ACK"<<std::endl;
						break;
					default:
						std::cout<<"收到了其他应答，这里不处理"<<std::endl;
						break;
					}

					eXosip_event_free(event);
				}
			}
		}
		else if (command.compare("h") == 0)
		{
			// 挂断
			std::cout<<"执行挂断"<<std::endl;

			eXosip_lock(context_eXosip);
			eXosip_call_terminate(context_eXosip, call_id, dialog_id);
			eXosip_unlock(context_eXosip);
		}
		else if (command.compare("c") == 0)
		{
			// 
			std::cout<<"此功能未完成"<<std::endl;
		}
		else if (command.compare("s") == 0)
		{
			// 传输 INFO 方法
			std::cout<<"发送 INFO 消息"<<std::endl;

			osip_message_t *info = NULL;
			eXosip_call_build_info(context_eXosip, dialog_id, &info);

			char msg[4096] = {0};
			strcpy_s(msg, 4096, "This a sip message (Method : INFO)");
			osip_message_set_body(info, msg, strlen(msg));

			// 格式可以任意设定，28181协议似乎是固定为XML了
			osip_message_set_content_type(info, "text/plain");
			eXosip_call_send_request(context_eXosip, dialog_id, info);
		}
		else if (command.compare("m") == 0)
		{
			// 传输 MESSAGE 方法，即时消息
			// 与 INFO 不同的是， MESSAGE 方法不许要建立在 INVITE 的基础上
			std::cout<<"发送 MESSAGE 消息"<<std::endl;

			osip_message_t *message = NULL;
			eXosip_message_build_request(context_eXosip, &message, "MESSAGE", dest_call, source_call, NULL);

			char msg[4096] = {0};
			strcpy_s(msg, 4096, "This is a MESSAGE message...");
			osip_message_set_body(message, msg, strlen(msg));

			// 假设格式是xml
			osip_message_set_content_type(message, "text/xml");

			// 发送消息
			eXosip_message_send_request(context_eXosip, message);
		}
		else if (command.compare("q") == 0)
		{
			// 
			std::cout<<"即将退出..."<<std::endl;
			eXosip_quit(context_eXosip);
			flag = 0;
		}
	}

	system("pause");
	return 0;
}

