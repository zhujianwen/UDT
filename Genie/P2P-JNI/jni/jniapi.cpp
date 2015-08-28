#include "jniapi.h"
#include "udt.h"
#include "api.h"
#include "core.h"

#include <stdlib.h>
#include <vector>

#include <android/log.h>
#include <com_BigIT_p2p_p2p.h>


//////////////////////////////////////////////////////////////////////////
// p2p callback
// init callback
void p2p_init_handler(int err, struct p2phandle* p2p)
{
	__android_log_print(ANDROID_LOG_INFO, "p2p", "p2p_init handle ok!");
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF("init callback");
		env->CallVoidMethod(CG::m_delegateObj, CG::m_OnInit, jsText);
	}
}

void p2p_connect_handler(int err, const char* reason, struct p2pconnection* p2pcon)
{
	re_printf("connect ok");
	int fdlisten = tcp_conn_fd(p2pcon->p2p->ltcp);
	UDPSOCKET udpsock = (UDPSOCKET)fdlisten;
	UDT::connect(p2pcon->udtsock, &udpsock, &(p2pcon->p2p->sactl->u.sa), p2pcon->p2p->sactl->len);
}

// receive callback
void p2p_receive_handler(const char* data, uint32_t len, struct p2pconnection* p2pcon)
{
	__android_log_print(ANDROID_LOG_INFO, "p2p", "p2p_init handle ok!");
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jbyteArray byteArray = env->NewByteArray((int)len);
		jstring name = env->NewStringUTF(p2pcon->peername);
		env->SetByteArrayRegion(byteArray, 0, (jsize)len, (jbyte*)data);
		env->CallVoidMethod(CG::m_delegateObj, CG::m_OnReceive, name, byteArray, (jint)len);
	}

	int fd = udp_sock_fd(p2pcon->ulsock, AF_INET);
	//CUDT::postRecv(p2pcon->p2p->usock, data, len);
	re_printf("p2p receive data:%s\n", data);
}

// connect callback
void p2p_request_handler(struct p2phandle *p2p, const char* peername, struct p2pconnection **p2pcon)
{
	__android_log_print(ANDROID_LOG_INFO, "JNILog","p2p_request:%s", peername);
	VMGuard vmguard;
	if (JNIEnv *env = vmguard.env())
	{
		jstring jsText = env->NewStringUTF(peername);
		env->CallVoidMethod(CG::m_delegateObj, CG::m_OnRequest, jsText);
	}

	int err;
	err = p2p_accept(p2p, p2pcon, peername, p2p_receive_handler, *p2pcon);
	int fdaccept = udp_sock_fd((*p2pcon)->ulsock, AF_INET);
	struct sa* lsa = NULL;
	lsa = (struct sa*)mem_zalloc(sizeof(struct sa),NULL);
	sa_cpy(lsa, p2p->sactl);
	int fdlisten = tcp_conn_fd(p2p->ltcp);
	UDPSOCKET udpsock = (UDPSOCKET)fdlisten;
	(*p2pcon)->udtsock = CUDT::socket(AF_INET, SOCK_STREAM, 0);
	CUDT::bind((*p2pcon)->udtsock, &udpsock, &(p2p->sactl->u.sa), p2p->sactl->len);
	CUDT::listen((*p2pcon)->udtsock, 10);
	if (err)
	{
		re_fprintf(stderr, "p2p accept error:%s\n", strerror(err));
		return ;
	}
}



//////////////////////////////////////////////////////////////////////////
// JNI API
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved)
{
	if (!CG::startup(jvm)) {
		return JNI_ERR;
	}

	return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved)
{
	CG::cleanup();
}


