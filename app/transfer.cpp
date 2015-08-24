#include "transfer.h"
#include <iostream>


using namespace std;

//////////////////////////////////////////////////////////////////////////
// TurboTransfer


TurboTransfer::TurboTransfer(TurboListener* pListener)
	: m_pListener(pListener)
	, m_TaskManager(NULL)
	, m_bListening(false)
	, m_bTransfering(false)
	, m_nPktSize(268)
	, SND_BLOCK(1024 * 100)
{
	// use this function to initialize the UDT library
	UDT::startup();
}

TurboTransfer::~TurboTransfer()
{
	// use this function to release the UDT library
	UDT::cleanup();
}

void TurboTransfer::StartServer(const char* host, const char* port)
{
	if (m_bListening) return ;

	m_Port = port;
	m_FileSavePath = "D:\\Turbo\\";

	CLIENTCONTEXT* clip = new CLIENTCONTEXT();
	clip->type = OP_SRV_LISTEN;

	if (!m_TaskManager) m_TaskManager = new CThreadManager();
	m_TaskManager->StartTask(new TurboTransferServerTask(this, 1000.0, clip));
	m_bListening = true;
}

void TurboTransfer::StopServer()
{
	if (!m_bListening) return ;

	m_bListening = false;
	UDT::close(m_sockListen);

	if (m_TaskManager)
	{
		m_TaskManager->StopAllTasks();
		delete m_TaskManager;
	}
	m_TaskManager = NULL;
}

void TurboTransfer::FileTransfer(const char* host, int port, const char* filepath, int fileoffset, int type)
{
	char strPort[8] = {0};
	sprintf(strPort, "%d", port);

	CLIENTCONTEXT* pClip = new CLIENTCONTEXT();
	pClip->host = host;
	pClip->port = strPort;
	pClip->filepath = filepath;
	pClip->startoffset = fileoffset;
	pClip->type = OP_SND_FILE;

	if (!m_TaskManager) m_TaskManager = new CThreadManager();
	m_TaskManager->StartTask(new TurboTransferClientTask(this, 1000.0, pClip));
}

void TurboTransfer::PauseTransfer(const char* filepath, int fileoffset, int type)
{

}

void TurboTransfer::ReplyAccept(const UDTSOCKET sock, const char* reply)
{

}

void TurboTransfer::Worker(CLIENTCONTEXT* clip)
{
	if (!clip) return ;
	int nResult = 200;

	if (clip->type == OP_SND_FILE)
	{
		printf("TurboTransfer::ProcessSendFile.....\n");
		nResult = ProcessSendFile(clip);
	}
	else if (clip->type == OP_RCV_FILE)
	{
		printf("TurboTransfer::ProcessRecvFile.....\n");
		nResult = ProcessRecvFile(clip);
	}
	else if (clip->type == OP_SRV_LISTEN)
	{
		printf("TurboTransfer::ProcessSrvListen.....\n");
		if (m_bListening)
			nResult = ServerListener(m_sockListen, NULL, m_Port.c_str());
	}

	if ( nResult == 550 || nResult == 551) UDT::close(clip->sock);
	if (m_pListener) m_pListener->OnFinished("OnFineshed", nResult);
}

int TurboTransfer::ServerListener(UDTSOCKET& serv, const char* host, const char* port)
{
	addrinfo hints, * res;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (0 != getaddrinfo(NULL, port, &hints, &res))
	{
		cout << "illegal port number or port is busy.\n" << endl;
		return 0;
	}

	serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// Windows UDP issue
	// For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
	int mss = 1052;
	UDT::setsockopt(serv, 0, UDT_MSS, &mss, sizeof(int));
#endif

	if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
	{
		cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
		return 0;
	}

	freeaddrinfo(res);

	cout << "server is ready at port: " << port << endl;

	UDT::listen(serv, 10);

	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);

	UDTSOCKET fhandle;

	while (true)
	{
		if (UDT::INVALID_SOCK == (fhandle = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen)))
		{
			cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
			return 0;
		}

		char clienthost[NI_MAXHOST];
		char clientservice[NI_MAXSERV];
		getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
		cout << "new connection: " << clienthost << ":" << clientservice << endl;

		CLIENTCONTEXT* clip = new CLIENTCONTEXT();
		clip->sock = fhandle;
		clip->type = OP_RCV_FILE;
		m_TaskManager->StartTask(new TurboTransferClientTask(this, 1000.0, clip));
	}

	UDT::close(serv);

	return 0;
}

