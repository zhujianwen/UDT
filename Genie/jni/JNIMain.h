#ifndef __JNIMain_h__
#define __JNIMain_h__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

JNIEXPORT jint  JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved);
JNIEXPORT void  JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved);

JNIEXPORT jlong JNICALL Java_com_dragonflow_FileTransfer_init(JNIEnv *env, jclass clazz, jobject delegateObj);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_destroy(JNIEnv *env, jclass clazz, jint ptr);
JNIEXPORT jint  JNICALL Java_com_dragonflow_FileTransfer_listenFileSend(JNIEnv* env,  jclass clazz, jlong ptr, jint port);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendMessage(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port,jstring message, jstring hostname, jstring sendtype);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendFile(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port, jstring filename, jstring hostname, jstring sendtype);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendMultiFiles(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port, jobjectArray filelist, jstring hostname, jstring sendtype);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_sendfolder(JNIEnv* env, jclass clazz, jlong ptr, jstring host, jint port, jstring folderName, jstring hostname, jstring sendtype);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_replyAccept(JNIEnv *env, jclass clazz, jlong ptr, jstring szReply);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_stopTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint nType);
JNIEXPORT void  JNICALL Java_com_dragonflow_FileTransfer_stopListen(JNIEnv *env, jclass clazz, jlong ptr);

#ifdef __cplusplus
}
#endif // __cplusplus


class CG
{
public:
	static bool init(JavaVM *jvm);
	static void term();

	static JavaVM *javaVM;
	static jclass c_String;
	static jclass c_FileTransfer;
	
	static jmethodID m_OnDebug;
	static jmethodID m_OnAccept;
	static jmethodID m_OnAcceptFolder;
	static jmethodID m_OnAcceptonFinish;
	static jmethodID m_OnTransfer;
	static jmethodID m_OnFinished;
	static jmethodID m_OnRecvMessage;
};

#endif // __JNIMain_h__
