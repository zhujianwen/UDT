#ifndef __TURBO_TRANSFER_H__
#define __TURBO_TRANSFER_H__

#ifndef WIN32
#include <sys/time.h>
#include <sys/uio.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#else
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#endif

#include <sys/stat.h>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <list>

#include "udt.h"
#include "lock.h"
#include "threads.h"
#include "threadtask.h"


//////////////////////////////////////////////////////////////////////////
// data struct
enum OP_TYPE
{
	OP_NULL = 0,
	OP_SND_CTRL,
	OP_SND_FILE,
	OP_RCV_CTRL,
	OP_RCV_FILE,
	OP_SRV_LISTEN
};

typedef struct ClientContext
{
	UDTSOCKET sock;
	std::string host;
	std::string port;
	std::string filepath;
	int64_t startoffset;
	int64_t total_filezie;
	OP_TYPE type;
}CLIENTCONTEXT, *PCLIENTCONTEXT;


//////////////////////////////////////////////////////////////////////////
// TurboListener class
class TurboListener
{
public:
	virtual ~TurboListener() {}

	virtual void OnAccept(const char* filename) = 0;
	virtual void OnTransfer(const char* filename, long totalsize, long cursize, int type) = 0;
	virtual void OnFinished(const char* message, int result) = 0;
};


//////////////////////////////////////////////////////////////////////////
// TurboTransfer class
class TurboTransfer
{
public:
	friend class TurboTransferServerTask;
	friend class TurboTransferClientTask;

	TurboTransfer(TurboListener* pListener);
	~TurboTransfer();

	void StartServer(const char* host, const char* port);
	void StopServer();

	// type: 1-upload, 2-download, 3-recvfile
	void FileTransfer(const char* host, int port, const char* filepath, int fileoffset, int type);
	void PauseTransfer(const char* filepath, int fileoffset, int type);
	void ReplyAccept(const UDTSOCKET sock, const char* reply);

private:
	int ServerListener(UDTSOCKET& sock, const char* host, const char* port);
	int ClientConnect(UDTSOCKET& sock, const char* host, const char* port);

	int ProcessSendFile(CLIENTCONTEXT* pClip);
	int ProcessRecvFile(CLIENTCONTEXT* pClip);

	void Worker(CLIENTCONTEXT* clip = NULL);

private:
	std::list<CLIENTCONTEXT *>	m_Clip;
	CThreadManager* m_TaskManager;
	TurboListener* m_pListener;
	Mutex m_Lock;
	bool m_bListening;
	bool m_bTransfering;
	UDTSOCKET m_sockListen;
	const int m_nPktSize;
	const int SND_BLOCK;
	std::string m_Port;
	std::string m_FileSavePath;
};


//////////////////////////////////////////////////////////////////////////
// TurboTransferTask class
class TurboTransferServerTask : public CThreadTask
{
public:
	TurboTransferServerTask(TurboTransfer* turbo, TimeStamp delay = TimeStamp(5.0), CLIENTCONTEXT* clip = NULL);

protected:
	~TurboTransferServerTask();
	void DoRun();

protected:
	TurboTransfer* m_turbo;
	TimeStamp m_Delay;
	CLIENTCONTEXT* m_Clip;
};


//////////////////////////////////////////////////////////////////////////
// TaskManager class
class TurboTransferClientTask : public CThreadTask
{
public:
	TurboTransferClientTask(TurboTransfer* trubo, TimeStamp delay = TimeStamp(5.0), CLIENTCONTEXT* clip = NULL);

protected:
	~TurboTransferClientTask();
	void DoRun();

protected:
	TurboTransfer* m_turbo;
	TimeStamp m_Delay;
	CLIENTCONTEXT* m_Clip;
};

#endif	// __TURBO_TRANSFER_H__