int TurboTransfer::ClientConnect(UDTSOCKET& sock, const char* host, const char* port)
{
	struct addrinfo hints, *peer;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	sock = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);

	if (0 != getaddrinfo(host, port, &hints, &peer))
	{
		cout << "incorrect server/peer address. " << host << ":" << port << endl;
		return -1;
	}

	// connect to the server, implict bind
	if (UDT::ERROR == UDT::connect(sock, peer->ai_addr, peer->ai_addrlen))
	{
		cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
		return -1;
	}

	freeaddrinfo(peer);

	return 0;
}

int TurboTransfer::ProcessSendFile(CLIENTCONTEXT* pClip)
{
	char cmd[512];
	int nLen = 0, nPos = 0, nFileCount = 0, nFileSize, nResult = 200;
	int64_t nFileTotalSize = 0, nSendSize = 0;
	string szTmp, szFolder, szAccept, szFileName, szFilePath, szFinish = "NETFAIL", szError = "";
	bool bConnect =false;
	UDTSOCKET sock;

	// connect 3 times
	for (int i =0; i < 3; i++)
	{
		if (ClientConnect(sock, pClip->host.c_str(), pClip->port.c_str()) != 0)
		{
			HelperSystem::Sleep(TimeStamp(1.0));
			continue;
		}
		bConnect = true;
	}
	if (!bConnect)
		return 550;

	printf("connect success....\n");

	szFilePath = pClip->filepath;
	nPos = szFilePath.find_last_of('/');
	if (nPos < 0)
	{
		nPos = szFilePath.find_last_of("\\");
	}
	szFileName = szFilePath.substr(nPos+1);

	try
	{
		// open the file
		fstream ifs(szFilePath.c_str(), ios::in | ios::binary);
		ifs.seekg(0, ios::end);
		nFileSize = ifs.tellg();
		ifs.seekg(0, ios::beg);
		int64_t nOffset = 0, left = nFileSize;
		nLen = szFileName.size();

		memset(cmd, 0, 8);
		memcpy(cmd, "STOR", 4);
		memcpy(cmd+4, &nFileSize, sizeof(nFileSize));
		memcpy(cmd+8, szFileName.c_str(), nLen);
		cmd[8+nLen] = '\0';

		if (UDT::ERROR == UDT::send(sock, (char*)&cmd, 268, 0))
			return 551;
		if (UDT::ERROR == UDT::recv(sock, (char*)&nResult, sizeof(int), 0))
			return 551;
		if (nResult != 200)
			return 552;

		while (left > 0)
		{
			int64_t send = 0;
			if (left > SND_BLOCK)
				send = UDT::sendfile(sock, ifs, nOffset, SND_BLOCK);
			else
				send = UDT::sendfile(sock, ifs, nOffset, left);

			if (UDT::ERROR == send)
			{
				nResult = 551;
				break;
			}

			left -= send;
			nSendSize += send;
			m_pListener->OnTransfer(szFileName.c_str(), nFileSize, nSendSize, 1);
			printf("Send size:%ld\n", nSendSize);
		}
		ifs.close();

		if (nResult != 551) nResult = 250;
		m_pListener->OnFinished(szFileName.c_str(), nResult);
	}
	catch (...)
	{
		nResult = 450;
	}

	memset(cmd, 0, 8);
	memcpy(cmd, "QUIT", 4);
	if (UDT::ERROR == UDT::send(sock, (char*)cmd, 4, 0))
		return 551;

	return 0;
}

