#include <string.h>#include <jni.h>#include <sys/stat.h>#include <arpa/inet.h>#include <netdb.h>#include <fstream>#include <iostream>#include <cstdlib>#include <cstring>#include <vector>#include <udt.h>#include <errno.h>#include <unistd.h>#include <dirent.h>#include <time.h>using namespace std;pthread_t _hAcceptThread;pthread_t _hSendThread;pthread_t _hRecvThread;#ifndef WIN32void * _AcceptThreadProc(void * pParam);void * _RecvThreadProc(void * pParam);void * _SendThreadProc(void * pParam);#elseDWORD WINAPI _AcceptThreadProc(LPVOID pParam);DWORD WINAPI _RecvThreadProc(LPVOID pParam);DWORD WINAPI _SendThreadProc(LPVOID pParam);#endif#ifndef WIN32static pthread_mutex_t Accept_Lock;static pthread_mutex_t Send_Lock;static pthread_cond_t Accpet_Cond;static pthread_cond_t Send_Cond;#elsestatic HANDLE hAcceptThreadExit = NULL;static HANDLE hSendThreadExit = NULL;static HANDLE hRecvThreadExit = NULL;#endifpthread_mutex_t mut_recv = PTHREAD_MUTEX_INITIALIZER;pthread_mutex_t mut_send = PTHREAD_MUTEX_INITIALIZER;UDTSOCKET sockSend = -1;UDTSOCKET sockRecv = -1;string g_strReplyfilepath = "";struct AppContext{	UDTSOCKET sock;	JNIEnv* env;	jobject thiz;	JavaVM *jvm;	char clienthost[NI_MAXHOST];};int setReplyAccept(JNIEnv* env,  jobject thiz, char * strReply){	pthread_mutex_lock(&mut_recv);	g_strReplyfilepath = strReply;	pthread_mutex_unlock(&mut_recv);	return 0;}void ReSetReplyAccept(){	pthread_mutex_lock(&mut_recv);	g_strReplyfilepath = "";	pthread_mutex_unlock(&mut_recv);}const char * getReplyAccept(){	pthread_mutex_lock(&mut_recv);	const char * str = g_strReplyfilepath.c_str();	pthread_mutex_unlock(&mut_recv);	return str;}int stopTransfer(JNIEnv* env,  jobject thiz, int type){	// 1：停止接收  2：停止发送	if (type == 2 && sockSend > 0)	{#ifndef WIN32
	pthread_cond_signal(&Send_Cond);
	pthread_join(_hSendThread, NULL);
	pthread_mutex_destroy(&Send_Lock);
	pthread_cond_destroy(&Send_Cond);
#else
	SetEvent(Send_Cond);
	WaitForSingleObject(_hSendThread, INFINITE);
	CloseHandle(_hSendThread);
	CloseHandle(Send_Lock);
	CloseHandle(Send_Cond);
