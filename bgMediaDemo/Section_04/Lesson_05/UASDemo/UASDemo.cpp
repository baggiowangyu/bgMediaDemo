// UASDemo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "eXosip2/eXosip.h"
#include "eXosip2.h"

#include <iostream>
#include <WinSock2.h>


int _tmain(int argc, _TCHAR* argv[])
{
	const char *sour_call = "sip:140@127.0.0.1";
	const char *dest_call = "sip:133@127.0.0.1:15060";  //client ip

	// 初始化eXosip
	struct eXosip_t context_eXosip;
	int errCode = eXosip_init(&context_eXosip);

	if (errCode != 0)
	{
		std::cout<<"注册 eXosip 失败！"<<std::endl;
		return errCode;
	}
	else
		std::cout<<"注册 eXosip 成功！"<<std::endl;


	// 绑定UAC自己的端口15060，并监听
	errCode = eXosip_listen_addr(&context_eXosip, IPPROTO_UDP, NULL, 15061, AF_INET, 0);
	if (errCode != 0)
	{
		std::cout<<"初始化传输层失败！"<<std::endl;
		eXosip_quit(&context_eXosip);
		return errCode;
	}
	else
		std::cout<<"初始化传输层成功！"<<std::endl;


	while (true)
	{
		// 侦听是否有消息到来
		eXosip_event_t *event = eXosip_event_wait(&context_eXosip, 0, 50);

		// 下面这几个语句是协议栈中要用的，目前还不知道干嘛用
		eXosip_lock(&context_eXosip);
		eXosip_default_action(&context_eXosip, event);
		eXosip_automatic_refresh(&context_eXosip);
		eXosip_unlock(&context_eXosip);

		if (event == NULL)
			continue;

		switch (event->type)
		{
		case EXOSIP_MESSAGE_NEW:
			{
				std::cout<<"EXOSIP_MESSAGE_NEW 收到新的消息！"<<std::endl;

				if (MSG_IS_MESSAGE(event->request))
				{
					// 接收到的消息是 MESSAGE
					osip_body_t *body = NULL;
					osip_message_get_body(event->request, 0, &body);
					std::cout<<"收到的消息是 MESSAGE 消息，内容为："<<body->body<<std::endl;
				}

				// 按照协议要求，我们要返回OK信息
				osip_message_t *answer = NULL;
				eXosip_message_build_answer(&context_eXosip, event->tid, 200, &answer);
				eXosip_message_send_answer(&context_eXosip, event->tid, 200, answer);
			}
			break;
		case EXOSIP_CALL_INVITE:
			{
				const char *host = event->request->req_uri->host;
				const char *port = event->request->req_uri->port;
				const char *username = event->request->req_uri->username;
				const char *password = event->request->req_uri->password;
				std::cout<<"收到了一个 INVITE 消息，发送："<<host<<":"<<port<<", 用户名："<<username<<", 密码："<<(password == NULL ? "无" : password)<<std::endl;

				// 取出消息体，我们默认认为就是sdp格式
				sdp_message_t *remote_sdp = eXosip_get_remote_sdp(&context_eXosip, event->did);

				int call_id = event->cid;
				int dialog_id = event->did;

				eXosip_lock(&context_eXosip);
				errCode = eXosip_call_send_answer(&context_eXosip, event->tid, 180, NULL);

				osip_message_t *answer = NULL;
				errCode = eXosip_call_build_answer(&context_eXosip, event->tid, 200, &answer);
				if (errCode != 0)
				{
					std::cout<<"这条请求消息不可用，不必应答"<<std::endl;
					eXosip_call_send_answer(&context_eXosip, event->tid, 400, NULL);
				}
				else
				{
					// 
					char msg[4096] = {0};
					sprintf_s(msg, 4096,
						"v=0\r\n"  
						"o=anonymous 0 0 IN IP4 0.0.0.0\r\n"  
						"t=1 10\r\n"  
						"a=username:rainfish\r\n"  
						"a=password:123\r\n");

					// 设置回复的SDP消息体
					osip_message_set_body(answer, msg, strlen(msg));
					osip_message_set_content_type(answer, "application/sdp");

					eXosip_call_send_answer(&context_eXosip, event->tid, 200, answer);
					std::cout<<"成功发送200应答"<<std::endl;
				}
				eXosip_unlock(&context_eXosip);

				// 显示出在sdp消息体重的attribute的内容，里面计划存放我们的信息
				std::cout<<"INFO 内容为："<<std::endl;
				int pos = 0;
				while (!osip_list_eol(&(remote_sdp->a_attributes), pos))
				{
					sdp_attribute_t *at = (sdp_attribute_t *)osip_list_get(&remote_sdp->a_attributes, pos);
					std::cout<<"FIELD : "<<at->a_att_field<<" ,VALUE : "<<at->a_att_value<<std::endl;

					++pos;
				}
			}
			break;
		case EXOSIP_CALL_ACK:
			std::cout<<"收到 ACK ..."<<std::endl;
			break;
		case EXOSIP_CALL_CLOSED:
			{
				std::cout<<"收到 Closed ..."<<std::endl;
				//
				osip_message_t *answer = NULL;
				errCode = eXosip_call_build_answer(&context_eXosip, event->tid, 200, &answer);
				if (errCode != 0)
				{
					std::cout<<"这条请求消息不可用，不必应答"<<std::endl;
					eXosip_call_send_answer(&context_eXosip, event->tid, 400, NULL);
				}
				else
				{
					eXosip_call_send_answer(&context_eXosip, event->tid, 200, answer);
					std::cout<<"成功发送200应答"<<std::endl;
				}
			}
			break;
		case EXOSIP_CALL_MESSAGE_NEW:
			std::cout<<"EXOSIP_CALL_MESSAGE_NEW"<<std::endl;
			{
				if (MSG_IS_INFO(event->request))
				{
					eXosip_lock(&context_eXosip);
					osip_message_t *answer = NULL;
					errCode = eXosip_call_build_answer(&context_eXosip, event->tid, 200, &answer);
					if (errCode == 0)
						eXosip_call_send_answer(&context_eXosip, event->tid, 200, answer);
					eXosip_unlock(&context_eXosip);
				}

				osip_body_t *body = NULL;
				osip_message_get_body(event->request, 0, &body);
				std::cout<<"MESSAGE 主干："<<body->body<<std::endl;
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

