#include "api.h"
#include <re.h>

//////////////////////////////////////////////////////////////////////////
// CUDTSocket
CUDTSocket::CUDTSocket():
m_Status(INIT), 
	m_TimeStamp(0), 
	m_iIPversion(0), 
	m_pSelfAddr(NULL), 
	m_pPeerAddr(NULL), 
	m_SocketID(0), 
	m_ListenSocket(0), 
	m_PeerID(0), 
	m_iISN(0), 
	m_pUDT(NULL), 
	m_pQueuedSockets(NULL), 
	m_pAcceptSockets(NULL), 
	m_AcceptCond(), 
	m_AcceptLock(), 
	m_uiBackLog(0), 
	m_iMuxID(-1)
{
#ifndef WIN32
	pthread_mutex_init(&m_AcceptLock, NULL);
	pthread_cond_init(&m_AcceptCond, NULL);
	pthread_mutex_init(&m_ControlLock, NULL);
#else
	m_AcceptLock = CreateMutex(NULL, false, NULL);
	m_AcceptCond = CreateEvent(NULL, false, false, NULL);
	m_ControlLock = CreateMutex(NULL, false, NULL);
#endif
}

CUDTSocket::~CUDTSocket()
{
	if (AF_INET == m_iIPversion)
	{
		delete (sockaddr_in*)m_pSelfAddr;
		delete (sockaddr_in*)m_pPeerAddr;
	}
	else
	{
		delete (sockaddr_in6*)m_pSelfAddr;
		delete (sockaddr_in6*)m_pPeerAddr;
	}

	delete m_pUDT;
	m_pUDT = NULL;

	delete m_pQueuedSockets;
	delete m_pAcceptSockets;

#ifndef WIN32
	pthread_mutex_destroy(&m_AcceptLock);
	pthread_cond_destroy(&m_AcceptCond);
	pthread_mutex_destroy(&m_ControlLock);
#else
	CloseHandle(m_AcceptLock);
	CloseHandle(m_AcceptCond);
	CloseHandle(m_ControlLock);
#endif
}


//////////////////////////////////////////////////////////////////////////
// CP2PUnited
CUDTUnited::CUDTUnited():
m_Sockets(), 
	m_ControlLock(), 
	m_IDLock(), 
	m_SocketID(0),
	m_pCache(NULL),
	m_bClosing(false),
	m_GCStopLock(),
	m_GCStopCond(),
	m_InitLock(),
	m_iInstanceCount(0),
	m_bGCStatus(false),
	m_WorkerThread(),
	m_ClosedSockets()
{
	// Socket ID MUST start from a random value
	srand((unsigned int)CTimer::getTime());
	m_SocketID = 1 + (int)((1 << 30) * (double(rand()) / RAND_MAX));

#ifndef WIN32
	pthread_mutex_init(&m_ControlLock, NULL);
	pthread_mutex_init(&m_IDLock, NULL);
	pthread_mutex_init(&m_InitLock, NULL);
#else
	m_ControlLock = CreateMutex(NULL, false, NULL);
	m_IDLock = CreateMutex(NULL, false, NULL);
	m_InitLock = CreateMutex(NULL, false, NULL);
#endif

	m_pCache = new CCache<CInfoBlock>;
}

CUDTUnited::~CUDTUnited()
{
#ifndef WIN32
	pthread_mutex_destroy(&m_ControlLock);
	pthread_mutex_destroy(&m_IDLock);
	pthread_mutex_destroy(&m_InitLock);
#else
	CloseHandle(m_ControlLock);
	CloseHandle(m_IDLock);
	CloseHandle(m_InitLock);
#endif

	delete m_pCache;
}

