#ifndef __JNIAPI_H__
#define __JNIAPI_H__

#include <jni.h>


class CG
{
public:
	static bool startup(JavaVM *jvm);
	static void cleanup();

	static JavaVM *javaVM;
	static jclass c_String;
	static jclass c_FileTransfer;
	
	static jmethodID m_OnInit;
	static jmethodID m_OnRequest;
	static jmethodID m_OnConnect;
	static jmethodID m_OnReceive;
	static jmethodID m_OnClose;

	static JNIEnv *m_env;
	static jobject m_delegateObj;
	static bool m_bJavaVM;
	static bool m_bEnv;
	static bool m_bInit;

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


#endif // __JNIAPI_H__
