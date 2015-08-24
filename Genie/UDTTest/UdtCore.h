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
#include "cc.h"
#include "common.h"

#define TO_SND 1024*500		// 文件发送数据块大小：((1(byte) * 1024)(kb) * 1024)(mb)

class CUDTCallBack
{
public:
	virtual void onAccept(const char* pstrAddr, const char* pstrFileName, int nFileCount, const char* recdevice, const char* rectype, const char* owndevice, const char* owntype, const char* SendType, int sock) = 0;
	virtual void onAcceptonFinish(const char* pstrAddr, const char* pFileName, int Type, int sock) = 0;
	virtual void onFinished(const char * pstrMsg, int Type, int sock) = 0;
	virtual void onTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName, int Type, int sock) = 0;
	virtual void onRecvMessage(const char* pstrMsg, const char* pIpAddr, const char* pHostName) = 0;
};

class CUdtCore
{
public:
	CUdtCore(CUDTCallBack * pCallback);
	~CUdtCore();

	int StartListen(const int nCtrlPort, const int nFilePort);
	int SendMsg(const char* pstrAddr, const char* pstrMsg, const char* pstrHostName);
	int SendFiles(const char* pstrAddr, const std::vector<std::string> vecFiles, const char* owndevice, const char* owntype, const char* recdevice, const char* rectype, const char* pstrSendtype);
	void ReplyAccept(const UDTSOCKET sock, const char* pstrReply);
	void StopTransfer(const UDTSOCKET sock, const int nType);
	void StopListen();

private:
	enum OP_TYPE
	{
		OP_SND_CTRL = 0,
		OP_SND_FILE,
		OP_RCV_CTRL,
		OP_RCV_FILE
	};

	typedef struct _ListenSocket
	{
		UDTSOCKET sockListen;
		UDTSOCKET sockAccept;
		char strServerPort[32];
		char strServerAddr[32];
		char strClientPort[32];
		char strClientAddr[32];
		char ownDev[128];
		char ownType[128];
		char recvDev[128];
		char recvType[128];
		char sendType[128];
		int64_t nFileTotalSize;
		int64_t nRecvSize;
		int nCtrlFileGroup;
		int nFileCount;
		double iProgress;
		std::string fileName;
		std::string fileSavePath;
		std::vector<std::string> vecFiles;
		std::vector<std::string> vecFilePath;
		CUdtCore * pThis;
		OP_TYPE Type;
		pthread_t hHandle;
		pthread_cond_t cond;
		pthread_mutex_t lock;
	}LISTENSOCKET, *PLISTENSOCKET;

	void SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName);
	void CreateDirectroy(const std::string & szPath);
	void ProcessAccept(LISTENSOCKET * pListen);
	int ProcessSendCtrl(LISTENSOCKET * cxt);
	int ProcessSendFile(LISTENSOCKET * cxt);
	int ProcessRecvFile(LISTENSOCKET * cxt);
	int InitListenSocket(const char* pstrPort, UDTSOCKET & sockListen);
	int CreateTCPSocket(SYSSOCKET & ssock, const char* pstrPort, bool rendezvous = false);
	int CreateUDTSocket(UDTSOCKET & usock, const char* pstrPort, bool rendezvous = false);
	int TCP_Connect(SYSSOCKET& ssock, const char* pstrAddr, const char* pstrPort);
	int UDT_Connect(UDTSOCKET & usock, const char* pstrAddr, const char* pstrPort);

private:
	CUDTCallBack * m_pCallBack;
	std::string m_szReplyfilepath;
	std::vector<PLISTENSOCKET> VEC_LISTEN;

	int m_nCtrlPort;
	int m_nFilePort;
	bool m_bListenStatus;

	pthread_mutex_t m_Lock;
	pthread_mutex_t m_LockLis;

	pthread_cond_t m_CondLisCtrl;
	pthread_cond_t m_CondLisFile;

	pthread_t m_hThrLisCtrl;
	pthread_t m_hThrLisFile;
#ifndef WIN32
	static void * _ListenThreadProc(void * pParam);
	static void * _WorkThreadProc(void * pParam);
#else
	static DWORD WINAPI _ListenThreadProc(LPVOID pParam);
	static DWORD WINAPI _WorkThreadProc(LPVOID pParam);
#endif
};

#endif	// __UDTCORE_H__
