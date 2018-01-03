#ifndef _UAS_IMP_H_
#define _UAS_IMP_H_

#include <string>
#include <windows.h>

struct eXosip_t;

class bgUASImp
{
public:
	bgUASImp();
	~bgUASImp();

public:
	int Init(const char *code, const char *ip = "0.0.0.0", int port = 5060, const char *name = "_bgUAS_");
	void Close();

public:
	int Start();
	//void Stop();

public:
	static DWORD WINAPI WorkingThread(LPVOID lpParam);

public:
	eXosip_t* get_context_();
	std::string get_uas_ip_();
	int get_uas_port_();
	std::string get_uas_code_();
	std::string get_uas_name_();

private:
	std::string uas_ip_;
	int uas_port_;
	std::string uas_code_;
	std::string uas_name_;

private:
	bool is_thread_working_;
	CRITICAL_SECTION start_lock_;

private:
	eXosip_t *sip_context_;
};

#endif//_UAS_IMP_H_