JNIEXPORT jint JNICALL Java_com_BigIT_p2p_p2p_p2pinit
  (JNIEnv *env, jclass clazz, jobject delegateObj, jstring ctlserv, jint ctlport, jstring stunserv, jint stunport, jstring name, jstring passwd)
{
	if (!CG::m_bJavaVM)
	{
		__android_log_print(ANDROID_LOG_INFO, "p2p", "GetEnv failed!");
		return -1;
	}

	CG::m_delegateObj = env->NewGlobalRef(delegateObj);

	int err;
	//struct threaddata *td;
	const char *strctlserv = env->GetStringUTFChars(ctlserv, 0);
	const char *strstunserv = env->GetStringUTFChars(stunserv, 0);
	const char *strname = env->GetStringUTFChars(name, 0);
	const char *strpasswd = env->GetStringUTFChars(passwd, 0);

	err = UDT::p2pInit(strctlserv, ctlport, strstunserv, stunport, strname, strpasswd);
	if (err)
	{
		//DEBUG_LOG("p2p_init err: %s !", strerror(err));
		__android_log_print(ANDROID_LOG_INFO, "p2p", "p2p_init failed!");
		return -1;
	}

	env->ReleaseStringUTFChars(ctlserv, strctlserv);
	env->ReleaseStringUTFChars(stunserv, strstunserv);
	env->ReleaseStringUTFChars(name, strname);
	env->ReleaseStringUTFChars(passwd, strpasswd);
	//DEBUG_LOG("p2p_init complete!");
	__android_log_print(ANDROID_LOG_INFO, "p2p", "p2p_init ok!");

	return err;
}

JNIEXPORT jint JNICALL Java_com_BigIT_p2p_p2p_p2pconnect
  (JNIEnv *env, jclass clazz, jstring peername)
{
	const char *strpeername = env->GetStringUTFChars(peername, 0);
	UDTSOCKET psock = UDT::p2pConnect(strpeername);
	env->ReleaseStringUTFChars(peername, strpeername);
	return psock;
}

JNIEXPORT jint JNICALL Java_com_BigIT_p2p_p2p_p2paccept
  (JNIEnv *, jclass, jstring)
{
	return 0;
}

JNIEXPORT jint JNICALL Java_com_BigIT_p2p_p2p_p2psend
  (JNIEnv *, jclass, jstring, jbyteArray, jint)
{
	return 0;
}

JNIEXPORT jint JNICALL Java_com_BigIT_p2p_p2p_p2pdisconnect
  (JNIEnv *, jclass, jstring)
{
	return 0;
}

JNIEXPORT jint JNICALL Java_com_BigIT_p2p_p2p_p2pclose
  (JNIEnv *, jclass)
{
	return 0;
}


/*
 *Call back funtion
 */
JavaVM *CG::javaVM = 0;

JNIEnv* CG::m_env = NULL;
jobject CG::m_delegateObj = NULL;
bool CG::m_bJavaVM = false;
bool CG::m_bEnv = false;
bool CG::m_bInit = false;

jclass CG::c_String = 0;
jclass CG::c_FileTransfer = 0;

jmethodID CG::m_OnInit = 0;
jmethodID CG::m_OnRequest = 0;
jmethodID CG::m_OnConnect = 0;
jmethodID CG::m_OnReceive = 0;
jmethodID CG::m_OnClose = 0;


#define CG_CACHE_CLASS(v, n) { jclass cc = env->FindClass(n); if (!cc) { return false; } v = static_cast<jclass>(env->NewGlobalRef(cc)); }
#define CG_CACHE_METHOD(c, m, n, s) { m = env->GetMethodID(c, n, s); if (!m) { return false; } }
#define CG_CACHE_FIELD(c, f, n, s) { f = env->GetFieldID(c, n, s); if (!f) { return false; } }

bool CG::startup(JavaVM *jvm)
{
	JNIEnv *env;
	if (JNI_OK != jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_2))
	{
		return false;
	}

	javaVM = jvm;
	m_env = env;
	m_bJavaVM = true;
	m_bEnv = true;
	
	CG_CACHE_CLASS(c_String, "java/lang/String");
	CG_CACHE_CLASS(c_FileTransfer, "com/BigIT/p2p/p2p");

	CG_CACHE_METHOD(c_FileTransfer, m_OnInit, "on_init", "(I)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnRequest, "on_request", "(Ljava/lang/String;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnConnect, "on_connect", "(ILjava/lang/String;)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnReceive, "on_receive", "(Ljava/lang/String;[BI)V");
	CG_CACHE_METHOD(c_FileTransfer, m_OnClose, "on_close", "(Ljava/lang/String;)V");
	
	return true;
}

void CG::cleanup()
{
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
