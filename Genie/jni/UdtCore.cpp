#include "UdtCore.h"


using namespace std;

CUdtCore::CUdtCore(CUDTCallBack * pCallback)
	: m_pCallBack(pCallback)
	, m_sockListen(-1)
	, m_bListenStatus(false)
	, m_bSendStatus(false)
	, m_bRecvStatus(false)
{
}


CUdtCore::~CUdtCore()
{
}

int CUdtCore::listenFileSend(const int nPort)
{
	if (m_bListenStatus)
		return true;

	UDT::startup();

	addrinfo hints;
	addrinfo * res;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	m_nPortListen = nPort;
	string service("7777");
	if (0 != getaddrinfo(NULL, service.c_str(), &hints, &res))
	{
		cout << "illegal port number or port is busy.\n" << endl;
		return 0;
	}

	m_sockListen = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// UDT Options
	//UDT::setsockopt(m_sockListen, 0, UDT_CC, new CCCFactory<CUDPBlast>, sizeof(CCCFactory<CUDPBlast>));
	//UDT::setsockopt(m_sockListen, 0, UDT_MSS, new int(9000), sizeof(int));
	//UDT::setsockopt(m_sockListen, 0, UDT_RCVBUF, new int(10000000), sizeof(int));
	//UDT::setsockopt(m_sockListen, 0, UDP_RCVBUF, new int(10000000), sizeof(int));

	// Windows UDP issue
	// For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
	int mss = 1052;
	UDT::setsockopt(m_sockListen, 0, UDT_MSS, &mss, sizeof(int));
#else
	int snd_buf = 64000;
	int rcv_buf = 64000;
	UDT::setsockopt(m_sockListen, 0, UDT_SNDBUF, &snd_buf, sizeof(snd_buf));
	UDT::setsockopt(m_sockListen, 0, UDT_RCVBUF, &rcv_buf, sizeof(rcv_buf));

	UDT::setsockopt(m_sockListen, 0, UDT_SNDBUF, &snd_buf, sizeof(snd_buf));
	UDT::setsockopt(m_sockListen, 0, UDT_RCVBUF, &rcv_buf, sizeof(rcv_buf));
#endif

	if (UDT::ERROR == UDT::bind(m_sockListen, res->ai_addr, res->ai_addrlen))
	{
		cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}
	freeaddrinfo(res);

	// listen socket
	if (UDT::ERROR == UDT::listen(m_sockListen, 10))
	{
		cout << "listen fail: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

#ifndef WIN32
	pthread_mutex_init(&m_AcceptLock, NULL);
	pthread_cond_init(&m_AcceptCond, NULL);
	pthread_create(&m_hListenThread, NULL, _ListenThreadProc, this);
#else
	DWORD dwThreadID;
	m_AcceptLock = CreateMutex(NULL, false, NULL);
	m_AcceptCond = CreateEvent(NULL, false, false, NULL);
	m_hListenThread = CreateThread(NULL, 0, _ListenThreadProc, this, NULL, &dwThreadID);
#endif

	m_bListenStatus = true;

	return 0;
}

void CUdtCore::SendEx(const char* pstrAddr, const int nPort, const char* pstrHostName, const char* pstrSendtype, const std::vector<std::string> vecArray, int nType)
{
	m_pSrvCxt = new _stServerContext();

	memset(m_pSrvCxt, 0, sizeof(_stServerContext));
	memcpy(m_pSrvCxt->strAddr, pstrAddr, 32);
	memcpy(m_pSrvCxt->strHostName, pstrHostName, 128);
	memcpy(m_pSrvCxt->strSendtype, pstrSendtype, 128);

	m_pSrvCxt->vecArray = vecArray;
	sprintf(m_pSrvCxt->strPort, "%d", nPort);
	if (nType == 1)
	{
		memcpy(m_pSrvCxt->strXSR, "TSR", 3);
		memcpy(m_pSrvCxt->strXSP, "TSP1", 4);
		memcpy(m_pSrvCxt->strXCS, "TCS", 3);
		memcpy(m_pSrvCxt->strXSF, "TSF", 3);
	}
	else if (nType == 2)
	{
		memcpy(m_pSrvCxt->strXSR, "FSR", 3);
		memcpy(m_pSrvCxt->strXSP, "FSP1", 4);
		memcpy(m_pSrvCxt->strXCS, "FCS", 3);
		memcpy(m_pSrvCxt->strXSF, "FSF", 3);
	}
	else if (nType == 3)
	{
		memcpy(m_pSrvCxt->strXSR, "MSR", 3);
		memcpy(m_pSrvCxt->strXSP, "MSP1", 4);
		memcpy(m_pSrvCxt->strXCS, "MCS", 3);
		memcpy(m_pSrvCxt->strXSF, "MSF", 3);
	}
	else if (nType == 4)
	{
		memcpy(m_pSrvCxt->strXSR, "DSR", 3);
		memcpy(m_pSrvCxt->strXSP, "DSP1", 4);
		memcpy(m_pSrvCxt->strXCS, "DFS", 3);
		memcpy(m_pSrvCxt->strXSF, "DSF", 3);
	}

#ifndef WIN32
	pthread_mutex_init(&m_SendLock, NULL);
	pthread_cond_init(&m_SendCond, NULL);
	pthread_create(&m_hSendThread, NULL, _SendThreadProc, this);
	pthread_detach(m_hSendThread);
#else
	DWORD ThreadID;
	m_SendLock = CreateMutex(NULL, false, NULL);
	m_SendCond = CreateEvent(NULL, false, false, NULL);
	m_hSendThread = CreateThread(NULL, 0, _SendThreadProc, this, NULL, &ThreadID);
#endif
}

void CUdtCore::replyAccept(const char* pstrReply)
{
	m_szReplyfilepath = pstrReply;
}

void CUdtCore::stopTransfer(const int nType)
{
	// 1:stop recv; 2:stop send
	if (nType == 1)
	{
		m_bSendStatus = false;
		UDT::close(m_pClientSocket->sock);
		#ifndef WIN32
			pthread_cond_signal(&m_SendCond);
			pthread_join(m_hSendThread, NULL);
			pthread_mutex_destroy(&m_SendLock);
			pthread_cond_destroy(&m_SendCond);
		#else
			SetEvent(m_SendCond);
			WaitForSingleObject(m_hSendThread, INFINITE);
			CloseHandle(m_hSendThread);
			CloseHandle(m_SendLock);
			CloseHandle(m_SendCond);
		#endif
	}
	else if (nType == 2)
	{
		m_bRecvStatus = false;
		UDT::close(m_pSrvCxt->sock);
		#ifndef WIN32
			pthread_cond_signal(&m_RecvCond);
			pthread_join(m_hRecvThread, NULL);
			pthread_mutex_destroy(&m_RecvLock);
			pthread_cond_destroy(&m_RecvCond);
		#else
			SetEvent(m_RecvCond);
			WaitForSingleObject(m_hRecvThread, INFINITE);
			CloseHandle(m_hRecvThread);
			CloseHandle(m_RecvLock);
			CloseHandle(m_RecvCond);
		#endif
	}
}

void CUdtCore::stopListen()
{
#ifndef WIN32
	pthread_mutex_lock(&m_AcceptLock);
	pthread_cond_signal(&m_AcceptCond);
	pthread_mutex_unlock(&m_AcceptLock);
#else
	SetEvent(m_AcceptCond);
#endif
}

#ifndef WIN32
void * CUdtCore::_ListenThreadProc(void * pParam)
#else
DWORD WINAPI CUdtCore::_ListenThreadProc(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;

	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);

	while (true)
	{
#ifndef WIN32
		pthread_mutex_lock(&pThis->m_AcceptLock);
		timeval now;
		timespec timeout;
		gettimeofday(&now, 0);
		timeout.tv_sec = now.tv_sec + 1;
		timeout.tv_nsec = now.tv_usec * 1000;

		int rc = pthread_cond_timedwait(&pThis->m_AcceptCond, &pThis->m_AcceptLock, &timeout);
		pthread_mutex_unlock(&pThis->m_AcceptLock);
		if (rc != ETIMEDOUT)
		{
			cout << "_ListenThreadProc timeout" << endl;
			break;
		}

#else
		if (WAIT_TIMEOUT != WaitForSingleObject(pThis->m_AcceptCond, 1000))
		{
			cout << "_ListenThreadProc timeout" << endl;
			break;
		}
#endif

		UDTSOCKET sockAccept;
		// accept client connect

		if (UDT::INVALID_SOCK == (sockAccept = UDT::accept(pThis->m_sockListen, (sockaddr*)&clientaddr, &addrlen)))
		{
			cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
			return 0;
		}

		char clienthost[NI_MAXHOST];
		char clientservice[NI_MAXSERV];
		getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
		cout << "new connection: " << clienthost << ":" << clientservice << endl;

		// char clienthost[1025];		// NI_MAXHOST = 1025
		// char clientservice[32];		// NI_MAXSERV = 32
		//	sprintf(clienthost, "%s", inet_ntoa(cli_add.sin_addr));
		//	sprintf(clientservice, "%s", ntohs(cli_add.sin_port));

		_stClientSocket * pCXT = new _stClientSocket;
		memset(pCXT, 0, sizeof(_stClientSocket));
		pCXT->sock = sockAccept;

#ifdef WIN32
		sprintf_s(pCXT->strAddr, "%s", clienthost);
		sprintf_s(pCXT->strPort, "%s", clientservice);
#else
		sprintf(pCXT->strAddr, "%s", clienthost);
		sprintf(pCXT->strPort, "%s", clientservice);
#endif

		// save connext clientSocket
		pThis->m_pClientSocket = pCXT;

#ifndef WIN32
		pthread_mutex_init(&pThis->m_RecvLock, NULL);
		pthread_cond_init(&pThis->m_RecvCond, NULL);
		pthread_create(&pThis->m_hRecvThread, NULL, _RecvThreadProc, pThis);
		pthread_detach(pThis->m_hRecvThread);
#else
		DWORD ThreadID;
		pThis->m_RecvLock = CreateMutex(NULL, false, NULL);
		pThis->m_RecvCond = CreateEvent(NULL, false, false, NULL);
		pThis->m_hRecvThread = CreateThread(NULL, 0, _RecvThreadProc, pThis, 0, &ThreadID);
#endif
	}

	#ifndef WIN32
		return NULL;
	#else
		return 0;
	#endif
}

