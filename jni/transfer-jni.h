#ifndef __TRANSFER_JNI_H__
#define __TRANSFER_JNI_H__


#include <jni.h>
#include <stdlib.h>
#include <vector>

#include "../app/transfer.h"


#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

JNIEXPORT jint  JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved);
JNIEXPORT void  JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved);


JNIEXPORT jlong JNICALL Java_com_zhd_core_helper_FileTransferHelper_init(JNIEnv *env, jclass clazz, jobject delegateObj);
JNIEXPORT void JNICALL Java_com_zhd_core_helper_FileTransferHelper_destroy(JNIEnv *env, jclass clazz, jlong ptr);
JNIEXPORT void JNICALL Java_com_zhd_core_helper_FileTransferHelper_setDebug(JNIEnv *env, jclass clazz, jboolean flags);
JNIEXPORT jint JNICALL Java_com_zhd_core_helper_FileTransferHelper_stopTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint nType, jstring filepath);
JNIEXPORT jint JNICALL Java_com_zhd_core_helper_FileTransferHelper_pauseTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint nType, jstring filepath, jint nCurrent);
JNIEXPORT jint JNICALL Java_com_zhd_core_helper_FileTransferHelper_startTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint type, jstring host, jint port, jstring filepath, jint offset);

#ifdef __cplusplus
}
#endif // __cplusplus


//////////////////////////////////////////////////////////////////////////
// CG class
class CG
{
public:
	static bool init(JavaVM *jvm);
	static void term();

	static JavaVM *javaVM;
	static jclass c_String;
	static jclass c_FileTransfer;
	
	static jmethodID m_CtorID;
	static jmethodID m_OnAccept;
	static jmethodID m_OnFinished;
	static jmethodID m_OnTransfer;
};


//////////////////////////////////////////////////////////////////////////
// VMGuard class
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


//////////////////////////////////////////////////////////////////////////
// JNICore class
class JNICore : public TurboListener
{
public:
	JNICore();
	~JNICore();

	bool init(JNIEnv *env, jobject delegateObj);
	
	TurboTransfer *core() const;

protected:
	// call back
	virtual void OnAccept(const char* filename);
	virtual void OnTransfer(const char* filename, long totalsize, long cursize, int type);
	virtual void OnFinished(const char* message, int result);

private:
	TurboTransfer *m_core;
	jobject m_delegateObj;
};

#endif	// __TRANSFER_JNI_H__