#endif	}	if (type == 1 && sockRecv > 0)	{		UDT::close(sockRecv);	}	return 0;}void searchFileInDirectroy(const char * strPath, int64_t & totalSize, vector<string> & verFileNameList){	DIR * pDir;	if ((pDir = opendir(strPath)) == NULL)	{		cout << "Open dir error \n" << endl;		return ;	}	struct stat dirInfo;	struct dirent * pDT;	char strFileName[256];	memset(strFileName, 0, sizeof(strFileName));	while ((pDT = readdir(pDir)))	{		//if (strcmp(pDT->d_name, ".") == 0 || strcmp(pDT->d_name, "..") == 0)		if (pDT->d_name[0] == '.')		{			continue;		}		else		{			sprintf(strFileName, "%s/%s", strPath, pDT->d_name);			struct stat buf;			if (lstat(strFileName, &buf) >= 0 && S_ISDIR(buf.st_mode))			{				searchFileInDirectroy(strFileName, totalSize, verFileNameList);			}			else			{				// open the file				fstream ifs(strFileName, ios::in | ios::binary);				ifs.seekg(0, ios::end);				int64_t size = ifs.tellg();				totalSize += size;				ifs.seekg(0, ios::beg);				verFileNameList.push_back(strFileName);				ifs.close();			}		}		memset(strFileName, 0, 256);	}	closedir(pDir);}int listenFileSend(JNIEnv* env,  jobject thiz, int port){//	fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);//	log << "***listenFileSend***" << endl;	struct addrinfo hints;	struct   addrinfo* res;	memset(&hints, 0, sizeof(struct addrinfo));	hints.ai_flags = AI_PASSIVE;	hints.ai_family = AF_INET;	hints.ai_socktype = SOCK_STREAM;	// use this function to initialize the UDT library	UDT::startup();	string service("9000");	char szPort[64];	memset(szPort, 0, 64);	sprintf(szPort, "%d", port);	service = szPort;	if (0 != getaddrinfo(NULL, service.c_str(), &hints, &res))	{		//cout << "illegal port number or port is busy.\n" << endl;		//log << "illegal port number or port is busy.\n" << endl;		return 0;	}	UDTSOCKET serv  = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);	// Windows UDP issue	// For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold#ifdef WIN32	int mss = 1052;	UDT::setsockopt(serv, 0, UDT_MSS, &mss, sizeof(int));#endif	if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))	{	//	log << "bind: " << UDT::getlasterror().getErrorMessage() << endl;		return 0;	}	freeaddrinfo(res);	UDT::listen(serv, 10);//	log << "bing listen sucess" << endl;	AppContext *context = new AppContext();	context->sock = serv;	context->env = env;	env->GetJavaVM(&(context->jvm));	context->thiz = (context->env)->NewGlobalRef(thiz);#ifndef WIN32	pthread_t filethread;	pthread_create(&filethread, NULL, _AcceptThreadProc, context);// new UDTSOCKET(serv));	pthread_detach(filethread);	//pthread_join(filethread, NULL);#else	hAcceptThreadExit = CreateEvent(NULL,TRUE, FALSE, NULL);	HANDLE hThread = CreateThread(NULL, 0, _AcceptThreadProc, context, 0, NULL);// new UDTSOCKET(serv), 0, NULL);	//::WaitForSingleObject(hThread, INFINITE);#endif//	log << "listen SUCESS" << endl;//	log.close();	return 0;}#ifndef WIN32void * _AcceptThreadProc(void * pParam)#elseDWORD WINAPI _AcceptThreadProc(LPVOID pParam)#endif{	AppContext *context = (AppContext *)pParam;	UDTSOCKET serv = context->sock;	//UDTSOCKET serv = *(UDTSOCKET*)usocket;	//delete (UDTSOCKET*)usocket;	sockaddr_storage clientaddr;	int addrlen = sizeof(clientaddr);	UDTSOCKET fhandle;	while (true)	{#ifndef WIN32		pthread_mutex_lock(&Accept_Lock);		struct timespec ts;		clock_gettime(CLOCK_REALTIME, &ts);		ts.tv_sec += 1;		int rc = pthread_cond_timedwait(&Accpet_Cond, &Accept_Lock, &ts);		pthread_mutex_unlock(&Accept_Lock);		if (rc != ETIMEDOUT)			break;#else		DWORD ret = WaitForSingleObject(hAcceptThreadExit,50);		if (ret != WAIT_TIMEOUT)		{			break;		}#endif		if (UDT::INVALID_SOCK == (fhandle = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen)))		{			cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;			return 0;		}		char clienthost[NI_MAXHOST];		char clientservice[NI_MAXSERV];		getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);		cout << "new connection: " << clienthost << ":" << clientservice << endl;		AppContext *cxt = new AppContext();		cxt->sock = fhandle;		cxt->env = context->env;		cxt->thiz = context->thiz;		cxt->jvm = context->jvm;		memset(cxt->clienthost, 0, NI_MAXHOST);		strcpy(cxt->clienthost, clienthost);#ifndef WIN32		pthread_t filethread;		pthread_create(&filethread, NULL, _RecvThreadProc, cxt);//new UDTSOCKET(fhandle));		pthread_detach(filethread);#else		CreateThread(NULL, 0, _RecvThreadProc, cxt, 0, NULL);//new UDTSOCKET(fhandle), 0, NULL);#endif	}	UDT::close(serv);	// use this function to release the UDT library	UDT::cleanup();	delete context;}enum SENDTYPE{	SND_NULL = 0,	SND_MSG,			// send message	SND_UNIFILE,		// send unifile	SND_MULFILE,		// send multyfile	SND_FOLDER		// send folder};struct ServerContext{	JavaVM * jvm;	JNIEnv * env;
	jobject thiz;	UDTSOCKET sock;	SENDTYPE eType;	int nPort;	char * pstrAddr;	char * pstrHostName;	char * pstrSendtype;	jobjectArray strArray;};int SendEx(JNIEnv* env, jobject thiz, const char* pstrAddr, const int nPort, const char* pstrHostName, const char* pstrSendtype, const jobjectArray strArray, SENDTYPE eType){	ServerContext * context = new ServerContext();

	memset(context, 0, sizeof(ServerContext));
	context->env = env;
	env->GetJavaVM(&(context->jvm));
	context->thiz = (context->env)->NewGlobalRef(thiz);

	context->pstrAddr = new char[strlen(pstrAddr)];
	strcpy(context->pstrAddr, pstrAddr);

	context->pstrHostName = new char[strlen(pstrHostName)];
	strcpy(context->pstrHostName, pstrHostName);

	context->pstrSendtype = new char[strlen(pstrSendtype)];
	strcpy(context->pstrSendtype, pstrSendtype);
	context->strArray = strArray;
	context->nPort = nPort;
	context->eType = eType;

#ifndef WIN32
	pthread_mutex_init(&Send_Lock, NULL);
	pthread_cond_init(&Send_Cond, NULL);
	pthread_create(&_hSendThread, NULL, _SendThreadProc, context);
	pthread_detach(_hSendThread);
#else
	DWORD ThreadID;
	Send_Lock = CreateMutex(NULL, false, NULL);
	Send_Cond = CreateEvent(NULL, false, false, NULL);
	_hSendThread = CreateThread(NULL, 0, _SendThreadProc, context, NULL, &ThreadID);
#endif	return 0;}#ifndef WIN32void * _SendThreadProc(void* pParam)#elseDWORD WINAPI _SendThreadProc(LPVOID pParam)#endif{//	fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);//	log << "***SendThread***" << endl;	ServerContext * cxt = (ServerContext *)pParam;	(cxt->jvm)->AttachCurrentThread(&(cxt->env), NULL);	// 回调方法	jstring jsOutput;	jboolean b = true;	jclass cls = (cxt->env)->GetObjectClass(cxt->thiz);	jmethodID mOnDebug =(cxt->env)->GetMethodID(cls, "OnDebug", "(Ljava/lang/String;)V");	jmethodID mOnTransfer =(cxt->env)->GetMethodID(cls, "onTransfer", "(JJLjava/lang/String;)V");	jmethodID mOnFinished =(cxt->env)->GetMethodID(cls, "onFinished", "(Ljava/lang/String;)V");	jsOutput = (cxt->env)->NewStringUTF("_SendThreadProc ");	(cxt->env)->CallVoidMethod(cxt->thiz, mOnDebug, jsOutput);	UDTSOCKET fhandle = -1;	string szFinish = "FAIL", szError = "";	UDT::startup();	addrinfo hints, *peer, *res;	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	char szPort[64];
	memset(szPort, 0, 64);
	sprintf(szPort,"%d", cxt->nPort);
	if (0 != getaddrinfo(cxt->pstrAddr, szPort, &hints, &peer))
	{
		szError = "getaddrinfo fail:";
		goto Loop;
	}

	fhandle = UDT::socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol);

	// connect to the server, implict bind
	if (UDT::ERROR == UDT::connect(fhandle, peer->ai_addr, peer->ai_addrlen))
	{
		goto Loop;
	}
	freeaddrinfo(peer);//	log << "SendType:" << cxt->eType << endl;	switch (cxt->eType)	{	case SND_MSG:		{			//////////////////////////////////////////////////////////////////////////
			// 消息传送通知（MSR）
		//	log << "Send message" << endl;			jsOutput = (cxt->env)->NewStringUTF("send message");			(cxt->env)->CallVoidMethod(cxt->thiz, mOnDebug, jsOutput);

			char head[8];
			memset(head, 0, 8);
			strcpy(head, "TSR");
			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
			{
				goto Loop;
			}

			// send name information of the requested file
			string actualName = cxt->pstrHostName;
			int len = actualName.size();
			if (UDT::ERROR == UDT::send(fhandle, (char*)&len, sizeof(int), 0))
			{
				goto Loop;
			}
			if (UDT::ERROR == UDT::send(fhandle, actualName.c_str(), len, 0))
			{
				goto Loop;
			}

			// send message size and information
			jstring jszMsg = (jstring)(cxt->env)->GetObjectArrayElement(cxt->strArray, 0);
			string strMessage = (char*)(cxt->env)->GetStringUTFChars(jszMsg, &b);
			int nLen = strMessage.size();
			if (UDT::ERROR == UDT::send(fhandle, (char*)&nLen, sizeof(int), 0))
			{
				goto Loop;
			}
			if (UDT::ERROR == UDT::send(fhandle, strMessage.c_str(), nLen, 0))
			{
				goto Loop;
			}

		//	log << "send message fineshed " << endl;			break;		}	case SND_UNIFILE:		{			//////////////////////////////////////////////////////////////////////////
			// 单文件传送请求（FSR）
		//	log << "Send unifile" << endl;

			char head[8];
			memset(head, 0, 8);
			strcpy(head, "FSR");
			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
			{
				goto Loop;
			}

			jstring jszFileName = (jstring)(cxt->env)->GetObjectArrayElement(cxt->strArray, 0);
			string strFileName = (char*)(cxt->env)->GetStringUTFChars(jszFileName, &b);

			string actualName;
			int pos = strFileName.find_last_of('/');
			actualName = strFileName.substr(pos+1);

			actualName += "\\";
			actualName += cxt->pstrHostName;
			if (0 == strcmp("GENIETURBO", cxt->pstrSendtype))
			{
				actualName += "\\";
				actualName += cxt->pstrSendtype;
			}

			// send name information of the requested file
			int nLen = actualName.size();
			if (UDT::ERROR == UDT::send(fhandle, (char*)&nLen, sizeof(int), 0))
			{
				goto Loop;
			}
			if (UDT::ERROR == UDT::send(fhandle, actualName.c_str(), nLen, 0))
			{
				goto Loop;
			}


			//文件传送应答（FSP）
			memset(head, 0, 8);
			if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 4, 0))
			{
				goto Loop;
			}

		//	log << "Recv FSP:" << head <<endl;
			if (strcmp(head, "FSP1") != 0)
			{
				szFinish = "";
				goto Loop;
			}

			//文件内容传送（FCS）
			memset(head, 0, 8);
			strcpy(head, "FCS");
			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
			{
				goto Loop;
			}

			// open the file
			fstream ifs(strFileName.c_str(), ios::in | ios::binary);
			ifs.seekg(0, ios::end);
			int64_t size = ifs.tellg();
			ifs.seekg(0, ios::beg);

			// send file size information
			if (UDT::ERROR == UDT::send(fhandle, (char*)&size, sizeof(int64_t), 0))
			{
				goto Loop;
			}

		//	log << "Send file name :" << actualName << ", file size:" << size << endl;
			// send the file
			int64_t offset = 0;
			int64_t left = size;
			int64_t iLastPercent = 0;
			jstring jsfile = (cxt->env)->NewStringUTF(actualName.c_str());
			while(left > 0)
			{
				int send = 0;
				if (left > 51200)
					send = UDT::sendfile(fhandle, ifs, offset, 51200);
				else
					send = UDT::sendfile(fhandle, ifs, offset, left);

				if (UDT::ERROR == send)
				{
					goto Loop;
				}
				left -= send;
				offset += send;

				// 接收应答、保证发送与接收的进度统一
				int iPercent = (offset*100)/size;
				if (iPercent == 1)
				{
					iPercent = 1;
				}
				if (iPercent > 100)
				{
					iPercent = iPercent;
				}
				if (iPercent != iLastPercent)
				{
				//	log << "recv percent, has send size:" << offset << endl;
					iLastPercent = iPercent;
					//文件传送应答（FCS）
					memset(head, 0, 8);
					if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 4, 0))
					{
						goto Loop;
					}
				}

				if (send > 0)
				{
					(cxt->env)->CallVoidMethod(cxt->thiz, mOnTransfer, size, offset, jsfile);
				}
			}
			ifs.close();		//	log << "send uniFile finished" << endl;			szFinish = "SUCCESS";			break;		}	case SND_MULFILE:		{			//////////////////////////////////////////////////////////////////////////
			// 多文件传送请求（MSR）
		//	log << "Send multifile" << endl;

			char head[8];
			memset(head, 0, 8);
			strcpy(head, "MSR");
			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
			{
				goto Loop;
			}

			// send total size
			jstring jsFileName;
			string oneFileName, strFileName;
			int64_t oneFileNameSize;
			int64_t totalSize = 0;
			int totalCount = 0;
			int row = (cxt->env)->GetArrayLength(cxt->strArray);//获得行数
			for(int i = 0; i < row; i++)
			{
				jsFileName = (jstring)(cxt->env)->GetObjectArrayElement(cxt->strArray, i);
				strFileName = (char*)(cxt->env)->GetStringUTFChars(jsFileName, &b);

				fstream ifs(strFileName.c_str(), ios::in | ios::binary);
				ifs.seekg(0, ios::end);
				int64_t size = ifs.tellg();
				totalSize += size;
				totalCount++;
				ifs.seekg(0, ios::beg);
				if (i == 0)
				{
					oneFileName = strFileName;
					oneFileNameSize = strFileName.length();
				}
				ifs.close();
			}

		//	log << "MultiFiles total size:" << totalSize << "totalCount:" << totalCount << endl;
			if (UDT::ERROR == UDT::send(fhandle, (char *)&totalSize, sizeof(int64_t), 0))
			{
				goto Loop;
			}

			// send file count
			if (UDT::ERROR == UDT::send(fhandle, (char*)&totalCount, sizeof(int), 0))
			{
				goto Loop;
			}

			string tmp = oneFileName;
			string actualName;
			int pos = oneFileName.find_last_of('/');
			actualName = tmp.substr(pos+1);
			actualName += "\\";
			actualName += cxt->pstrHostName;
			if (0 == strcmp("GENIETURBO", cxt->pstrSendtype))
			{
				actualName += "\\";
				actualName += cxt->pstrSendtype;
			}

			int actualLength = actualName.length();

			// send file name size
			if (UDT::ERROR == UDT::send(fhandle, (char*)&actualLength, sizeof(int), 0))
			{
				goto Loop;
			}
			if (UDT::ERROR == UDT::send(fhandle, actualName.c_str(), actualLength, 0))
			{
				goto Loop;
			}

			//文件传送应答（MSP）
			memset(head, 0, 8);
			if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 4, 0))
			{
				goto Loop;
			}

		//	log << "Recv request MSP:" << head << endl;
			if (strcmp(head, "MSP1") != 0)
			{
				szFinish = "";
				goto Loop;
			}

			int64_t sendsize = 0;
			int64_t iLastPercent = 0;
			for(int j = 0; j < row; j++)
			{
				memset(head, 0, 8);
				strcpy(head, "MCS");
				if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
				{
					goto Loop;
				}

				jsFileName = (jstring)(cxt->env)->GetObjectArrayElement(cxt->strArray, j);
				strFileName = (char*)(cxt->env)->GetStringUTFChars(jsFileName, &b);

				string tmp = strFileName;
				string actualName;
				int pos = strFileName.find_last_of('/');
				actualName = tmp.substr(pos+1);
				int actualLength = actualName.length();

				if (UDT::ERROR == UDT::send(fhandle, (char*)&actualLength, sizeof(int), 0))
				{
					goto Loop;
				}
				if (UDT::ERROR == UDT::send(fhandle, actualName.c_str(), actualLength, 0))
				{
					goto Loop;
				}
				jstring jsname = (cxt->env)->NewStringUTF(actualName.c_str());

				fstream ifs(strFileName.c_str(), ios::in | ios::binary);
				ifs.seekg(0, ios::end);
				int64_t size = ifs.tellg();
				ifs.seekg(0, ios::beg);

				// send file size information
				if (UDT::ERROR == UDT::send(fhandle, (char*)&size, sizeof(int64_t), 0))
				{
					goto Loop;
				}

			//	log << "send file full name and size:" << strFileName << "-" << size << endl;
				//UDT::TRACEINFO trace;
				//UDT::perfmon(fhandle, &trace);

				// send the file
				int64_t offset = 0;
				int64_t left = size;
				while(left > 0)
				{
					int send = 0;
					if (left > 51200)
						send = UDT::sendfile(fhandle, ifs, offset, 51200);
					else
						send = UDT::sendfile(fhandle, ifs, offset, left);

					if (UDT::ERROR == send)
					{
						goto Loop;
					}

					left -= send;
					offset += send;
					sendsize += send;

					// 接收应答、保证发送与接收的进度统一
					int iPercent = (sendsize*100)/totalSize;
					if (iPercent == 1)
					{
						iPercent = 1;
					}
					if (iPercent > 100)
					{
						iPercent = iPercent;
					}
					if (iPercent != iLastPercent)
					{
					//	log << "recv percent, has send size:" << sendsize << endl;
						iLastPercent = iPercent;
						//文件传送应答（FCS）
						memset(head, 0, 8);
						if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 4, 0))
						{
							goto Loop;
						}
					}
					(cxt->env)->CallVoidMethod(cxt->thiz, mOnTransfer, totalSize, sendsize, jsname);
				}
				ifs.close();

			//	log << "send file finished:" << strFileName << endl;

				//UDT::perfmon(fhandle, &trace);
				//cout << "speed = " << trace.mbpsSendRate << "Mbits/sec" << endl;
			}

			memset(head, 0, 8);
			strcpy(head, "MSF");
			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
			{
				goto Loop;
			}

		//	log << "Send multiFile finished" << endl;			szFinish = "SUCCESS";			break;		}	case SND_FOLDER:		{			//////////////////////////////////////////////////////////////////////////
			// 文件夹传送请求（DSR）
		//	log << "Send folderFile" << endl;

			char head[8];
			memset(head, 0, 8);
			strcpy(head, "DSR");
			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
			{
				goto Loop;
			}

			jstring jszFolderName = (jstring)(cxt->env)->GetObjectArrayElement(cxt->strArray, 0);
			string strFolderName = (char*)(cxt->env)->GetStringUTFChars(jszFolderName, &b);

			string tmp = strFolderName;
			string folderName, devName, actualName;
			string fileName, filePath;

			int64_t totalFileSize = 0;
			vector<string> vecFilePath;
			searchFileInDirectroy(strFolderName.c_str(), totalFileSize, vecFilePath);
		//	log << "Foler name and  totalSize:" << strFolderName << "-" << totalFileSize << endl;

			int pos = tmp.find_last_of('/');
			folderName = tmp.substr(pos+1);
			actualName = tmp.substr(pos+1);
			actualName += "\\";
			actualName += cxt->pstrHostName;
			if (0 == strcmp("GENIETURBO", cxt->pstrSendtype))
			{
				actualName += "\\";
				actualName += cxt->pstrSendtype;
			}

			// send file count size
			if (UDT::ERROR == UDT::send(fhandle, (char*)&totalFileSize, sizeof(int64_t), 0))
			{
				goto Loop;
			}

			int actualLength = actualName.length();
			// send foldername size
			if (UDT::ERROR == UDT::send(fhandle, (char *)&actualLength, sizeof(int), 0))
			{
				goto Loop;
			}
			// send foldername
			if (UDT::ERROR == UDT::send(fhandle, actualName.c_str(), actualLength, 0))
			{
				goto Loop;
			}

			//文件夹传送应答（FSP）
			memset(head, 0, 8);
			if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 4, 0))
			{
				goto Loop;
			}
			//if (memcmp(head, "DSP1", 0)!=0)
			if (strcmp(head, "DSP1") != 0)
			{
				szFinish = "";
				goto Loop;
			}

			string strTempForlder;
			vector<string> vecDir;
			//空文件夹传输请求
			for(int i = 0; i < vecFilePath.size(); i++)
			{
				int len = 0;
				tmp = vecFilePath[i];
				int pos = tmp.find_last_of('\\');
				if (pos > 0)
				{
					strTempForlder = tmp.substr(0,pos);
					tmp = tmp.substr(0,pos);
				}
				else
				{
					pos = tmp.find_last_of('/');
					filePath = tmp.substr(0,pos);
					tmp = tmp.substr(0,pos);
				}

				len = strFolderName.size() - folderName.size();
				tmp = filePath.substr(len);
				for (int j = 0; j < vecDir.size(); j++)
				{
					if (vecDir[j].compare(tmp) == 0)
					{
						break;
					}
				}
				vecDir.push_back(tmp);

				memset(head, 0, 8);
				strcpy(head,"DCR");
				if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
				{
					goto Loop;
				}

				len = tmp.size();
				if (UDT::ERROR == UDT::send(fhandle, (char*)&len, sizeof(int), 0))
				{
					goto Loop;
				}

				if (UDT::ERROR == UDT::send(fhandle, tmp.c_str(), len, 0))
				{
					goto Loop;
				}

			//	log << "Foldername :" << tmp << endl;
			}

			int64_t sendsize = 0;
			int64_t iLastPercent = 0;
			//目录文件传输请求
			for(int i = 0; i < vecFilePath.size(); i++)
			{
				memset(head, 0, 8);
				strcpy(head,"DFS");
				if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
				{
					goto Loop;
				}

				int len = 0;
				filePath = vecFilePath[i];

				len = strFolderName.size() - folderName.size();
				fileName = filePath.substr(len);
			//	log << "file name :" << fileName << "\n" << endl;

				len = fileName.size();
				if (UDT::ERROR == UDT::send(fhandle, (char*)&len, sizeof(int), 0))
				{
					goto Loop;
				}

				if (UDT::ERROR == UDT::send(fhandle, fileName.c_str(), len, 0))
				{
					goto Loop;
				}

				string actualName;
				int pos = fileName.find_last_of('/');
				actualName = fileName.substr(pos+1);
				jstring jsname = (cxt->env)->NewStringUTF(actualName.c_str());

				// open the file
				fstream ifs(filePath.c_str(), ios::in | ios::binary);
				ifs.seekg(0, ios::end);
				int64_t size = ifs.tellg();
				ifs.seekg(0, ios::beg);

				// send file size information
				if (UDT::ERROR == UDT::send(fhandle, (char*)&size, sizeof(int64_t), 0))
				{
					goto Loop;
				}

			//	log << "send file full name and size :" << filePath << "-" << size << endl;
				// send the file
				int64_t offset = 0;
				int64_t left = size;
				while(left > 0)
				{
					int send = 0;
					if (left > 51200)
						send = UDT::sendfile(fhandle, ifs, offset, 51200);
					else
						send = UDT::sendfile(fhandle, ifs, offset, left);

					if (UDT::ERROR == send)
					{
						goto Loop;
					}
					left -= send;
					offset += send;
					sendsize += send;

					// 接收应答、保证发送与接收的进度统一
					int iPercent = (sendsize*100)/totalFileSize;
					if (iPercent == 1)
					{
						iPercent = 1;
					}
					if (iPercent > 100)
					{
						iPercent = iPercent;
					}
					if (iPercent != iLastPercent)
					{
					//	log << "recv percent, has send size:" << sendsize << endl;
						iLastPercent = iPercent;
						//文件传送应答（FCS）
						memset(head, 0, 8);
						if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 4, 0))
						{
							goto Loop;
						}
					}
					(cxt->env)->CallVoidMethod(cxt->thiz, mOnTransfer, totalFileSize, sendsize, jsname);
				}
				ifs.close();

			//	log << "send file finished:" << filePath << endl;
			}

			//目录传送完成
			memset(head, 0, 8);
			strcpy(head, "DSF");
			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 3, 0))
			{
				goto Loop;
			}

		//	log << "Send finished\n" << endl;			szFinish = "SUCCESS";			break;		}	default:
	//	log << "SendType error" << endl;
		break;	}	// goto loop for endLoop:	// SUCCESS, FAIL	if (!szFinish.empty())	{
		jsOutput = (cxt->env)->NewStringUTF(szFinish.c_str());
		(cxt->env)->CallVoidMethod(cxt->thiz, mOnFinished, jsOutput);	}

	// Debug info
	if (szError.empty())	{		szError = UDT::getlasterror().getErrorMessage();		jsOutput = (cxt->env)->NewStringUTF(szError.c_str());
		(cxt->env)->CallVoidMethod(cxt->thiz, mOnDebug, jsOutput);	}

	UDT::close(fhandle);//	log.close();	(cxt->jvm)->DetachCurrentThread();	delete cxt;	#ifndef WIN32		return NULL;	#else		return 0;	#endif}#ifndef WIN32void * _RecvThreadProc(void * pParam)#elseDWORD WINAPI _RecvThreadProc(LPVOID pParam)#endif{	AppContext *context = (AppContext *)pParam;	UDTSOCKET fhandle = context->sock;	sockRecv = fhandle;	ReSetReplyAccept();//	fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);//	log << "***UdtRecv***" << endl;	//time_t logTime;	//logTime = time(NULL);	//char timeBuf[62];	//strftime(timeBuf, sizeof(timeBuf), "%Y/%m/%d %X", localtime(&logTime));	//log << timeBuf << "strReplyPath:" << strReplyPath << "/n" << endl;	char head[8];	vector<string> vecFileName;	jboolean  b  = true;	string filepath = "/mnt/sdcard/";;	string strErrorMsg, strTemp, strFolderName;	int64_t totalSize = 0;	int64_t recvSize = 0;	int64_t iLastPercent = 0;	(context->jvm)->AttachCurrentThread(&(context->env), NULL);	//回调方法	jclass cls = (context->env)->GetObjectClass(context->thiz);	jmethodID mOnAccept = (context->env)->GetMethodID(cls,"onAccept","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");	jmethodID mOnAcceptonFinish = (context->env)->GetMethodID(cls, "onAcceptonFinish", "(Ljava/lang/String;[Ljava/lang/Object;)V");	jmethodID mOnAcceptFolder = (context->env)->GetMethodID(cls, "onAcceptFolder", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");	jmethodID mOnTransfer =(context->env)->GetMethodID(cls,"onTransfer","(JJLjava/lang/String;)V");	jmethodID mOnFinished =(context->env)->GetMethodID(cls,"onFinished","(Ljava/lang/String;)V");	jmethodID mOnRecvMessage =(context->env)->GetMethodID(cls,"onRecvMessage","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");	while(true)	{		memset(head, 0, 8);		if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 3, 0))		{			//log << "recv head: " << strErrorMsg << endl;			//jstring jsret = (context->env)->NewStringUTF("FAIL");			//(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);			break;		}		//////////////////////////////////////////////////////////////////////////		// 1.	文字传送请求（TSR）		if (memcmp(head,"TSR",3) == 0)		{			int len;			// recv hostName len			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			//recv hostName			char * hostName = new char[len + 2];			if (UDT::ERROR == UDT::recv(fhandle, hostName, len, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			hostName[len] = '\0';			//recv message info len			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			//recv message info			char * message = new char[len+2];			if (UDT::ERROR == UDT::recv(fhandle, message, len, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			message[len] = '\0';			string strMessage = message;			string strHostName = hostName;			jstring jsret = (context->env)->NewStringUTF(strMessage.c_str());			jstring jshost = (context->env)->NewStringUTF((char*)context->clienthost);			jstring jshostName = (context->env)->NewStringUTF(strHostName.c_str());			(context->env)->CallVoidMethod(context->thiz, mOnRecvMessage,jsret, jshost, jshostName);			break;		}		//////////////////////////////////////////////////////////////////////////		// 2.	单文件传送请求（FSR）		else if (memcmp(head,"FSR",3) == 0)		{			//log << "FSR" << endl;			int len;			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			char *file = new char[len+2];			if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))			{				cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				//                log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			file[len] = '\0';			string tmp = file;			string devName;			string strCloud;			string fileName;			int pos = tmp.find_last_of('\\');			int iFirstPos;			if (pos > 0)			{				iFirstPos = tmp.find_first_of('\\');				if (pos == iFirstPos)				{					devName = tmp.substr(pos+1);					//tmp = tmp.substr(pos+1);					fileName = tmp.substr(0,pos);					tmp = tmp.substr(0,pos);				}				else				{					devName = tmp.substr(iFirstPos+1,pos-iFirstPos-1);					fileName = tmp.substr(0,iFirstPos);					strCloud = tmp.substr(pos+1);					tmp = tmp.substr(0,iFirstPos);				}			}			else			{				break;			}			jstring jshost = (context->env)->NewStringUTF((char*)context->clienthost);			jstring jsDeviceName = (context->env)->NewStringUTF(devName.c_str());			jstring jsFileName = (context->env)->NewStringUTF(fileName.c_str());			jstring jsSendType = (context->env)->NewStringUTF("empty");			if (!strCloud.empty())			{				jsSendType = (context->env)->NewStringUTF(strCloud.c_str());			}			//char *  strSendType = (char*) (context->env)->GetStringUTFChars(jsSendType, &b);			//jstring jsPath = (jstring) (context->env)->CallObjectMethod(context->thiz, mOnAccept, jshost, jsDeviceName, jsSendType, jsFileName, 1);			//filepath =  context->env->GetStringUTFChars(jsPath, &b);			(context->env)->CallVoidMethod(context->thiz, mOnAccept, jshost, jsDeviceName, jsSendType, jsFileName, 1);			int nWaitTime = 0;			string strReplyPath = "";			while(nWaitTime < 31)			{				const char * str = getReplyAccept();				strReplyPath = str;				//if (strReplyPath.compare("REJECT") == 0 || strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.compare(0, 12, "/mnt/sdcard/") == 0)				if (!strReplyPath.empty())				{					break;				}			//	memset(head, 0, 8);			//	strcpy(head,"WAIT");			//	if (UDT::ERROR == UDT::send(fhandle, (char*)head, 4, 0))			//	{					//log << "send FSP: " << UDT::getlasterror().getErrorMessage() << endl;			//		jstring jsret = (context->env)->NewStringUTF("FAIL");			//		(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);			//		UDT::close(fhandle);			//		return NULL;			//	}				nWaitTime += 1;				sleep(1);			}			// 目录传送应答（FSP）			memset(head, 0, 8);			if (strReplyPath.compare("REJECT")==0)			{				strcpy(head,"FSP0");			}			else if (strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.empty())			{				strcpy(head,"FSP2");			}			else			{				strcpy(head,"FSP1");			}			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 4, 0))			{				//log << "send FSP: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}		//	log << "send FSP:" << head << endl;			if (strcmp(head, "FSP1") != 0)			{				break;			}			//文件内容传送（FCS）			memset(head, 0, 8);			if (UDT::ERROR == UDT::recv(fhandle, (char*)head, 3, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			filepath = strReplyPath + fileName;			int64_t size;			if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}		//	log << "receive fileName and size:" << filepath << "-" << size << endl;			// receive the file			fstream ofs(filepath.c_str(), ios::out | ios::binary | ios::trunc);			int64_t offset = 0;			int64_t left = size;			jstring jsfile = (context->env)->NewStringUTF(file);			while(left > 0)			{				int recv = 0;				if (left > 51200)					recv = UDT::recvfile(fhandle, ofs, offset, 51200);				else					recv = UDT::recvfile(fhandle, ofs, offset, left);				if (UDT::ERROR == recv)				{					//log << "recv size: " << UDT::getlasterror().getErrorMessage() << endl;					jstring jsret = (context->env)->NewStringUTF("FAIL");					(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);					ofs.close();					break;				}				left -= recv;				offset +=recv;				// 发送文件接收进度				int iPercent = (offset*100)/size;				if (iPercent == 1)				{					iPercent = 1;				}				if (iPercent > 100)				{					iPercent = iPercent;				}				if (iPercent != iLastPercent)				{					iLastPercent = iPercent;					//文件传送应答（FSC）					memset(head, 0, 8);					strcpy(head,"FSC1");					if (UDT::ERROR == UDT::send(fhandle, (char*)head, 4, 0))					{						//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;						jstring jsret = (context->env)->NewStringUTF("FAIL");						(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);						ofs.close();						break;					}				}				if (size-left > 0)				{					(context->env)->CallVoidMethod(context->thiz, mOnTransfer, size, size-left, jsfile);				}				if (left <= 0)				{				//	log << "receive the file Finished" << endl;					jstring jsret = (context->env)->NewStringUTF("SUCCESS");					(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);					ofs.close();				}			}			break;		}		//////////////////////////////////////////////////////////////////////////		// 3.	文件夹传送请求（DSR）		else if (memcmp(head,"DSR",3) == 0)		{			// aquiring directory name information from client			int len;			uint64_t size;			if (UDT::ERROR == UDT::recv(fhandle, (char*)&totalSize, sizeof(uint64_t), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}		//	log << "recv DSR total size:" << totalSize << endl;			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			char *file = new char[len+2];			if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			file[len] = '\0';		//	log << "recv DSR Directory name:" << file << endl;			string tmp = file;			string devName;			string strCloud;			string fileName;			int pos = tmp.find_last_of('\\');			int iFirstPos;			if (pos > 0)			{				iFirstPos = tmp.find_first_of('\\');				if (pos == iFirstPos)				{					devName = tmp.substr(pos+1);					//tmp = tmp.substr(pos+1);					fileName = tmp.substr(0,pos);					tmp = tmp.substr(0,pos);				}				else				{					devName = tmp.substr(iFirstPos+1,pos-iFirstPos-1);					fileName = tmp.substr(0,iFirstPos);					strCloud = tmp.substr(pos+1);					tmp = tmp.substr(0,iFirstPos);				}			}			else			{				break;			}			strFolderName = fileName;			jstring jshost = (context->env)->NewStringUTF((char*)context->clienthost);			jstring jsDeviceName = (context->env)->NewStringUTF(devName.c_str());			jstring jsFileName = (context->env)->NewStringUTF(fileName.c_str());			jstring jsSendType = (context->env)->NewStringUTF("UNKNOWN");			if (!strCloud.empty())			{				jsSendType = (context->env)->NewStringUTF(strCloud.c_str());			}			(context->env)->CallVoidMethod(context->thiz, mOnAcceptFolder, jshost, jsDeviceName, jsSendType, jsFileName, 1);			int nWaitTime = 0;			string strReplyPath = "";			while(nWaitTime < 31)			{				const char * str = getReplyAccept();				strReplyPath = str;				//if (strReplyPath.compare("REJECT") == 0 || strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.compare(0, 12, "/mnt/sdcard/") == 0)				if (!strReplyPath.empty())				{					break;				}				nWaitTime += 1;				sleep(1);			}			// 目录传送应答（DSP）			memset(head, 0, 8);			if (strReplyPath.compare("REJECT")==0)			{				strcpy(head,"DSP0");			}			else if (strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.empty())			{				strcpy(head,"DSP2");			}			else			{				strcpy(head,"DSP1");				filepath = strReplyPath;			}		//	log << "send DSP:" << head << endl;			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 4, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			if (strcmp(head, "DSP1") != 0)			{				break;			}		}		else if (memcmp(head,"DCR",3) == 0) // c)	目录创建请求（DCR）		{			int len;			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			char *file = new char[len+2];			if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			file[len] = '\0';			string fn =  filepath + file + "/";			string strFolder = filepath;			DIR * pDir;			// mkdir: /mnt/sdcard/NetgearGenie/baidu/searchbox/img_cache/			while (strFolder.compare(fn) != 0)
			{
				strTemp = fn.substr(strFolder.size());
				int nPos = strTemp.find_first_of('/');
				if (nPos > 0)
				{
					strFolder = strFolder + strTemp.substr(0, nPos);
					if ((pDir = opendir(strFolder.c_str())) == NULL)					{						if (mkdir(strFolder.c_str(), 0777) == 0)						{						//	log << "mkdir error: " << strFolder << endl;						//	jstring jsret = (context->env)->NewStringUTF("FAIL");						//	(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);						//	break;						}					//	log << "mkdir sucess: " << strFolder << endl;					}
					strFolder += "/";
				}
			}		/*	if ((pDir = opendir(fn.c_str())) == NULL)			{				int nTime = 0;				while (nTime <= 5)				{					if (mkdir(fn.c_str(),0777) == 0)					{						break;					}					nTime++;					sleep(1);				}				log << "mkdir error: " << fn << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}	*/		}		else if (memcmp(head,"DFS",3) == 0)		{			int len;			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			char *file = new char[len+2];			if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			file[len] = '\0';			string fn =  filepath + file;			vecFileName.push_back(file);			// get size information			int64_t size;			if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}		//	log << "file name and size:" << fn << "-" << size << endl;			// receive the file			fstream ofs(fn.c_str(), ios::out | ios::binary | ios::trunc);			int64_t offset = 0;			int64_t left = size;			jstring jsfile = (context->env)->NewStringUTF(file);			while(left > 0)			{				int recv = 0;				if (left > 51200)					recv = UDT::recvfile(fhandle, ofs, offset, 51200);				else					recv = UDT::recvfile(fhandle, ofs, offset, left);				if (UDT::ERROR == recv)				{					//log <<"recv size: " << UDT::getlasterror().getErrorMessage() << endl; 					jstring jsret = (context->env)->NewStringUTF("FAIL");					(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);					break;				}				left -= recv;				offset +=recv;				recvSize += recv;				if (size > 0 && recv <= 0)				{					jstring jsret = (context->env)->NewStringUTF("FAIL");					(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);					break;				}				// 发送文件接收进度				int iPercent = (recvSize*100)/totalSize;				if (iPercent == 1)				{					iPercent = 1;				}				if (iPercent > 100)				{					iPercent = iPercent;				}				if (iPercent != iLastPercent)				{					iLastPercent = iPercent;					//文件传送应答（DSC）					memset(head, 0, 8);					strcpy(head,"DSC1");					if (UDT::ERROR == UDT::send(fhandle, (char*)head, 4, 0))					{						//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;						jstring jsret = (context->env)->NewStringUTF("FAIL");						(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);						break;					}				}			//	log << "recv file and size:" << fn << "-" << recvSize <<  endl;				(context->env)->CallVoidMethod(context->thiz, mOnTransfer, totalSize, recvSize, jsfile);			}			ofs.close();		}		else if (memcmp(head,"DSF",3) == 0) //d)	目录传送结束（DFS）		{			jstring jfilename;			jstring jshost = (context->env)->NewStringUTF((char*)context->clienthost);			jobjectArray arry = (context->env)->NewObjectArray(1, (context->env)->FindClass("java/lang/String"), 0);			jfilename = (context->env)->NewStringUTF(strFolderName.c_str());			(context->env)->SetObjectArrayElement(arry, 0, jfilename);			(context->env)->CallVoidMethod(context->thiz, mOnAcceptonFinish, jshost, arry);		//	log << "recv DSF request mOnAcceptonFinish" << endl;			jstring jsret = (context->env)->NewStringUTF("SUCCESS");			(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);			break;		}		//////////////////////////////////////////////////////////////////////////		// 4.	多文件传送（MFS）		else if (memcmp(head,"MSR",3) == 0)		{			//log << "recv MSR request" << endl;			// receive total count			if (UDT::ERROR == UDT::recv(fhandle, (char*)&totalSize, sizeof(uint64_t), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			//log << "total size:" << totalSize << "/n" << endl;			int count;			// receive file count			if (UDT::ERROR == UDT::recv(fhandle, (char*)&count, sizeof(int), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}		//	log << "multiFile totalSize and totalCount:" << totalSize << "-" << count << endl;			int len;			// receive one file name size			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv one file name length error : " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			char *file = new char[len+2];			// receive one file name			if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			file[len] = '\0';			//log << "one file name:" << file << endl;			//string strFile = file;			string tmp = file;			string devName;			string strCloud;			string fileName;			int pos = tmp.find_last_of('\\');			int iFirstPos;			if (pos > 0)			{				iFirstPos = tmp.find_first_of('\\');				if (pos == iFirstPos)				{					devName = tmp.substr(pos+1);					//tmp = tmp.substr(pos+1);					fileName = tmp.substr(0,pos);					tmp = tmp.substr(0,pos);				}				else				{					devName = tmp.substr(iFirstPos+1,pos-iFirstPos-1);					fileName = tmp.substr(0,iFirstPos);					strCloud = tmp.substr(pos+1);					tmp = tmp.substr(0,iFirstPos);				}			}			else			{				break;			}			jstring jshost = (context->env)->NewStringUTF((char*)context->clienthost);			jstring jsDeviceName = (context->env)->NewStringUTF(devName.c_str());			jstring jsFileName = (context->env)->NewStringUTF(fileName.c_str());			jstring jsSendType = (context->env)->NewStringUTF("UNKNOWN");			jstring jsPath;			if (!strCloud.empty())			{				jsSendType = (context->env)->NewStringUTF(strCloud.c_str());			}			(context->env)->CallVoidMethod(context->thiz, mOnAcceptFolder, jshost, jsDeviceName, jsSendType, jsFileName, count);			int nWaitTime = 0;			string strReplyPath = "";			while(nWaitTime < 31)			{				const char * str = getReplyAccept();				strReplyPath = str;				//if (strReplyPath.compare("REJECT") == 0 || strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.compare(0, 12, "/mnt/sdcard/") == 0)				if (!strReplyPath.empty())				{					break;				}				nWaitTime += 1;				sleep(1);			}			//log << "strReplyPath:" << strReplyPath << "\n" << endl;			//文件传送应答（FSP）			memset(head, 0, 8);			if (strReplyPath.compare("REJECT")==0)			{				strcpy(head,"MSP0");			}			else if (strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.empty())			{				strcpy(head,"MSP2");			}			else			{				filepath = strReplyPath;				strcpy(head,"MSP1");			}			if (UDT::ERROR == UDT::send(fhandle, (char*)head, 4, 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			//log << "send MSP request:" << head << endl;		}		else if (memcmp(head,"MCS",3) == 0)		{			int len;			// receive file name size			if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))			{				//log << "recv file name length error : " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			char *file = new char[len+2];			if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))			{				//log << "recv file name length error : " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			file[len] = '\0';			vecFileName.push_back(file);		//	log << "recv file name" << file << "\n" << endl;			uint64_t size;			// receive total count			if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(uint64_t), 0))			{				//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;				jstring jsret = (context->env)->NewStringUTF("FAIL");				(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);				break;			}			string fn =  filepath + file;			// receive the file			fstream ofs(fn.c_str(), ios::out | ios::binary | ios::trunc);			int64_t offset = 0;			int64_t left = size;			jstring jsfile = (context->env)->NewStringUTF(file);			while(left > 0)			{				int recv = 0;				if (left > 51200)					recv = UDT::recvfile(fhandle, ofs, offset, 51200);				else					recv = UDT::recvfile(fhandle, ofs, offset, left);				if (UDT::ERROR == recv)				{					cout << "recv size: " << UDT::getlasterror().getErrorMessage() << endl;					jstring jsret = (context->env)->NewStringUTF("FAIL");					(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);					ofs.close();					break;				}				left -= recv;				offset +=recv;				recvSize += recv;				// 发送文件接收进度				int iPercent = (recvSize*100)/totalSize;				if (iPercent == 1)				{					iPercent = 1;				}				if (iPercent > 100)				{					iPercent = iPercent;				}				if (iPercent != iLastPercent)				{					iLastPercent = iPercent;					//文件传送应答（FSC）					memset(head, 0, 8);					strcpy(head,"MSC1");					if (UDT::ERROR == UDT::send(fhandle, (char*)head, 4, 0))					{						//log << "recv: " << UDT::getlasterror().getErrorMessage() << endl;						jstring jsret = (context->env)->NewStringUTF("FAIL");						(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);						ofs.close();						break;					}				}				if (size-left > 0)				{					(context->env)->CallVoidMethod(context->thiz, mOnTransfer, totalSize, recvSize, jsfile);				}			}			ofs.close();		//	log << "recv file finished:" << fn << endl;		}		else if (memcmp(head,"MSF",3) == 0)		{			int nCount = vecFileName.size();			jstring jfilename;			jstring jshost = (context->env)->NewStringUTF((char*)context->clienthost);			jobjectArray arry = (context->env)->NewObjectArray(nCount, (context->env)->FindClass("java/lang/String"), 0);			for (int i = 0; i < nCount; i++)			{				jfilename = (context->env)->NewStringUTF(vecFileName[i].c_str());				(context->env)->SetObjectArrayElement(arry, i, jfilename);			}			(context->env)->CallVoidMethod(context->thiz, mOnAcceptonFinish, jshost, arry);		//	log << "recv MSF request mOnAcceptonFinish" << endl;			jstring jsret = (context->env)->NewStringUTF("SUCCESS");			(context->env)->CallVoidMethod(context->thiz, mOnFinished,jsret);			break;		}	}	UDT::close(fhandle);//	log.close();	(context->jvm)->DetachCurrentThread();	delete context;#ifndef WIN32	return NULL;#else	return 0;#endif}void stopListen(){#ifndef WIN32	pthread_mutex_lock(&Accept_Lock);	pthread_cond_signal(&Accpet_Cond);	pthread_mutex_unlock(&Accept_Lock);#else	SetEvent(_hAcceptThread);#endif}extern "C"{	jint Java_com_dragonflow_FileTransfer_sendMessage(JNIEnv* env, jobject thiz,		jstring host, jint port,jstring message, jstring hostname, jstring sendtype)	{		jboolean  b  = true;		jobjectArray arry = (env)->NewObjectArray(1, (env)->FindClass("java/lang/String"), 0);

		(env)->SetObjectArrayElement(arry, 0, message);		SENDTYPE eType = SND_MSG;		int ret = SendEx(env, thiz, (char*)env->GetStringUTFChars(host,&b), port, (char*)env->GetStringUTFChars(hostname,&b), (char*)env->GetStringUTFChars(sendtype,&b), arry, eType);		return ret;	}	jint Java_com_dragonflow_FileTransfer_sendFile( JNIEnv* env, jobject thiz,		jstring host, jint port,jstring fileName, jstring hostname, jstring sendtype)	{		jboolean  b  = true;		jobjectArray arry = (env)->NewObjectArray(1, (env)->FindClass("java/lang/String"), 0);

		(env)->SetObjectArrayElement(arry, 0, fileName);		SENDTYPE eType = SND_UNIFILE;		int ret = SendEx(env, thiz, (char*)env->GetStringUTFChars(host,&b), port, (char*)env->GetStringUTFChars(hostname,&b), (char*)env->GetStringUTFChars(sendtype,&b), arry, eType);		return ret;	}	jint Java_com_dragonflow_FileTransfer_sendfolder(JNIEnv* env, jobject thiz,		jstring host, jint port, jstring folderName, jstring hostname, jstring sendtype)	{		jboolean  b  = true;		jobjectArray arry = (env)->NewObjectArray(1, (env)->FindClass("java/lang/String"), 0);

		(env)->SetObjectArrayElement(arry, 0, folderName);		SENDTYPE eType = SND_FOLDER;		int ret = SendEx(env, thiz, (char*)env->GetStringUTFChars(host,&b), port, (char*)env->GetStringUTFChars(hostname,&b), (char*)env->GetStringUTFChars(sendtype,&b), arry, eType);		return ret;	}	jint Java_com_dragonflow_FileTransfer_sendMultiFiles(JNIEnv* env, jobject thiz,		jstring host, jint port, jobjectArray filelist, jstring hostname, jstring sendtype)	{		jboolean b = true;		SENDTYPE eType = SND_MULFILE;		int ret = SendEx(env, thiz, (char*)env->GetStringUTFChars(host,&b), port, (char*)env->GetStringUTFChars(hostname,&b), (char*)env->GetStringUTFChars(sendtype,&b), filelist, eType);		return ret;	}	jint Java_com_dragonflow_FileTransfer_listenFileSend( JNIEnv* env,  jobject thiz, jint port)	{		int ret = listenFileSend( env,  thiz, port);		return ret;	}	jint Java_com_dragonflow_FileTransfer_stopListen( JNIEnv* env, jobject thiz)	{		stopListen();		return 0;	}	jint Java_com_dragonflow_FileTransfer_stopTransfer(JNIEnv* env,  jobject thiz, jint type)	{		stopTransfer(env, thiz, type);		return 0;	}	jint Java_com_dragonflow_FileTransfer_replyAccept( JNIEnv* env, jobject thiz,		jstring strReply)	{		jboolean  b  = true;		int ret = setReplyAccept(env, thiz, (char*)env->GetStringUTFChars(strReply,&b));		return ret;	}}