#ifndef WIN32
void * CUdtCore::_SendThreadProc(void * pParam)
#else
DWORD WINAPI CUdtCore::_SendThreadProc(LPVOID pParam)
#endif
{
	CUdtCore * pThis = (CUdtCore *)pParam;

	_stServerContext * cxt = pThis->m_pSrvCxt;
	int nLen = 0, nPos = 0, nFileCount = 0;
	int64_t nFileTotalSize = 0, nSendSize = 0, iLastPercent = 0;
	string szTmp, szFolderName, szFileName, szFilePath, szFinish = "FAIL", szError = "";
	vector<string> vecFileName;
	vector<string> vecDirName;

	struct addrinfo hints, *local, *peer;
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_socktype = SOCK_DGRAM;

	if (0 != getaddrinfo(NULL, "7777", &hints, &local))
	{
		cout << "incorrect network address.\n" << endl;
		return 0;
	}

	UDTSOCKET fhandle = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
	freeaddrinfo(local);
	if (0 != getaddrinfo(cxt->strAddr, cxt->strPort, &hints, &peer))
	{
		cout << "incorrect server/peer address. " << cxt->strAddr << ":" << cxt->strPort << endl;
		goto Loop;
	}

	// connect to the server, implict bind
	if (UDT::ERROR == UDT::connect(fhandle, peer->ai_addr, peer->ai_addrlen))
		goto Loop;

	freeaddrinfo(peer);

	char Head[8];
	memset(Head, 0, 8);
	memcpy(Head, cxt->strXSR, 3);
	if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 3, 0))
		goto Loop;

	if (memcmp(Head, "TSR", 3) == 0)
	{
		// send text message
		szFileName = cxt->strHostName;
		nLen = szFileName.size();
		if (UDT::ERROR == UDT::send(fhandle, (char*)&nLen, sizeof(int), 0))
			goto Loop;
		if (UDT::ERROR == UDT::send(fhandle, szFileName.c_str(), nLen, 0))
			goto Loop;
		// send message size and information
		int nLen = cxt->vecArray[0].size();
		if (UDT::ERROR == UDT::send(fhandle, (char*)&nLen, sizeof(int), 0))
			goto Loop;
		if (UDT::ERROR == UDT::send(fhandle, cxt->vecArray[0].c_str(), nLen, 0))
			goto Loop;

		szFinish = "";
		goto Loop;
	}
	else if (memcmp(Head, "FSR", 3) == 0 || memcmp(Head, "MSR", 3) == 0)
	{
		vecFileName = cxt->vecArray;
		for (int i = 0; i < vecFileName.size(); i++)
		{
			fstream ifs(cxt->vecArray[i].c_str(), ios::in | ios::binary);
			ifs.seekg(0, ios::end);
			int64_t size = ifs.tellg();
			nFileTotalSize += size;
			nFileCount++;
			ifs.close();
		}
		if (memcmp(Head, "MSR", 3) == 0)
		{
			// send file total size
			if (UDT::ERROR == UDT::send(fhandle, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
				goto Loop;
			// send file count
			if (UDT::ERROR == UDT::send(fhandle, (char*)&nFileCount, sizeof(nFileCount), 0))
				goto Loop;
		}
	}
	else if (memcmp(Head, "DSR", 3) == 0)
	{
		szTmp = cxt->vecArray[0];
		if ('\\' == szTmp[szTmp.size()-1] || '/' == szTmp[szTmp.size()-1])
			cxt->vecArray[0] = szTmp.substr(0, szTmp.size()-1);
		else
			cxt->vecArray[0] = szTmp;
		
		szFolderName = cxt->vecArray[0];
		pThis->SearchFileInDirectroy(szFolderName, nFileTotalSize, vecDirName, vecFileName);
		// send file total size
		if (UDT::ERROR == UDT::send(fhandle, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
			goto Loop;
	}

	// send file name,(filename\hostname\sendtype)
	szFilePath = cxt->vecArray[0];
	nPos = szFilePath.find_last_of('/');
	if (nPos < 0)
	{
		nPos = szFilePath.find_last_of("\\");
	}
	szFileName = szFilePath.substr(nPos+1);
	// add hostname and sendtype
	szTmp = szFileName + "\\" + cxt->strHostName;
	if (0 == strcmp("GENIETURBO", cxt->strSendtype))
	{
		szTmp += "\\";
		szTmp += cxt->strSendtype;
	}
	// send name information of the requested file
	nLen = szTmp.size();
	if (UDT::ERROR == UDT::send(fhandle, (char*)&nLen, sizeof(int), 0))
		goto Loop;
	if (UDT::ERROR == UDT::send(fhandle, szTmp.c_str(), nLen, 0))
		goto Loop;

	// recv accept response
	memset(Head, 0, 8);
	if (UDT::ERROR == UDT::recv(fhandle, (char*)Head, 4, 0))
		goto Loop;
	if (memcmp(Head, cxt->strXSP, 4) != 0)
	{
		szFinish = "REJECT";
		goto Loop;
	}

	// send folder name
	if (memcmp(cxt->strXSR, "DSR", 3) == 0)
	{
		for (int i = 0; i < vecDirName.size(); i++)
		{
			szTmp = vecDirName[i];
			nLen = szFilePath.size() - szFileName.size();
			szTmp = szTmp.substr(nLen);
			// send file tage (DCR)
			memset(Head, 0, 8);
			memcpy(Head, "DCR", 3);
			if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 3, 0))
				goto Loop;

			// send name information of the requested file
			nLen = szTmp.size();
			if (UDT::ERROR == UDT::send(fhandle, (char*)&nLen, sizeof(int), 0))
				goto Loop;
			if (UDT::ERROR == UDT::send(fhandle, szTmp.c_str(), nLen, 0))
				goto Loop;
		}
	}

	pThis->m_bSendStatus = true;
	for (int i = 0; i < vecFileName.size(); i++)
	{
		// send file tage
		memset(Head, 0, 8);
		memcpy(Head, cxt->strXCS, 3);
		if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 3, 0))
			goto Loop;

		if (memcmp(cxt->strXCS, "DFS", 3) == 0)
		{
			szFilePath = vecFileName[i];
			nLen = szFolderName.size() - szFileName.size();
			szTmp = szFilePath.substr(nLen);
		}
		else
		{
			szFilePath = vecFileName[i];
			nPos = szFilePath.find_last_of('/');
			if (nPos < 0)
			{
				nPos = szFilePath.find_last_of("\\");
			}
			szTmp = szFilePath.substr(nPos+1);
		}

		// send filename size and filename
		if (memcmp(cxt->strXSR, "FSR", 3) != 0)
		{
			int nLen = szTmp.size();
			if (UDT::ERROR == UDT::send(fhandle, (char*)&nLen, sizeof(int), 0))
				goto Loop;
			if (UDT::ERROR == UDT::send(fhandle, szTmp.c_str(), nLen, 0))
				goto Loop;
		}

		// open the file
		fstream ifs(szFilePath.c_str(), ios::in | ios::binary);
		ifs.seekg(0, ios::end);
		int64_t size = ifs.tellg();
		ifs.seekg(0, ios::beg);
		// send file size information
		if (UDT::ERROR == UDT::send(fhandle, (char*)&size, sizeof(int64_t), 0))
			goto Loop;

		// send the file
		int64_t nOffset = 0;
		int64_t left = size;
		while (true)
		{
			CGuard::enterCS(pThis->m_SendLock);
			if (!pThis->m_bSendStatus)
				goto Loop;
			CGuard::leaveCS(pThis->m_SendLock);

			// check network states
			//	UDTSTATUS states = UDT::getStatus(fhandle);
			//	if (states != UDTSTATUS::CONNECTED)
			//		goto Loop;

			int64_t send = 0;
			if (left > 51200)
				send = UDT::sendfile(fhandle, ifs, nOffset, 51200);
			else
				send = UDT::sendfile(fhandle, ifs, nOffset, left);

			if (UDT::ERROR == send)
				goto Loop;

			left -= send;
		//	nOffset += send;
			nSendSize += send;
			int64_t iPercent = (nSendSize*100)/nFileTotalSize;
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
				// recv response（FCS）
				iLastPercent = iPercent;
				memset(Head, 0, 8);
				if (UDT::ERROR == UDT::recv(fhandle, (char*)Head, 4, 0))
					goto Loop;
			}

			pThis->m_pCallBack->onSendTransfer(nFileTotalSize, nSendSize, szFileName.c_str());
			if (left <= 0)
				break;
		}
		ifs.close();
	}

	if (memcmp(cxt->strXSR, "MSR", 3) == 0 || memcmp(cxt->strXSR, "DSR", 3) == 0)
	{
		memset(Head, 0, 8);
		memcpy(Head, cxt->strXSF, 3);
		if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 3, 0))
			goto Loop;
	}

	szFinish = "SUCCESS";
	goto Loop;

	// goto loop for end
