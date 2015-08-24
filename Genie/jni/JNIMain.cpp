#include "JNIMain.h"
#include "JNICore.h"

#include <stdlib.h>
#include <vector>


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved)
{
	if (!CG::init(jvm)) {
		return JNI_ERR;
	}

	return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
	CG::term();
}


JNIEXPORT jlong JNICALL Java_com_dragonflow_FileTransfer_init(JNIEnv *env, jclass clazz, jobject delegateObj)
{
	JNICore *core = new JNICore();
	if (!core->init(env, delegateObj))
	{
		delete core;
		return 0;
	}

	return reinterpret_cast<jlong>(core);
}

JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_destroy(JNIEnv *env, jclass clazz, jint ptr)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	delete core;
}

JNIEXPORT jint JNICALL Java_com_dragonflow_FileTransfer_listenFileSend(JNIEnv* env,  jclass clazz, jlong ptr, jint port)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	return core->core()->listenFileSend(port);
}

JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_stopTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint nType)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	core->core()->stopTransfer(nType);
}

JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_stopListen(JNIEnv *env, jclass clazz, jlong ptr)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	core->core()->stopListen();
}


JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_replyAccept(JNIEnv *env, jclass clazz, jlong ptr, jstring szReply)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pstrTmp = env->GetStringUTFChars(szReply, &isCopy);
	core->core()->replyAccept(pstrTmp);
	env->ReleaseStringUTFChars(szReply, pstrTmp);
}

JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendMessage(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port,jstring message, jstring hostname, jstring sendtype)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pAddr = env->GetStringUTFChars(host, &isCopy);
	const char *pMsg = env->GetStringUTFChars(message, &isCopy);
	const char *pHost = env->GetStringUTFChars(hostname, &isCopy);
	const char *pType = env->GetStringUTFChars(sendtype, &isCopy);
	std::vector<std::string> vecArray;
	vecArray.push_back(pMsg);
	core->core()->SendEx(pAddr, port, pHost, pType, vecArray, 1);
	env->ReleaseStringUTFChars(host, pAddr);
	env->ReleaseStringUTFChars(message, pMsg);
	env->ReleaseStringUTFChars(hostname, pHost);
	env->ReleaseStringUTFChars(sendtype, pType);
}

JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendFile(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port, jstring filename, jstring hostname, jstring sendtype)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pAddr = env->GetStringUTFChars(host, &isCopy);
	const char *pHost = env->GetStringUTFChars(hostname, &isCopy);
	const char *pType = env->GetStringUTFChars(sendtype, &isCopy);
	const char *pText = env->GetStringUTFChars(filename, &isCopy);
	std::vector<std::string> vecArray;
	vecArray.push_back(pText);
	core->core()->SendEx(pAddr, port, pHost, pType, vecArray, 2);
	env->ReleaseStringUTFChars(host, pAddr);
	env->ReleaseStringUTFChars(hostname, pHost);
	env->ReleaseStringUTFChars(sendtype, pType);
	env->ReleaseStringUTFChars(filename, pText);
}

JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendMultiFiles(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port, jobjectArray filelist, jstring hostname, jstring sendtype)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pAddr = env->GetStringUTFChars(host, &isCopy);
	const char *pHost = env->GetStringUTFChars(hostname, &isCopy);
	const char *pType = env->GetStringUTFChars(sendtype, &isCopy);

	const char *pText;
	std::vector<std::string> vecArray;
	for (jsize i = 0; i < env->GetArrayLength(filelist); i++)
	{
		jstring jsText = static_cast<jstring>(env->GetObjectArrayElement(filelist, i));
		pText = env->GetStringUTFChars(jsText, &isCopy);
		vecArray.push_back(pText);
		env->ReleaseStringUTFChars(jsText, pText);
		env->DeleteLocalRef(jsText);
	}
	core->core()->SendEx(pAddr, port, pHost, pType, vecArray, 3);
	env->ReleaseStringUTFChars(host, pAddr);
	env->ReleaseStringUTFChars(hostname, pHost);
	env->ReleaseStringUTFChars(sendtype, pType);
}

JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendfolder(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port, jstring folderName, jstring hostname, jstring sendtype)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pAddr = env->GetStringUTFChars(host, &isCopy);
	const char *pHost = env->GetStringUTFChars(hostname, &isCopy);
	const char *pType = env->GetStringUTFChars(sendtype, &isCopy);
	const char *pText = env->GetStringUTFChars(folderName, &isCopy);
	std::vector<std::string> vecArray;
	vecArray.push_back(pText);
	core->core()->SendEx(pAddr, port, pHost, pType, vecArray, 4);
	env->ReleaseStringUTFChars(host, pAddr);
	env->ReleaseStringUTFChars(hostname, pHost);
	env->ReleaseStringUTFChars(sendtype, pType);
	env->ReleaseStringUTFChars(folderName, pText);
}



/*
 *Call back funtion
 */
JavaVM *CG::javaVM = 0;

jclass CG::c_String = 0;

jclass CG::c_FileTransfer = 0;
jmethodID CG::m_OnDebug = 0;
jmethodID CG::m_OnAccept = 0;
jmethodID CG::m_OnAcceptFolder = 0;
jmethodID CG::m_OnAcceptonFinish = 0;
jmethodID CG::m_OnTransfer = 0;
jmethodID CG::m_OnFinished = 0;
jmethodID CG::m_OnRecvMessage = 0;


#define CG_CACHE_CLASS(v, n) { jclass cc = env->FindClass(n); if (!cc) { return false; } v = static_cast<jclass>(env->NewGlobalRef(cc)); }
#define CG_CACHE_METHOD(c, m, n, s) { m = env->GetMethodID(c, n, s); if (!m) { return false; } }
#define CG_CACHE_FIELD(c, f, n, s) { f = env->GetFieldID(c, n, s); if (!f) { return false; } }

bool CG::init(JavaVM *jvm)
{
	JNIEnv *env;
	if (JNI_OK != jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_2))
	{
		return false;
	}

	javaVM = jvm;
	
	CG_CACHE_CLASS(c_String, "java/lang/String");
	CG_CACHE_CLASS(c_FileTransfer, "com/dragonflow/FileTransfer");

	CG_CACHE_METHOD(c_FileTransfer, m_OnDebug, "onDebug", "(Ljava/lang/String;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnAccept, "onAccept", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnAcceptFolder, "onAcceptFolder", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnAcceptonFinish, "onAcceptonFinish", "(Ljava/lang/String;[Ljava/lang/Object;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnTransfer, "onTransfer", "(JJLjava/lang/String;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnFinished, "onFinished", "(Ljava/lang/String;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnRecvMessage, "onRecvMessage", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	
	return true;
}

void CG::term()
{
}