int TurboTransfer::ProcessRecvFile(CLIENTCONTEXT* pClip)
{
	UDTSOCKET sock = pClip->sock;
	char cmd[512];
	int nResult = 108, nLen = 0, nRet = 0, nPos = 0, nCount = 0, nFileSize = 0;
	int64_t nFileTotalSize = 0, nRecvSize = 0, iLastPercent = 0; 
	string szTmp, szTmp2, strReplyPath = "", szFinish = "NETFAIL", szSlash = "", szFilePath = "", szReFileName = "", szFolder = "";
	string szHostName, szFileName, szRecdevice, szRectype, szOwndevice, szOwntype, szSendType, szFileType;

	while (true)
	{
		memset(cmd, 0, 512);
		if (UDT::ERROR == UDT::recv(sock, (char *)cmd, 4, 0))
			return 108;
		if (memcmp(cmd,"STOR", 4) == 0)
		{
			if (UDT::ERROR == UDT::recv(sock, (char *)&nFileSize, sizeof(int), 0))
				return 108;
			memset(cmd, 0, 512);
			if (UDT::ERROR == UDT::recv(sock, (char *)cmd, 260, 0))
				return 108;

			szFilePath = m_FileSavePath;
			szFileName = cmd;
			szFilePath += szFileName;
			fstream ofs(szFilePath.c_str(), ios::out | ios::binary | ios::trunc);
			int64_t recvsize = 0;
			int64_t offset = 0, left = nFileSize;

			nResult = 200;
			if (UDT::ERROR == UDT::send(sock, (char*)&nResult, sizeof(int), 0))
				return 108;

			while (left > 0)
			{
				int64_t recv = 0;
				if (left > SND_BLOCK)
					recv = UDT::recvfile(sock, ofs, offset, SND_BLOCK);
				else
					recv = UDT::recvfile(sock, ofs, offset, left);
				if (UDT::ERROR == recv)
				{
					printf("Receive failed!\n");
					nResult = 108;
					break;
				}
				left -= recv;
				recvsize += recv;
				printf("%s Recv size:%ld\n", szFilePath.c_str(), recvsize);
			}

			printf("File receive finished:%s\n", szFilePath.c_str());
			// close file
			ofs.close();
		}
		else if (memcmp(cmd,"QUIT",4) == 0)
		{
			break;
		}
	}

	UDT::close(sock);

	return 0;
}


//////////////////////////////////////////////////////////////////////////
// TurboTransferTask
TurboTransferServerTask::TurboTransferServerTask(TurboTransfer* turbo, TimeStamp delay /* = TimeStamp */, CLIENTCONTEXT* clip /* = OP_NULL */)
	: m_turbo(turbo)
	, m_Delay(delay)
	, m_Clip(clip)
{
}

TurboTransferServerTask::~TurboTransferServerTask()
{
}

void TurboTransferServerTask::DoRun()
{
	while (!IsAborting(m_Delay))
	{
		if (m_turbo) m_turbo->Worker(m_Clip);
	}

	delete m_Clip;
	m_Clip = NULL;
	printf("DoRun exit.....\n");
}


//////////////////////////////////////////////////////////////////////////
// TurboTransferTask
TurboTransferClientTask::TurboTransferClientTask(TurboTransfer* turbo, TimeStamp delay /* = 500 */, CLIENTCONTEXT* clip /* = OP_NULL */)
	: m_turbo(turbo)
	, m_Delay(delay)
	, m_Clip(clip)
{
}

TurboTransferClientTask::~TurboTransferClientTask()
{
}

void TurboTransferClientTask::DoRun()
{
	//while (!IsAborting(m_Delay))
	{
		if (m_turbo) m_turbo->Worker(m_Clip);
		Stop(true);
	}

	delete m_Clip;
	m_Clip = NULL;
	printf("DoRun exit.....\n");
}