int CUDTUnited::startup(const char* ctlip, const uint16_t ctlport, const char* stunip, const uint16_t stunport, const char* name, const char* passwd, p2p_init_h* inith, p2p_request_h* requesth)
{
	CGuard gcinit(m_InitLock);

	if (m_iInstanceCount++ > 0)
		return 0;

	if (m_bGCStatus)
		return 0;

	p2p_init(&m_p2ph, ctlip, ctlport, stunip, stunport, name, passwd, p2p_init_handler, p2p_request_handler);

	m_bClosing = false;
#ifndef WIN32
	pthread_mutex_init(&m_GCStopLock, NULL);
	pthread_cond_init(&m_GCStopCond, NULL);
	pthread_create(&m_WorkerThread, NULL, worker, this);
#else
	m_GCStopLock = CreateMutex(NULL, false, NULL);
	m_GCStopCond = CreateEvent(NULL, false, false, NULL);
	m_WorkerThread = CreateThread(NULL, 0, worker, this, 0, NULL);
#endif

	//m_p2ph = p;
	m_bGCStatus = true;

	return 0;
}

int CUDTUnited::p2pAccept()
{
	return 0;
}

UDTSOCKET CUDTUnited::p2pConnect(const UDTSOCKET u, const char* peername)
{
	struct p2pconnection* p2pcon = NULL;
	p2p_connect(m_p2ph, &p2pcon, peername, p2p_connect_handler, p2p_receive_handler, p2pcon);
	return 0;
}

int CUDTUnited::p2pSend(const UDTSOCKET u, const char* peername, const char* buf, int len)
{
	return 0;
}

int CUDTUnited::p2pDisconnect(const char* peername)
{
	return 0;
}

int CUDTUnited::p2pClose()
{
	//libre_close();
	return 0;
}

int CUDTUnited::bind(const UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	CGuard cg(s->m_ControlLock);

	// cannot bind a socket more than once
	if (INIT != s->m_Status)
		throw CUDTException(5, 0, 0);

	// check the size of SOCKADDR structure
	if (AF_INET == s->m_iIPversion)
	{
		if (namelen != sizeof(sockaddr_in))
			throw CUDTException(5, 3, 0);
	}
	else
	{
		if (namelen != sizeof(sockaddr_in6))
			throw CUDTException(5, 3, 0);
	}

	s->m_pUDT->open();
	updateMux(s, name, udpsaock);
	s->m_Status = OPENED;

	// copy address information of local node
	s->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(s->m_pSelfAddr);

	return 0;
}

int CUDTUnited::listen(const UDTSOCKET u, int backlog)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	CGuard cg(s->m_ControlLock);

	// do nothing if the socket is already listening
	if (LISTENING == s->m_Status)
		return 0;

	// a socket can listen only if is in OPENED status
	if (OPENED != s->m_Status)
		throw CUDTException(5, 5, 0);

	// listen is not supported in rendezvous connection setup
	if (s->m_pUDT->m_bRendezvous)
		throw CUDTException(5, 7, 0);

	if (backlog <= 0)
		throw CUDTException(5, 3, 0);

	s->m_uiBackLog = backlog;

	try
	{
		s->m_pQueuedSockets = new std::set<UDTSOCKET>;
		s->m_pAcceptSockets = new std::set<UDTSOCKET>;
	}
	catch (...)
	{
		delete s->m_pQueuedSockets;
		delete s->m_pAcceptSockets;
		throw CUDTException(3, 2, 0);
	}

	s->m_pUDT->listen();

	s->m_Status = LISTENING;

	return 0;
}

