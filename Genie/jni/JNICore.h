#ifndef __JNICore_h__
#define __JNICore_h__

#include <jni.h>
#include <stdlib.h>
#include <vector>

#include "UdtCore.h"

class JNICore : public CUDTCallBack
{
public:
	JNICore();
	~JNICore();

	bool init(JNIEnv *env, jobject delegateObj);
	
	CUdtCore *core() const;

protected:
	// call back
	virtual void onAccept(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount);
	virtual void onAcceptFolder(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount);
	virtual void onAcceptonFinish(const char* host, std::vector<std::string> vecFileName);
	virtual void onSendFinished(const char * pstrMsg);
	virtual void onRecvFinished(const char * pstrMsg);
	virtual void onSendTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName);
	virtual void onRecvTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName);
	virtual void onRecvMessage(const char* pstrIpAddr, const char* pstrHostName, const char* pstrMsg);

private:
	CUdtCore *m_core;
	jobject m_delegateObj;
};

class VMGuard
{
public:
	VMGuard();
	~VMGuard();
	
	bool isValid() const;
	JNIEnv *env() const;
	
private:
	JNIEnv *m_env;
	bool m_attached;
	bool m_valid;
};

#endif // __JNICore_h__
