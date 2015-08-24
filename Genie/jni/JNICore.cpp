#include "JNICore.h"
#include "JNIMain.h"


JNICore::JNICore()
	: m_delegateObj(NULL)
{
	m_core = new CUdtCore(this);
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


CUdtCore *JNICore::core() const
{
	return m_core;
}

bool JNICore::init(JNIEnv *env, jobject delegateObj)
{
	m_delegateObj = env->NewGlobalRef(delegateObj);
	return true;
}

void JNICore::onAccept(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsAddr = env->NewStringUTF(pstrIpAddr);
		jstring jsHost = env->NewStringUTF(pstrHostName);
		jstring jsType = env->NewStringUTF(pstrSendType);
		jstring jsFile = env->NewStringUTF(pstrFileName);
		env->CallVoidMethod(m_delegateObj, CG::m_OnAccept, jsAddr, jsHost, jsType, jsFile, nFileCount);
	}
}

void JNICore::onAcceptFolder(const char* pstrIpAddr, const char* pstrHostName, const char* pstrSendType, const char* pstrFileName, int nFileCount)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsAddr = env->NewStringUTF(pstrIpAddr);
		jstring jsHost = env->NewStringUTF(pstrHostName);
		jstring jsType = env->NewStringUTF(pstrSendType);
		jstring jsFile = env->NewStringUTF(pstrFileName);
		env->CallVoidMethod(m_delegateObj, CG::m_OnAcceptFolder, jsAddr, jsHost, jsType, jsFile, nFileCount);
	}
}

void JNICore::onAcceptonFinish(const char* host, std::vector<std::string> vecFileName)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		int nCount = vecFileName.size();
		jstring jsTmp;
		jstring jsHost = env->NewStringUTF(host);
		jobjectArray arry = env->NewObjectArray(nCount, CG::c_String, 0);
		for (int i = 0; i < nCount; i++)
		{
			jsTmp = env->NewStringUTF(vecFileName[i].c_str());
			env->SetObjectArrayElement(arry, i, jsTmp);
			env->DeleteLocalRef(jsTmp);
		}
		env->CallVoidMethod(m_delegateObj, CG::m_OnAcceptonFinish, jsHost, arry);
	}
}

void JNICore::onSendFinished(const char * pstrMsg)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(pstrMsg);
		env->CallVoidMethod(m_delegateObj, CG::m_OnFinished, jsText);
	}
}

void JNICore::onRecvFinished(const char * pstrMsg)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(pstrMsg);
		env->CallVoidMethod(m_delegateObj, CG::m_OnFinished, jsText);
	}
}

void JNICore::onSendTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(pstrFileName);
		env->CallVoidMethod(m_delegateObj, CG::m_OnTransfer, nFileTotalSize, nCurrent, jsText);
	}
}

void JNICore::onRecvTransfer(const int64_t nFileTotalSize, const int64_t nCurrent, const char* pstrFileName)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(pstrFileName);
		env->CallVoidMethod(m_delegateObj, CG::m_OnTransfer, nFileTotalSize, nCurrent, jsText);
	}
}

void JNICore::onRecvMessage(const char* pstrIpAddr, const char* pstrHostName, const char* pstrMsg)
{
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsAddr = env->NewStringUTF(pstrIpAddr);
		jstring jsHost = env->NewStringUTF(pstrHostName);
		jstring jsText = env->NewStringUTF(pstrMsg);
		env->CallVoidMethod(m_delegateObj, CG::m_OnRecvMessage, jsAddr, jsHost, jsText);
	}
}



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