UDTSOCKET CUDTUnited::newSocket(int af, int type)
{
	if ((type != SOCK_STREAM) && (type != SOCK_DGRAM))
		throw CUDTException(5, 3, 0);

	CUDTSocket* ns = NULL;

	try
	{
		ns = new CUDTSocket;
		ns->m_pUDT = new CUDT;
		if (AF_INET == af)
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in);
			((sockaddr_in*)(ns->m_pSelfAddr))->sin_port = 0;
		}
		else
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in6);
			((sockaddr_in6*)(ns->m_pSelfAddr))->sin6_port = 0;
		}
	}
	catch (...)
	{
		delete ns;
		throw CUDTException(3, 2, 0);
	}

	CGuard::enterCS(m_IDLock);
	ns->m_SocketID = -- m_SocketID;
	CGuard::leaveCS(m_IDLock);

	ns->m_Status = INIT;
	ns->m_ListenSocket = 0;
	ns->m_pUDT->m_SocketID = ns->m_SocketID;
	ns->m_pUDT->m_iSockType = (SOCK_STREAM == type) ? UDT_STREAM : UDT_DGRAM;
	ns->m_pUDT->m_iIPversion = ns->m_iIPversion = af;
	ns->m_pUDT->m_pCache = m_pCache;

	// protect the m_Sockets structure.
	CGuard::enterCS(m_ControlLock);
	try
	{
		m_Sockets[ns->m_SocketID] = ns;
	}
	catch (...)
	{
		//failure and rollback
		CGuard::leaveCS(m_ControlLock);
		delete ns;
		ns = NULL;
	}
	CGuard::leaveCS(m_ControlLock);

	if (NULL == ns)
		throw CUDTException(3, 2, 0);

	return ns->m_SocketID;
}

int CUDTUnited::newConnection(const UDTSOCKET listen, const sockaddr* peer, CHandShake* hs)
{
	CUDTSocket* ls = locate(listen);
	if (NULL == ls)
		return -1;

	CUDTSocket* ns = NULL;

	// if this connection has already been processed
	if (ns->m_pUDT->m_bBroken)
	{
		// last connection from the "peer" address has been broken
		ns->m_Status = CLOSED;
		ns->m_TimeStamp = CTimer::getTime();

		CGuard::enterCS(ls->m_AcceptLock);
		ls->m_pQueuedSockets->erase(ns->m_SocketID);
		ls->m_pAcceptSockets->erase(ns->m_SocketID);
		CGuard::leaveCS(ls->m_AcceptLock);
	}
	else
	{
		// connection already exist, this is a repeated connection request
		// respond with existing HS information

		hs->m_iISN = ns->m_pUDT->m_iISN;
		hs->m_iMSS = ns->m_pUDT->m_iMSS;
		hs->m_iFlightFlagSize = ns->m_pUDT->m_iFlightFlagSize;
		hs->m_iReqType = -1;
		hs->m_iID = ns->m_SocketID;

		return 0;

		//except for this situation a new connection should be started
	}

	// exceeding backlog, refuse the connection request
	if (ls->m_pQueuedSockets->size() >= ls->m_uiBackLog)
		return -1;

	try
	{
		ns = new CUDTSocket;
		ns->m_pUDT = new CUDT(*(ls->m_pUDT));
		if (AF_INET == ls->m_iIPversion)
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in);
			((sockaddr_in*)(ns->m_pSelfAddr))->sin_port = 0;
			ns->m_pPeerAddr = (sockaddr*)(new sockaddr_in);
			memcpy(ns->m_pPeerAddr, peer, sizeof(sockaddr_in));
		}
		else
		{
			ns->m_pSelfAddr = (sockaddr*)(new sockaddr_in6);
			((sockaddr_in6*)(ns->m_pSelfAddr))->sin6_port = 0;
			ns->m_pPeerAddr = (sockaddr*)(new sockaddr_in6);
			memcpy(ns->m_pPeerAddr, peer, sizeof(sockaddr_in6));
		}
	}
	catch (...)
	{
		delete ns;
		return -1;
	}

	CGuard::enterCS(m_IDLock);
	ns->m_SocketID = -- m_SocketID;
	CGuard::leaveCS(m_IDLock);

	ns->m_ListenSocket = listen;
	ns->m_iIPversion = ls->m_iIPversion;
	ns->m_pUDT->m_SocketID = ns->m_SocketID;
	ns->m_PeerID = hs->m_iID;
	ns->m_iISN = hs->m_iISN;

	int error = 0;

	try
	{
		// bind to the same addr of listening socket
		ns->m_pUDT->open();
		updateMux(ns, ls);
		ns->m_pUDT->connect(peer, hs);
	}
	catch (...)
	{
		error = 1;
		goto ERR_ROLLBACK;
	}

	ns->m_Status = CONNECTED;

	// copy address information of local node
	ns->m_pUDT->m_pSndQueue->m_pChannel->getSockAddr(ns->m_pSelfAddr);
	CIPAddress::pton(ns->m_pSelfAddr, ns->m_pUDT->m_piSelfIP, ns->m_iIPversion);

	// protect the m_Sockets structure.
	CGuard::enterCS(m_ControlLock);
	try
	{
		m_Sockets[ns->m_SocketID] = ns;
		m_PeerRec[(ns->m_PeerID << 30) + ns->m_iISN].insert(ns->m_SocketID);
	}
	catch (...)
	{
		error = 2;
	}
	CGuard::leaveCS(m_ControlLock);

	CGuard::enterCS(ls->m_AcceptLock);
	try
	{
		ls->m_pQueuedSockets->insert(ns->m_SocketID);
	}
	catch (...)
	{
		error = 3;
	}
	CGuard::leaveCS(ls->m_AcceptLock);

