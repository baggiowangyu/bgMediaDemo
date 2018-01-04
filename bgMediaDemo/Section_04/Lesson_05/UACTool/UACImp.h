#ifndef _UAC_IMP_H_
#define _UAC_IMP_H_

#include <string>
#include <windows.h>

struct eXosip_t;

class bgUACImp
{
public:
	bgUACImp();
	~bgUACImp();

public:
	int Init(const char *code, const char *ip = "0.0.0.0", int port = 5090, const char *name = "_bgUAC_");
	void Close();

public:
	int SetUASEnvironment(const char *code, const char *ip, int port);

public:

	// ×¢²á
	int Register();
	// ×¢Ïú
	int Unregister();

	// ºô½Ð
	int Call();

	// ¹Ò¶Ï
	int ReleaseCall();

	// ·¢ËÍ¶ÌÏûÏ¢
	int SendSMS(const char *text);

private:
	std::string get_uac_sip_uri_();
	std::string get_uas_sip_uri_();

private:
	std::string uas_ip_;
	int uas_port_;
	std::string uas_code_;
	std::string uas_name_;

private:
	std::string uac_ip_;
	int uac_port_;
	std::string uac_code_;
	std::string uac_name_;

private:
	eXosip_t *sip_context_;
};

#endif//_UAC_IMP_H_
