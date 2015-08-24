#include "UdtProxy.h"

CUdtProxy * CUdtProxy::m_pUdtProxy = nullptr;

CUdtProxy::CUdtProxy()
{
	m_pUdt = new CUdtCore(this);
}


CUdtProxy::~CUdtProxy()
{
}

CUdtProxy * CUdtProxy::GetInStance()
{
	if (m_pUdtProxy == nullptr)
	{
		m_pUdtProxy = new CUdtProxy;
	}

	return m_pUdtProxy;
}

CUdtCore *CUdtProxy::core() const
{
	return m_pUdt;
}

//////////////////////////////////////////////////////////////////////////
// Call Back
void CUdtProxy::onAccept(const char* pstrAddr, const char* pstrFileName, int nFileCount, const char* recdevice, const char* rectype, const char* owndevice, const char* owntype, const char* SendType, int sock)
{
	std::cout<< "Accept file name:" << pstrFileName << std::endl;

	char path[MAX_PATH];
#if WIN32
	GetCurrentDirectoryA(MAX_PATH, path);
#else
	if (NULL == getcwd(path, MAX_PATH))
		Error("Unable to get current path");
#endif

	if ('\\' != path[strlen(path)-1])
	{
		strcat(path, ("\\"));
	}

	m_sock = sock;
	if (nFileCount == 1)
	{
		strcat(path, pstrFileName);
		m_pUdt->ReplyAccept(sock, path);
	}
	else
		m_pUdt->ReplyAccept(sock, path);

	std::cout<< "save file to:" << path << std::endl;
}

void CUdtProxy::onAcceptonFinish(const char* pstrAddr, const char* pFileName, int Type, int sock)
{
	if (Type == 1)
		std::cout<< "Recv onAcceptonFinish file name:" << pFileName << std::endl;
	else
		std::cout<< "Send onAcceptonFinish file name:" << pFileName << std::endl;
}

void CUdtProxy::onFinished(const char * pstrMsg, int Type, int sock)
{
	if (Type == 1)
		std::cout<< "onRecvFinished:" << pstrMsg << std::endl;
	else
		std::cout<< "onSendFinished:" << pstrMsg << std::endl;
}

void CUdtProxy::onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName, int Type, int sock)
{
	double dTotal = (double)nFileTotalSize;
	double dPercent = nCurrent / dTotal;
	static char * Tmp = "%";
	if (Type == 1)
		printf("Recv file name:%s, percent:%.2f%s\n", pstrFileName, dPercent*100, Tmp);
	else
		printf("Send file name:%s, percent:%.2f%s\n", pstrFileName, dPercent*100, Tmp);
}

void CUdtProxy::onRecvMessage(const char* pstrMsg, const char* pIpAddr, const char* pHostName)
{
	std::cout<< "Recv message:" << pstrMsg << std::endl;
}