ERR_ROLLBACK:
	if (error > 0)
	{
		ns->m_pUDT->close();
		ns->m_Status = CLOSED;
		ns->m_TimeStamp = CTimer::getTime();

		return -1;
	}

	return 1;
}

int CUDTUnited::connect(const UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	CGuard cg(s->m_ControlLock);

	// check the size of SOCKADDR structure
	if (AF_INET == s->m_iIPversion)
	{
		if (namelen != sizeof(sockaddr_in))
			throw CUDTException(5, 3, 0);
	}
	else
	{
		if (namelen != sizeof(sockaddr_in6))
			throw CUDTException(5, 3, 0);
	}

	// a socket can "connect" only if it is in INIT or OPENED status
	if (INIT == s->m_Status)
	{
		if (!s->m_pUDT->m_bRendezvous)
		{
			s->m_pUDT->open();
			updateMux(s, name, udpsaock);
			s->m_Status = OPENED;
		}
		else
			throw CUDTException(5, 8, 0);
	}
	else if (OPENED != s->m_Status)
		throw CUDTException(5, 2, 0);

	// connect_complete() may be called before connect() returns.
	// So we need to update the status before connect() is called,
	// otherwise the status may be overwritten with wrong value (CONNECTED vs. CONNECTING).
	s->m_Status = CONNECTING;
	try
	{
		s->m_pUDT->connect(name);
	}
	catch (CUDTException e)
	{
		s->m_Status = OPENED;
		throw e;
	}

	// record peer address
	delete s->m_pPeerAddr;
	if (AF_INET == s->m_iIPversion)
	{
		s->m_pPeerAddr = (sockaddr*)(new sockaddr_in);
		memcpy(s->m_pPeerAddr, name, sizeof(sockaddr_in));
	}
	else
	{
		s->m_pPeerAddr = (sockaddr*)(new sockaddr_in6);
		memcpy(s->m_pPeerAddr, name, sizeof(sockaddr_in6));
	}

	return 0;
}

int CUDTUnited::postRecv(UDTSOCKET u, const char* buf, uint32_t len)
{
	CUDTSocket* s = locate(u);
	if (NULL == s)
		throw CUDTException(5, 4, 0);

	s->m_pUDT->m_pRcvQueue->postRecv(buf, len);
	return 0;
}

void CUDTUnited::connect_complete(const UDTSOCKET u)
{

}

CUDTSocket* CUDTUnited::locate(const UDTSOCKET u)
{
	CGuard cg(m_ControlLock);

	std::map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

	if ((i == m_Sockets.end()) || (i->second->m_Status == CLOSED))
		return NULL;

	return i->second;
}

CUDTSocket* CUDTUnited::locate(const sockaddr* peer, const UDTSOCKET id, int32_t isn)
{
	CGuard cg(m_ControlLock);

	std::map<int64_t, std::set<UDTSOCKET> >::iterator i = m_PeerRec.find((id << 30) + isn);
	if (i == m_PeerRec.end())
		return NULL;

	for (std::set<UDTSOCKET>::iterator j = i->second.begin(); j != i->second.end(); ++ j)
	{
		std::map<UDTSOCKET, CUDTSocket*>::iterator k = m_Sockets.find(*j);
		// this socket might have been closed and moved m_ClosedSockets
		if (k == m_Sockets.end())
			continue;

		if (CIPAddress::ipcmp(peer, k->second->m_pPeerAddr, k->second->m_iIPversion))
			return k->second;
	}

	return NULL;
}