Loop:
	if (!szFinish.empty())
	{
		pThis->m_pCallBack->onSendFinished(szFinish.c_str());
	}

	UDT::close(fhandle);
	delete cxt;

	#ifndef WIN32
		return NULL;
	#else
		return 0;
	#endif
}

#ifndef WIN32
void * CUdtCore::_RecvThreadProc(void * pParam)
#else
DWORD WINAPI CUdtCore::_RecvThreadProc(LPVOID pParam)
#endif
{
	//fstream log("/mnt/sdcard/UdtLog.txt", ios::out | ios::binary | ios::trunc);
	//log << "***_RecvThreadProc***" << endl;

	CUdtCore * pThis = (CUdtCore *)pParam;

	UDTSOCKET fhandle = pThis->m_pClientSocket->sock;

	char Head[8];
	int nLen = 0, nPos = 0, nCount = 0;
	int64_t nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0;
	vector<string> vecFileName;
	string szTmp, strReplyPath = "", szFinish = "FAIL", szError = "", szFilePath = "/mnt/sdcard/";
	string szHostName, szFileName, szSendType = "UNKNOWN";

	while (true)
	{
		memset(Head, 0, 8);
		if (UDT::ERROR == UDT::recv(fhandle, (char *)Head, 3, 0))
			goto Loop;

		if (memcmp(Head,"TSR",3) == 0)	// 1.	recv message response（TSR）
		{
			// recv hostname
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrHostName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrHostName, nLen, 0))
				goto Loop;
			pstrHostName[nLen] = '\0';

			// recv message
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrMsg = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrMsg, nLen, 0))
				goto Loop;
			pstrMsg[nLen] = '\0';

			// notify to up
			pThis->m_pCallBack->onRecvMessage((char*)pThis->m_pClientSocket->strAddr, pstrHostName, pstrMsg);

			szFinish = "";
			goto Loop;
		}
		else if (memcmp(Head,"FSR",3) == 0 || memcmp(Head,"MSR",3) == 0 || memcmp(Head,"DSR",3) == 0)
		{
			if (memcmp(Head, "MSR", 3) == 0 || memcmp(Head, "DSR", 3) == 0)
			{
				// recv file total size, and file count
				if (UDT::ERROR == UDT::recv(fhandle, (char*)&nFileTotalSize, sizeof(nFileTotalSize), 0))
					goto Loop;
				if (memcmp(Head, "MSR", 3) == 0)
				{
					if (UDT::ERROR == UDT::recv(fhandle, (char*)&nCount, sizeof(nCount), 0))
						goto Loop;
				}
			}
			// recv filename hostname sendtype
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';

			szTmp = pstrFileName;
			int nPos = szTmp.find_last_of('\\');
			if (nPos > 0)
			{
				int iFirstPos = szTmp.find_first_of('\\');
				if (nPos == iFirstPos)
				{
					szHostName = szTmp.substr(nPos+1);
					szFileName = szTmp.substr(0,nPos);
				}
				else
				{
					szHostName = szTmp.substr(iFirstPos+1,nPos-iFirstPos-1);
					szFileName = szTmp.substr(0,iFirstPos);
					szSendType = szTmp.substr(nPos+1);
				}
			}
			else
				goto Loop;

			pThis->m_szReplyfilepath = "";
			if (memcmp(Head, "FSR", 3) == 0)
			{
				pThis->m_pCallBack->onAccept((char*)pThis->m_pClientSocket->strAddr, szHostName.c_str(), szSendType.c_str(), szFileName.c_str(), 1);
			}
			else
			{
				pThis->m_pCallBack->onAcceptFolder((char*)pThis->m_pClientSocket->strAddr, szHostName.c_str(), szSendType.c_str(), szFileName.c_str(), nCount);
			}

			int nWaitTime = 0;
			while(nWaitTime < 31)
			{
				CGuard::enterCS(pThis->m_RecvLock);
				strReplyPath = pThis->m_szReplyfilepath;
				CGuard::leaveCS(pThis->m_RecvLock);
				if (!strReplyPath.empty())
				{
					cout << "getReply:" << strReplyPath << endl;
					break;
				}

				nWaitTime += 1;
				#ifndef WIN32
					sleep(1);
				#else
					Sleep(1000);
				#endif
			}

			// file transfer response(FSP0/MSP0/DSP0)
			Head[2] = 'P';
			if (strReplyPath.compare("REJECT")==0)
				Head[3] = '0';
			else if (strReplyPath.compare("REJECTBUSY") == 0 || strReplyPath.empty())
				Head[3] = '2';
			else
				Head[3] = '1';

			if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 4, 0))
				goto Loop;
			if (Head[3] != '1')
			{
				szFinish = "";
				goto Loop;
			}
		}
		else if (memcmp(Head,"DCR",3) == 0)
		{
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
				goto Loop;
			char * pstrFileName = new char[nLen+2];
			if (UDT::ERROR == UDT::recv(fhandle, pstrFileName, nLen, 0))
				goto Loop;
			pstrFileName[nLen] = '\0';

			string szFolder = strReplyPath + pstrFileName;
			pThis->CreateDirectroy(szFolder);
		}
		else if (memcmp(Head,"FCS",3) == 0 || memcmp(Head,"MCS",3) == 0 || memcmp(Head,"DFS",3) == 0)
		{
			if (memcmp(Head, "MCS", 3) == 0 || memcmp(Head, "DFS", 3) == 0)
			{
				if (UDT::ERROR == UDT::recv(fhandle, (char*)&nLen, sizeof(nLen), 0))
					goto Loop;
				char * pstrFileName = new char[nLen+2];
				if (UDT::ERROR == UDT::recv(fhandle, pstrFileName, nLen, 0))
					goto Loop;
				pstrFileName[nLen] = '\0';
				szFileName = pstrFileName;
				vecFileName.push_back(szFileName);
			}

			szFilePath = strReplyPath + szFileName;
			string szSlash = "";

			do 
			{
#ifdef WIN32
				szSlash = '\\';
				nPos = szFilePath.find('/', 0);
				if (nPos >= 0)
				{
					szFilePath.replace(nPos, 1, szSlash);
				}
#else
				szSlash = '/';
				nPos = szFilePath.find('\\', 0);
				if (nPos >= 0)
				{
					szFilePath.replace(nPos, 1, szSlash);
				}
#endif
			}while (nPos >= 0);

			int64_t size;
			if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))
				goto Loop;
			if (memcmp(Head, "FCS", 3) == 0)
				nFileTotalSize = size;

			//log << "FileName:" << szFilePath << ", TotalSize:" << nFileTotalSize << endl;

			pThis->m_bRecvStatus = true;
			// receive the file
			fstream ofs(szFilePath.c_str(), ios::out | ios::binary | ios::trunc);
			int64_t offset = 0;
			int64_t left = size;
			while(true)
			{
				CGuard::enterCS(pThis->m_SendLock);
				if (!pThis->m_bRecvStatus)
					break;
				CGuard::leaveCS(pThis->m_SendLock);

				int64_t recv = 0;
				if (left > 51200)
					recv = UDT::recvfile(fhandle, ofs, offset, 51200);
				else
					recv = UDT::recvfile(fhandle, ofs, offset, left);

				if (UDT::ERROR == recv)
					goto Loop;

				left -= recv;
			//	offset +=recv;
				nRecvSize += recv;
				// 发送文件接收进度
				int64_t iPercent = (nRecvSize*100)/nFileTotalSize;
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
					iLastPercent = iPercent;
					if (UDT::ERROR == UDT::send(fhandle, (char*)Head, 4, 0))
						goto Loop;
				}

				if (recv > 0)
				{
					pThis->m_pCallBack->onRecvTransfer((long)nFileTotalSize, nRecvSize, szFileName.c_str());
				}

				if (left <= 0)
					break;
			}

			if (memcmp(Head, "FCS", 3) == 0)
			{
				szFinish = "SUCCESS";
				goto Loop;
			}
		}
		else if (memcmp(Head,"MSF",3) == 0 || memcmp(Head,"DSF",3) == 0)
		{
			pThis->m_pCallBack->onAcceptonFinish((char*)pThis->m_pClientSocket->strAddr, vecFileName);
			szFinish = "SUCCESS";
			goto Loop;
		}
		else
			goto Loop;
	}

	// goto loop for end
