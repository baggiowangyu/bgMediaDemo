#include "stdafx.h"

#include "eXosip2/eXosip.h"
#include "eXosip2.h"
#include "UASImp.h"




bgUASImp::bgUASImp()
: sip_context_(NULL)
, is_thread_working_(false)
{

}

bgUASImp::~bgUASImp()
{

}

int bgUASImp::Init(const char *code, const char *ip /* = "0.0.0.0" */, int port /* = 5060 */, const char *name /* = "_bgUAS_" */)
{
	int errCode = 0;

	InitializeCriticalSection(&start_lock_);

	// 初始化 eXosip 库
	sip_context_ = eXosip_malloc();
	if (sip_context_ == NULL)
		return -1;

	errCode = eXosip_init(sip_context_);
	if (errCode != 0)
		return errCode;

	// 本地开启监听
	errCode = eXosip_listen_addr(sip_context_, IPPROTO_UDP, ip, port, AF_INET, 0);
	if (errCode == 0)
	{
		uas_ip_ = ip;
		uas_port_ = port;
		uas_name_ = name;
		uas_code_ = code;
	}

	return errCode;
}

void bgUASImp::Close()
{
	if (sip_context_)
		eXosip_quit(sip_context_);
	sip_context_ = NULL;
}

int bgUASImp::Start()
{
	EnterCriticalSection(&start_lock_);

	if (is_thread_working_)
		return 0;

	int errCode = 0;

	// 启动线程，处理各种事件
	HANDLE thread_handle = CreateThread(NULL, 0, WorkingThread, this, 0, NULL);
	if (thread_handle == NULL)
		errCode = GetLastError();
	else
	{
		is_thread_working_ = true;
		CloseHandle(thread_handle);
	}
	
	LeaveCriticalSection(&start_lock_);
	return errCode;
}

eXosip_t* bgUASImp::get_context_()
{
	return sip_context_;
}

std::string bgUASImp::get_uas_ip_()
{
	return uas_ip_;
}

int bgUASImp::get_uas_port_()
{
	return uas_port_;
}

std::string bgUASImp::get_uas_code_()
{
	return uas_code_;
}

std::string bgUASImp::get_uas_name_()
{
	return uas_name_;
}

