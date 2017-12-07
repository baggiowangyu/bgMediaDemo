#include "stdafx.h"

#include "GBT28181_Interface.h"
#include "third_party/eXosip/include/eXosip2/eXosip.h"

GBT28181Interface::GBT28181Interface()
: context_(NULL)
{

}

GBT28181Interface::~GBT28181Interface()
{

}

int GBT28181Interface::Init()
{
	int errCode = OSIP_SUCCESS;

	// ��ʼ��eXosip��
	errCode = eXosip_init();
	if (errCode != OSIP_SUCCESS)
		return errCode;

	eXosip_set_user_agent(NULL);

	// ��������ʱ��֪���Ǹ�ʲô��
	errCode = eXosip_listen_addr(IPPROTO_UDP, NULL, 5061)
}