Loop:
	// SUCCESS, FAIL
	if (!szFinish.empty())
	{
		pThis->m_pCallBack->onRecvFinished(szFinish.c_str());
	}

	UDT::close(fhandle);

	#ifndef WIN32
		return NULL;
	#else
		return 0;
	#endif
}


//////////////////////////////////////////////////////////////////////////
// private method
void CUdtCore::SearchFileInDirectroy(const std::string & szPath, int64_t & nTotalSize, std::vector<std::string> & vecDirName, std::vector<std::string> & vecFileName)
{
	char strPath[256] = {0};
	memcpy(strPath, szPath.c_str(), 256);

#ifdef WIN32
	WIN32_FIND_DATA sFindFileData = {0};
	HANDLE hFind = INVALID_HANDLE_VALUE;
	hFind = FindFirstFile(strPath, &sFindFileData);
	if (sFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		vecDirName.push_back(strPath);
		if ('/' == strPath[strlen(strPath)-1])
		{
			strcat_s(strPath, ("/*.*"));
		}
		else
			strcat_s(strPath, ("/*.*"));

		hFind = INVALID_HANDLE_VALUE;
		if (INVALID_HANDLE_VALUE != (hFind = FindFirstFile(strPath, &sFindFileData)))
		{
			do 
			{
				if (sFindFileData.cFileName[0] == '.')
					continue;
				sprintf_s(strPath, "%s/%s", szPath.c_str(), sFindFileData.cFileName);
				if (sFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					SearchFileInDirectroy(strPath, nTotalSize, vecDirName, vecFileName);
				}
				else
				{
					fstream ifs(strPath, ios::in | ios::binary);
					ifs.seekg(0, ios::end);
					int64_t size = ifs.tellg();
					nTotalSize += size;
					vecFileName.push_back(strPath);
				}
			} while (FindNextFile(hFind, &sFindFileData));

			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}
	else
	{
		fstream ifs(strPath, ios::in | ios::binary);
		ifs.seekg(0, ios::end);
		int64_t size = ifs.tellg();
		nTotalSize += size;
		vecFileName.push_back(strPath);

		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}
#else
	DIR * pDir;
	if ((pDir = opendir(strPath)) == NULL)
	{
		fstream ifs(strPath, ios::in | ios::binary);
		ifs.seekg(0, ios::end);
		int64_t size = ifs.tellg();
		nTotalSize += size;
		vecFileName.push_back(strPath);
		return ;
	}

	struct stat dirInfo;
	struct dirent * pDT;
	char strFileName[256];
	memset(strFileName, 0, sizeof(strFileName));
	vecDirName.push_back(strPath);
	while ((pDT = readdir(pDir)))
	{
		if (strcmp(pDT->d_name, ".") == 0 || strcmp(pDT->d_name, "..") == 0)
			continue;
		else
		{
			sprintf(strFileName, "%s/%s", strPath, pDT->d_name);
			struct stat buf;
			if (lstat(strFileName, &buf) >= 0 && S_ISDIR(buf.st_mode))
			{
				SearchFileInDirectroy(strFileName, nTotalSize, vecDirName, vecFileName);
			}
			else
			{
				fstream ifs(strFileName, ios::in | ios::binary);
				ifs.seekg(0, ios::end);
				int64_t size = ifs.tellg();
				nTotalSize += size;
				vecFileName.push_back(strFileName);
			}
		}
		memset(strFileName, 0, 256);
	}
	closedir(pDir);
#endif
}

void CUdtCore::CreateDirectroy(const std::string & szPath)
{
	int nLen = 0, nPos = 0;
	string szFolder = "", szSlash = "";
	string szTmpPath = szPath;
	// WIN32路径分隔符换成："\", LINUX：'/'
	do 
	{
#ifdef WIN32
		szSlash = '\\';
		nPos = szTmpPath.find('/', 0);
		if (nPos >= 0)
		{
			szTmpPath.replace(nPos, 1, szSlash);
		}
#else
		szSlash = '/';
		nPos = szTmpPath.find('\\', 0);
		if (nPos >= 0)
		{
			szTmpPath.replace(nPos, 1, szSlash);
		}
#endif
	} while (nPos >= 0);

	if ('\\' != szTmpPath[szTmpPath.size()-1] || '/' != szTmpPath[szTmpPath.size()-1])
	{
		szTmpPath += szSlash;
	}
	string szTmp = szTmpPath;
	while (true)
	{
		int nPos = szTmp.find_first_of(szSlash);
		if (nPos >= 0)
		{
			nLen += nPos;
			szTmp = szTmp.substr(nPos+1);
			szFolder = szTmpPath.substr(0, nLen);
			nPos = szTmp.find_first_of(szSlash);
			if (nPos >= 0)
			{
				nLen = nLen + 1 + nPos;
				szTmp = szTmp.substr(nPos);
				szFolder = szTmpPath.substr(0, nLen);
#ifdef WIN32
				if (_access(szFolder.c_str(), 0))
					_mkdir(szFolder.c_str());
#else
				if (access(szFolder.c_str(), F_OK) != 0)
					mkdir(szFolder.c_str(),770);
#endif
			}
			else
				break;
		}
		else
			break;
	}
}
