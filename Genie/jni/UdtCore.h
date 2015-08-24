#ifndef __UDTCORE_H__
#define __UDTCORE_H__

#ifndef WIN32
	#include <sys/time.h>
	#include <sys/uio.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <cstdlib>
	#include <cstring>
	#include <netdb.h>
	#include <pthread.h>
	#include <errno.h>
	#include <dirent.h>
	#include <time.h>
#else
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <wspiapi.h>
	#include <direct.h>
	#include <io.h>
#endif

#include <iostream>
#include <sys/stat.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdio.h>

#include "udt.h"
#include "common.h"

class CUDTCallBack
{
public:
	virtual void onAccept(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount) = 0;
	virtual void onAcceptFolder(const char* pstrAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount) = 0;
	virtual void onAcceptonFinish(const char* pstrAddr, std::vector<std::string> vecFileName) = 0;
	virtual void onSendFinished(const char * pstrMsg) = 0;
	virtual void onRecvFinished(const char * pstrMsg) = 0;
	virtual void onSendTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName) = 0;
	virtual void onRecvTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName) = 0;
	virtual void onRecvMessage(const char* pstrIpAddr, const char* pstrHostName, const char* pstrMsg) = 0;
};


class CUdtCore
{
public:
	CUdtCore(CUDTCallBack * pCallback);
	~CUdtCore();

	enum SENDTYPE
	{
		SND_NULL = 0,
		SND_MSG,			// send message
		SND_UNIFILE,		// send unifile
		SND_MULFILE,		// send multyfile
		SND_FOLDER		// send folder
	};

	struct _stClientSocket
	{
		UDTSOCKET sock;
		char strAddr[32];
		char strPort[32];
	};

	struct _stServerContext
	{
		UDTSOCKET sock;
		char strXSR[8];
		char strXSP[8];
		char strXCS[8];
		char strXSF[8];
		char strAddr[32];
		char strPort[32];
		char strHostName[128];
		char strSendtype[128];
		std::vector<std::string> vecArray;
	};

	int listenFileSend(const int nPort);
	void SendEx(const char* pstrAddr, const int nPort, const char* pstrHostName, const char* pstrSendtype, const std::vector<std::string> vecArray, int  nType);
	void replyAccept(const char* pstrReply);
	void stopTransfer(const int nType);
	void stopListen();

private:
	void SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName);
	void CreateDirectroy(const std::string & szPath);
private:
	CUDTCallBack * m_pCallBack;

	_stClientSocket * m_pClientSocket;
	_stServerContext * m_pSrvCxt;

	int m_nPortListen;
	UDTSOCKET m_sockListen;
	std::string m_szReplyfilepath;
	bool m_bSendStatus;
	bool m_bRecvStatus;
	bool m_bListenStatus;

	pthread_mutex_t m_AcceptLock;
	pthread_mutex_t m_SendLock;
	pthread_mutex_t m_RecvLock;

	pthread_cond_t m_AcceptCond;
	pthread_cond_t m_SendCond;
	pthread_cond_t m_RecvCond;

	pthread_t m_hListenThread;
	pthread_t m_hSendThread;
	pthread_t m_hRecvThread;
#ifndef WIN32
	static void * _ListenThreadProc(void * pParam);
	static void * _SendThreadProc(void * pParam);
	static void * _RecvThreadProc(void * pParam);
#else
	static DWORD WINAPI _ListenThreadProc(LPVOID pParam);
	static DWORD WINAPI _SendThreadProc(LPVOID pParam);
	static DWORD WINAPI _RecvThreadProc(LPVOID pParam);
#endif
};

#endif	// __UDTCORE_H__