DWORD WINAPI bgUASImp::WorkingThread(LPVOID lpParam)
{
	bgUASImp *uas = (bgUASImp*)lpParam;
	eXosip_t *sip_context = uas->get_context_();

	while (true)
	{
		// 等待SIP事件
		eXosip_event_t *sip_event = eXosip_event_wait(sip_context, 0, 50);

		eXosip_lock(sip_context);
		eXosip_default_action(sip_context, sip_event);
		eXosip_automatic_refresh(sip_context);
		eXosip_unlock(sip_context);

		if (sip_event == NULL)
			continue;

		switch (sip_event->type)
		{
		//////////////////////////////////////////////////////////////////////////
		// 注册相关事件
		//////////////////////////////////////////////////////////////////////////
		case EXOSIP_REGISTRATION_NEW:
			// 新注册信息通知
			OutputDebugString(_T("来了一条注册请求"));
			{
				// 准备发送的数据
				int request_id = 0;				// 注册请求ID
				int expires = 0;				// 超时时间
				const char *code = NULL;		// 国标码
				const char *ip = NULL;			// UAC-IP
				const char *port = NULL;		// UAC-PORT
				const char *content = NULL;		// 正文
				bool is_authen_null = true;		// 是否有提交认证数据
				const char *user_name = NULL;
				const char *algorithm = NULL;
				const char *realm = NULL;
				const char *nonce = NULL;
				const char *response = NULL;
				const char *uri = NULL;

				// 首先检查expires
				osip_header_t* header_expires = NULL;
				osip_message_header_get_byname(sip_event->request, "expires", 0, &header_expires);
				if (NULL != header_expires && NULL != header_expires->hvalue)
					expires = atoi(header_expires->hvalue);

				// 注册返回，由发送方维护的请求ID，接收方接收后原样返回即可
				request_id = sip_event->tid;

				// 查看contact字段
				osip_contact_t* contact = NULL;
				osip_message_get_contact(sip_event->request, 0, &contact);
				if (NULL != contact)
				{
					code = contact->url->username;
					ip = contact->url->host;
					port = contact->url->port;
				}

				// 解析 注册消息体
				osip_body_t * body = NULL;
				osip_message_get_body(sip_event->request, 0, &body);
				if (NULL != body)
					content = body->body;

				// 鉴权信息
				osip_authorization_t* authentication = NULL;
				osip_message_get_authorization(sip_event->request, 0, &authentication);
				if (NULL == authentication)
					is_authen_null = true;
				else
				{
					is_authen_null = false;
					user_name = authentication->username;
					algorithm = authentication->algorithm;
					realm = authentication->realm;
					nonce = authentication->nonce;
					response = authentication->response;
					uri = authentication->uri;
				}

				//
				// 一切准备就绪，发送应答
				int status = 500;
				if (is_authen_null)
					status = 401;
				else
					status = 200;

				eXosip_lock(sip_context);
				osip_message_t* answer = NULL;
				int result = ::eXosip_message_build_answer(sip_context, request_id, status, &answer);

				if (401 == status)
				{
					// 由SIP库生成认证方法和认证参数发送给客户端
					const char *random = "9bd055";
					const char *alg = "MD5";
					char stream[4096] = {0};
					sprintf_s(stream, 4096, "Digest realm=\"%s\",nonce=\"%s\",algorithm=%s", ip, random, alg);
					osip_message_set_header(answer, "WWW-Authenticate", stream);
				}
				else if (200 == status)
				{
					char header[4096] = {0};
					sprintf_s(header, 4096, "<sip:%s@%s:%s>;expires=%d", code, ip, port, expires);
					osip_message_set_header(answer, "Contact", header);
				}
				else
				{
					// Do nothing ...
				}

				if (OSIP_SUCCESS != result)
					eXosip_message_send_answer(sip_context, request_id, 400, NULL);
				else
					eXosip_message_send_answer(sip_context, request_id, status, answer);

				if (0 == expires)
					eXosip_register_remove(sip_context, request_id);

				eXosip_unlock(sip_context);
			}
			break;
		case EXOSIP_REGISTRATION_SUCCESS:
			// 用户注册成功
			OutputDebugString(_T("EXOSIP_REGISTRATION_SUCCESS"));
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			// 用户注册失败
			OutputDebugString(_T("EXOSIP_REGISTRATION_FAILURE"));
			break;
		case EXOSIP_REGISTRATION_REFRESHED:
			// 注册已经被刷新
			OutputDebugString(_T("EXOSIP_REGISTRATION_REFRESHED"));
			break;
		case EXOSIP_REGISTRATION_TERMINATED:
			// UA 不再执行注册
			OutputDebugString(_T("EXOSIP_REGISTRATION_TERMINATED"));
			break;

		//////////////////////////////////////////////////////////////////////////
		// 通话中INVITE相关事件
		//////////////////////////////////////////////////////////////////////////
		case EXOSIP_CALL_INVITE:
			// 新的邀请通话通知
			OutputDebugString(_T("EXOSIP_CALL_INVITE"));
			break;
		case EXOSIP_CALL_REINVITE:
			// 邀请通话过程中来了一个新的邀请通知
			OutputDebugString(_T("EXOSIP_CALL_REINVITE"));
			break;
		case EXOSIP_CALL_NOANSWER:
			// 没有应答，直至超时
			OutputDebugString(_T("EXOSIP_CALL_NOANSWER"));
			break;
		case EXOSIP_CALL_PROCEEDING:
			// 远程应用正在处理
			OutputDebugString(_T("EXOSIP_CALL_PROCEEDING"));
			break;
		case EXOSIP_CALL_RINGING:
			// 给主叫回铃音
			OutputDebugString(_T("EXOSIP_CALL_RINGING"));
			break;
		case EXOSIP_CALL_ANSWERED:
			// 通话开始
			OutputDebugString(_T("EXOSIP_CALL_ANSWERED"));
			break;
		case EXOSIP_CALL_REDIRECTED:
			// 呼叫转移
			OutputDebugString(_T("EXOSIP_CALL_REDIRECTED"));
			break;
		case EXOSIP_CALL_REQUESTFAILURE:
			// 请求失败
			OutputDebugString(_T("EXOSIP_CALL_REQUESTFAILURE"));
			break;
		case EXOSIP_CALL_SERVERFAILURE:
			// 服务器失败
			OutputDebugString(_T("EXOSIP_CALL_SERVERFAILURE"));
			break;
		case EXOSIP_CALL_GLOBALFAILURE:
			// 全局失败
			OutputDebugString(_T("EXOSIP_CALL_GLOBALFAILURE"));
			break;
		case EXOSIP_CALL_ACK:
			// INVITE返回200，就会收到一个ACK
			OutputDebugString(_T("EXOSIP_CALL_ACK"));
			break;
		case EXOSIP_CALL_CANCELLED:
			// 通话被取消
			OutputDebugString(_T("EXOSIP_CALL_CANCELLED"));
			break;
		case EXOSIP_CALL_TIMEOUT:
			// 通话超时
			OutputDebugString(_T("EXOSIP_CALL_TIMEOUT"));
			break;

		//////////////////////////////////////////////////////////////////////////
		// 通话中除了INVITE之外的其他请求
		//////////////////////////////////////////////////////////////////////////
		case EXOSIP_CALL_MESSAGE_NEW:
			// 一个新的输入请求
			OutputDebugString(_T("EXOSIP_CALL_MESSAGE_NEW"));
			break;
		case EXOSIP_CALL_MESSAGE_PROCEEDING:
			// 请求返回了 1xx 状态码
			OutputDebugString(_T("EXOSIP_CALL_MESSAGE_PROCEEDING"));
			break;
		case EXOSIP_CALL_MESSAGE_ANSWERED:
			// 请求返回 200 状态码
			OutputDebugString(_T("EXOSIP_CALL_MESSAGE_ANSWERED"));
			break;
		case EXOSIP_CALL_MESSAGE_REDIRECTED:
			// 错误
			OutputDebugString(_T("EXOSIP_CALL_MESSAGE_REDIRECTED"));
			break;
		case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
			// 错误
			OutputDebugString(_T("EXOSIP_CALL_MESSAGE_REQUESTFAILURE"));
			break;
		case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
			// 错误
			OutputDebugString(_T("EXOSIP_CALL_MESSAGE_SERVERFAILURE"));
			break;
		case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
			// 错误
			OutputDebugString(_T("EXOSIP_CALL_MESSAGE_GLOBALFAILURE"));
			break;
		case EXOSIP_CALL_CLOSED:
			// 本次通话收到一个BYE消息
			OutputDebugString(_T("EXOSIP_CALL_CLOSED"));
			break;

		//////////////////////////////////////////////////////////////////////////
		// 这个消息 UAC 和 UAS 都可以用到
		//////////////////////////////////////////////////////////////////////////
		case EXOSIP_CALL_RELEASED:
			// 通话上下文被清理
			OutputDebugString(_T("EXOSIP_CALL_RELEASED"));
			break;

		//////////////////////////////////////////////////////////////////////////
		// 外部调用的请求收到了响应数据
		//////////////////////////////////////////////////////////////////////////
		case EXOSIP_MESSAGE_NEW:
			// 一个新的输入请求
			OutputDebugString(_T("EXOSIP_MESSAGE_NEW"));
			break;
		case EXOSIP_MESSAGE_PROCEEDING:
			// 请求返回了 1xx 状态码
			OutputDebugString(_T("EXOSIP_MESSAGE_PROCEEDING"));
			break;
		case EXOSIP_MESSAGE_ANSWERED:
			// 请求返回 200 状态码
			OutputDebugString(_T("EXOSIP_MESSAGE_ANSWERED"));
			break;
		case EXOSIP_MESSAGE_REDIRECTED:
			// 错误
			OutputDebugString(_T("EXOSIP_MESSAGE_REDIRECTED"));
			break;
		case EXOSIP_MESSAGE_REQUESTFAILURE:
			// 错误
			OutputDebugString(_T("EXOSIP_MESSAGE_REQUESTFAILURE"));
			break;
		case EXOSIP_MESSAGE_SERVERFAILURE:
			// 错误
			OutputDebugString(_T("EXOSIP_MESSAGE_SERVERFAILURE"));
			break;
		case EXOSIP_MESSAGE_GLOBALFAILURE:
			// 错误
			OutputDebugString(_T("EXOSIP_MESSAGE_GLOBALFAILURE"));
			break;

		//////////////////////////////////////////////////////////////////////////
		// 即时消息
		//////////////////////////////////////////////////////////////////////////
		case EXOSIP_SUBSCRIPTION_UPDATE:
			// 来了一个 SUBSCRIBE（订阅） 消息
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_UPDATE"));
			break;
		case EXOSIP_SUBSCRIPTION_CLOSED:
			// 订阅结束
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_CLOSED"));
			break;
		case EXOSIP_SUBSCRIPTION_NOANSWER:
			// 订阅没有应答
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_NOANSWER"));
			break;
		case EXOSIP_SUBSCRIPTION_PROCEEDING:
			// 订阅处理中，返回 1xx 状态码
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_PROCEEDING"));
			break;
		case EXOSIP_SUBSCRIPTION_ANSWERED:
			// 订阅应答，返回 200 状态码
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_ANSWERED"));
			break;
		case EXOSIP_SUBSCRIPTION_REDIRECTED:
			// 订阅重定向
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_REDIRECTED"));
			break;
		case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
			// 订阅请求失败
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_REQUESTFAILURE"));
			break;
		case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
			// 订阅服务器失败
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_SERVERFAILURE"));
			break;
		case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
			// 订阅全局失败
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_GLOBALFAILURE"));
			break;
		case EXOSIP_SUBSCRIPTION_NOTIFY:
			// 一个新的阅通知
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_NOTIFY"));
			break;
		case EXOSIP_SUBSCRIPTION_RELEASED:
			// 订阅上下文被释放
			OutputDebugString(_T("EXOSIP_SUBSCRIPTION_RELEASED"));
			break;
		case EXOSIP_IN_SUBSCRIPTION_NEW:
			// 一个新的订阅请求
			OutputDebugString(_T("EXOSIP_IN_SUBSCRIPTION_NEW"));
			break;
		case EXOSIP_IN_SUBSCRIPTION_RELEASED:
			// 订阅结束
			OutputDebugString(_T("EXOSIP_IN_SUBSCRIPTION_RELEASED"));
			break;
		case EXOSIP_NOTIFICATION_NOANSWER:
			// 通知，无应答
			OutputDebugString(_T("EXOSIP_NOTIFICATION_NOANSWER"));
			break;
		case EXOSIP_NOTIFICATION_PROCEEDING:
			// 通知，处理中，返回 1xx 状态码
			OutputDebugString(_T("EXOSIP_NOTIFICATION_PROCEEDING"));
			break;
		case EXOSIP_NOTIFICATION_ANSWERED:
			// 通知，已应答，返回 200 状态码
			OutputDebugString(_T("EXOSIP_NOTIFICATION_ANSWERED"));
			break;
		case EXOSIP_NOTIFICATION_REDIRECTED:
			// 通知，被重定向
			OutputDebugString(_T("EXOSIP_NOTIFICATION_REDIRECTED"));
			break;
		case EXOSIP_NOTIFICATION_REQUESTFAILURE:
			// 通知，请求失败
			OutputDebugString(_T("EXOSIP_NOTIFICATION_REQUESTFAILURE"));
			break;
		case EXOSIP_NOTIFICATION_SERVERFAILURE:
			// 通知，服务器失败
			OutputDebugString(_T("EXOSIP_NOTIFICATION_SERVERFAILURE"));
			break;
		case EXOSIP_NOTIFICATION_GLOBALFAILURE:
			// 通知，全局失败
			OutputDebugString(_T("EXOSIP_NOTIFICATION_GLOBALFAILURE"));
			break;
		default:
			break;
		}
	}

	return 0;
}