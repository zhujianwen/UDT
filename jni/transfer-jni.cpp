#include "transfer-jni.h"

#include <android/log.h>


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


JNIEXPORT jlong JNICALL Java_com_zhd_core_helper_FileTransferHelper_init(JNIEnv *env, jclass clazz, jobject delegateObj)
{
	__android_log_print(ANDROID_LOG_INFO, "TURBO", "init success1!");
	JNICore *core = new JNICore();
	if (!core->init(env, delegateObj))
	{
		delete core;
		return 0;
	}

	return reinterpret_cast<jlong>(core);
}

JNIEXPORT void JNICALL Java_com_zhd_core_helper_FileTransferHelper_destroy(JNIEnv *env, jclass clazz, jlong ptr)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	delete core;
}

JNIEXPORT void JNICALL Java_com_zhd_core_helper_FileTransferHelper_setDebug(JNIEnv *env, jclass clazz, jboolean flags)
{
	
}

JNIEXPORT jint JNICALL Java_com_zhd_core_helper_FileTransferHelper_stopTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint nType, jstring filepath)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pFilePath  = env->GetStringUTFChars(filepath, &isCopy);
	env->ReleaseStringUTFChars(filepath, pFilePath);
	return 0;
}

JNIEXPORT jint JNICALL Java_com_zhd_core_helper_FileTransferHelper_pauseTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint nType, jstring filepath, jint nCurrent)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pFilePath  = env->GetStringUTFChars(filepath, &isCopy);
	core->core()->PauseTransfer(pFilePath, nCurrent, nType);
	env->ReleaseStringUTFChars(filepath, pFilePath);
	return 0;
}

JNIEXPORT jint JNICALL Java_com_zhd_core_helper_FileTransferHelper_startTransfer(JNIEnv *env, jclass clazz, jlong ptr, jint type, jstring host, jint port, jstring filepath, jint offset)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	int nRet;
	jboolean isCopy;
	const char *pAddress = env->GetStringUTFChars(host, &isCopy);
	const char *pFilePath  = env->GetStringUTFChars(filepath, &isCopy);

	core->core()->FileTransfer(pAddress, port, pFilePath, offset, type);
	
	env->ReleaseStringUTFChars(host, pAddress);
	env->ReleaseStringUTFChars(filepath, pFilePath);
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_zhd_helper_FileTransfer_replyAccept(JNIEnv *env, jclass clazz, jlong ptr, jstring szReply, jint sock)
{
	JNICore *core = reinterpret_cast<JNICore*>(ptr);
	jboolean isCopy;
	const char *pstrTmp = env->GetStringUTFChars(szReply, &isCopy);
	core->core()->ReplyAccept(sock, pstrTmp);
	env->ReleaseStringUTFChars(szReply, pstrTmp);
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// CG class
// Call back fucntion
JavaVM *CG::javaVM = 0;

jclass CG::c_String = 0;
jclass CG::c_FileTransfer = 0;

jmethodID CG::m_CtorID = 0;
jmethodID CG::m_OnAccept = 0;
jmethodID CG::m_OnTransfer = 0;
jmethodID CG::m_OnFinished = 0;


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
	CG_CACHE_CLASS(c_FileTransfer, "com/zhd/core/helper/FileTransferHelper");

	CG_CACHE_METHOD(c_String, m_CtorID, "<init>", "([BLjava/lang/String;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnAccept, "onAccept", "(Ljava/lang/String;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnTransfer, "onTransfer", "(Ljava/lang/String;JJI)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnFinished, "onFinish", "(Ljava/lang/String;I)V");
	
	return true;
}

void CG::term()
{
}


//////////////////////////////////////////////////////////////////////////
// VMGuard class
VMGuard::VMGuard()
: m_env(NULL), m_attached(true), m_valid(false)
{
	JNIEnv *env = NULL;
	jint r = CG::javaVM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_2);
	if (r == JNI_OK) {
		m_attached = false;
	} else if (r == JNI_EDETACHED) {
		r = CG::javaVM->AttachCurrentThread(&env, NULL);
	}

	if (r == JNI_OK) {
		m_env = env;
		m_valid = true;
	}
}

VMGuard::~VMGuard()
{
	if (m_valid && m_attached) {
		CG::javaVM->DetachCurrentThread();
	}
}

bool VMGuard::isValid() const
{
	return m_valid;
}

JNIEnv *VMGuard::env() const
{
	return m_env;
}


//////////////////////////////////////////////////////////////////////////
// JNICore class
JNICore::JNICore()
	: m_delegateObj(NULL)
{
	m_core = new TurboTransfer(this);
}

JNICore::~JNICore()
{
	delete m_core;
	if (m_delegateObj)
	{
		VMGuard vmguard;
		if (JNIEnv *env = vmguard.env())
		{
			env->DeleteGlobalRef(m_delegateObj);
		}	
	}
}


TurboTransfer *JNICore::core() const
{
	return m_core;
}

bool JNICore::init(JNIEnv *env, jobject delegateObj)
{
	m_delegateObj = env->NewGlobalRef(delegateObj);

	return true;
}

void JNICore::OnAccept(const char* filename)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsAddr = env->NewStringUTF(filename);
		env->CallVoidMethod(m_delegateObj, CG::m_OnAccept, jsAddr);
	}
}

void JNICore::OnTransfer(const char* filename, long totalsize, long cursize, int type)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(filename);
		env->CallVoidMethod(m_delegateObj, CG::m_OnTransfer, jsText, totalsize, cursize, type);
	}
}

void JNICore::OnFinished(const char* message, int result)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(message);
		env->CallVoidMethod(m_delegateObj, CG::m_OnFinished, jsText, result);
	}
}