CUDT* CUDTUnited::lookup(const UDTSOCKET u)
{
	// protects the m_Sockets structure
	CGuard cg(m_ControlLock);

	std::map<UDTSOCKET, CUDTSocket*>::iterator i = m_Sockets.find(u);

	if ((i == m_Sockets.end()) || (i->second->m_Status == CLOSED))
		throw CUDTException(5, 4, 0);

	return i->second->m_pUDT;
}

UDTSTATUS CUDTUnited::getStatus(const UDTSOCKET u)
{
	return NONEXIST;
}

void CUDTUnited::updateMux(CUDTSocket* s, const sockaddr* addr, const UDPSOCKET* udpsock)
{
	CGuard cg(m_ControlLock);

	if ((s->m_pUDT->m_bReuseAddr) && (NULL != addr))
	{
		int port = (AF_INET == s->m_pUDT->m_iIPversion) ? ntohs(((sockaddr_in*)addr)->sin_port) : ntohs(((sockaddr_in6*)addr)->sin6_port);

		// find a reusable address
		for (std::map<int, CMultiplexer>::iterator i = m_mMultiplexer.begin(); i != m_mMultiplexer.end(); ++ i)
		{
			if ((i->second.m_iIPversion == s->m_pUDT->m_iIPversion) && (i->second.m_iMSS == s->m_pUDT->m_iMSS) && i->second.m_bReusable)
			{
				if (i->second.m_iPort == port)
				{
					// reuse the existing multiplexer
					++ i->second.m_iRefCount;
					s->m_pUDT->m_pSndQueue = i->second.m_pSndQueue;
					s->m_pUDT->m_pRcvQueue = i->second.m_pRcvQueue;
					s->m_iMuxID = i->second.m_iID;
					return;
				}
			}
		}
	}

	// a new multiplexer is needed
	CMultiplexer m;
	m.m_iMSS = s->m_pUDT->m_iMSS;
	m.m_iIPversion = s->m_pUDT->m_iIPversion;
	m.m_iRefCount = 1;
	m.m_bReusable = s->m_pUDT->m_bReuseAddr;
	m.m_iID = s->m_SocketID;

	m.m_pChannel = new CChannel(s->m_pUDT->m_iIPversion);
	m.m_pChannel->setSndBufSize(s->m_pUDT->m_iUDPSndBufSize);
	m.m_pChannel->setRcvBufSize(s->m_pUDT->m_iUDPRcvBufSize);

	try
	{
		if (NULL != udpsock)
			m.m_pChannel->open(*udpsock);
		else
			m.m_pChannel->open(addr);
	}
	catch (CUDTException& e)
	{
		m.m_pChannel->close();
		delete m.m_pChannel;
		throw e;
	}

	sockaddr* sa = (AF_INET == s->m_pUDT->m_iIPversion) ? (sockaddr*) new sockaddr_in : (sockaddr*) new sockaddr_in6;
	m.m_pChannel->getSockAddr(sa);
	m.m_iPort = (AF_INET == s->m_pUDT->m_iIPversion) ? ntohs(((sockaddr_in*)sa)->sin_port) : ntohs(((sockaddr_in6*)sa)->sin6_port);
	if (AF_INET == s->m_pUDT->m_iIPversion) delete (sockaddr_in*)sa; else delete (sockaddr_in6*)sa;

	m.m_pTimer = new CTimer;

	m.m_pSndQueue = new CSndQueue;
	m.m_pSndQueue->init(m.m_pChannel, m.m_pTimer);
	m.m_pRcvQueue = new CRcvQueue;
	m.m_pRcvQueue->init(32, s->m_pUDT->m_iPayloadSize, m.m_iIPversion, 1024, m.m_pChannel, m.m_pTimer);

	m_mMultiplexer[m.m_iID] = m;

	s->m_pUDT->m_pSndQueue = m.m_pSndQueue;
	s->m_pUDT->m_pRcvQueue = m.m_pRcvQueue;
	s->m_iMuxID = m.m_iID;
}

void CUDTUnited::updateMux(CUDTSocket* s, const CUDTSocket* ls)
{
	CGuard cg(m_ControlLock);

	int port = (AF_INET == ls->m_iIPversion) ? ntohs(((sockaddr_in*)ls->m_pSelfAddr)->sin_port) : ntohs(((sockaddr_in6*)ls->m_pSelfAddr)->sin6_port);

	// find the listener's address
	for (std::map<int, CMultiplexer>::iterator i = m_mMultiplexer.begin(); i != m_mMultiplexer.end(); ++ i)
	{
		if (i->second.m_iPort == port)
		{
			// reuse the existing multiplexer
			++ i->second.m_iRefCount;
			s->m_pUDT->m_pSndQueue = i->second.m_pSndQueue;
			s->m_pUDT->m_pRcvQueue = i->second.m_pRcvQueue;
			s->m_iMuxID = i->second.m_iID;
			return;
		}
	}
}

#ifndef WIN32
void* CUDTUnited::worker(void* p)
#else
DWORD WINAPI CUDTUnited::worker(LPVOID p)
#endif
{
	CUDTUnited* pThis = (CUDTUnited*)p;

	CGuard workguard(pThis->m_GCStopLock);

	while (!pThis->m_bClosing)
	{
		p2p_run(pThis->m_p2ph);
	}

#ifndef WIN32
	return NULL;
#else
	return 0;
#endif
}



////////////////////////////////////////////////////////////////////////////////
// UDT_API
namespace UDT
{
	UDTSOCKET socket(int af, int type, int protocol)
	{
		return CUDT::socket(af, type, protocol);
		return 0;
	}

	int bind(UDTSOCKET u, const struct sockaddr* name, int namelen)
	{
		//return CUDT::bind(u, name, namelen);
		return 0;
	}

	int listen(UDTSOCKET u, int backlog)
	{
		//return CUDT::listen(u, backlog);
		return 0;
	}

	UDTSOCKET accept(UDTSOCKET u, struct sockaddr* addr, int* addrlen)
	{
		//return CUDT::accept(u, addr, addrlen);
		return 0;
	}

	int connect(UDTSOCKET u, const UDPSOCKET* udpsaock, const struct sockaddr* name, int namelen)
	{
		return CUDT::connect(u, udpsaock, name, namelen);
	}

	int close(UDTSOCKET u)
	{
		//return CUDT::close(u);
		return 0;
	}

	int getpeername(UDTSOCKET u, struct sockaddr* name, int* namelen)
	{
		//return CUDT::getpeername(u, name, namelen);
		return 0;
	}

	int getsockname(UDTSOCKET u, struct sockaddr* name, int* namelen)
	{
		//return CUDT::getsockname(u, name, namelen);
		return 0;
	}

	int getsockopt(UDTSOCKET u, int level, SOCKOPT optname, void* optval, int* optlen)
	{
		//return CUDT::getsockopt(u, level, optname, optval, optlen);
		return 0;
	}

	int setsockopt(UDTSOCKET u, int level, SOCKOPT optname, const void* optval, int optlen)
	{
		//return CUDT::setsockopt(u, level, optname, optval, optlen);
		return 0;
	}

	int send(UDTSOCKET u, const char* buf, int len, int flags)
	{
		return CUDT::send(u, buf, len, flags);
		return 0;
	}

	int recv(UDTSOCKET u, char* buf, int len, int flags)
	{
		//return CUDT::recv(u, buf, len, flags);
		return 0;
	}

	int64_t sendfile(UDTSOCKET u, std::fstream& ifs, int64_t& offset, int64_t size, int block)
	{
		//return CUDT::sendfile(u, ifs, offset, size, block);
		return 0;
	}

	int64_t recvfile(UDTSOCKET u, std::fstream& ofs, int64_t& offset, int64_t size, int block)
	{
		//return CUDT::recvfile(u, ofs, offset, size, block);
		return 0;
	}


	int perfmon(UDTSOCKET u, TRACEINFO* perf, bool clear)
	{
		//return CUDT::perfmon(u, perf, clear);
		return 0;
	}

	//UDTSTATUS getsockstate(UDTSOCKET u)
	//{
		//return CUDT::getsockstate(u);
	//	return 0;
	//}

	int p2pInit(const char* ctlip, const uint16_t ctlport, const char* stunip, const uint16_t stunport, const char* name, const char* passwd)
	{
		return CUDT::startup(ctlip, ctlport, stunip, stunport, name, passwd);
	}

	UDTSOCKET p2pConnect(const char* peername)
	{
		return CUDT::p2pConnect(peername);
	}

	int p2pDisconnect(const UDTSOCKET u, const char* peername)
	{
		return CUDT::p2p_disconnect(peername);
	}

	int p2pSend(const UDTSOCKET u, const char* peername, const char* buf, int len)
	{
		return CUDT::p2p_send(u, peername, buf, len);
	}

	int p2pClose()
	{
		return 0;
	}

}  // namespace UDT



//////////////////////////////////////////////////////////////////////////
// API
int CUDT::startup(const char* ctlip, const uint16_t ctlport, const char* stunip, const uint16_t stunport, const char* name, const char* passwd)
{
	try
	{
		return s_UDTUnited.startup(ctlip, ctlport, stunip, stunport, name, passwd, p2p_init_handler, p2p_request_handler);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

UDTSOCKET CUDT::p2pConnect(const char* peername)
{
	try
	{
		UDTSOCKET u = s_UDTUnited.newSocket(AF_INET, SOCK_STREAM);
		CUDT* udt = s_UDTUnited.lookup(u);
		return udt->p2pConnect(u, peername, s_UDTUnited.m_p2ph);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::p2p_disconnect(const char* peername)
{
	try
	{
		return s_UDTUnited.p2pDisconnect(peername);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::p2p_send(const UDTSOCKET u, const char* peername, const char* buf, int len)
{
	try
	{
		CUDT* udt = s_UDTUnited.lookup(u);
		return udt->send(buf, len);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}


int CUDT::p2p_close()
{
	try
	{
		return s_UDTUnited.p2pClose();
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

UDTSOCKET CUDT::socket(int af, int type, int protocol)
{
	try
	{
		return s_UDTUnited.newSocket(af, type);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::bind(UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen)
{
	try
	{
		return s_UDTUnited.bind(u, udpsaock, name, namelen);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::listen(UDTSOCKET u, int backlog)
{
	try
	{
		return s_UDTUnited.listen(u, backlog);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::newConnection(const UDTSOCKET listen, const sockaddr* peer, CHandShake* hs)
{
	try
	{
		return s_UDTUnited.newConnection(listen, peer, hs);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::connect(UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen)
{
	try
	{
		//CUDT* udt = s_UDTUnited.lookup(u);
		//return udt->connect();
		return s_UDTUnited.connect(u, udpsaock, name, namelen);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::postRecv(UDTSOCKET u, const char* buf, uint32_t len)
{
	try
	{
		return s_UDTUnited.postRecv(u, buf, len);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::send(UDTSOCKET u, const char* buf, int len, int flags)
{
	try
	{
		CUDT* udt = s_UDTUnited.lookup(u);
		return udt->send(buf, len);
	}
	catch (...)
	{
		return INVALID_SOCK;
	}
}

int CUDT::getpeername(UDTSOCKET u, sockaddr* name, int* namelen)
{
	return 0;
}

int CUDT::getsockname(UDTSOCKET u, sockaddr* name, int* namelen)
{
	return 0;
}

CUDT* CUDT::getUDTHandle(UDTSOCKET u)
{
	return NULL;
}

UDTSTATUS CUDT::getsockstate(UDTSOCKET u)
{
	return NONEXIST;
}
