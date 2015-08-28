#ifndef __API_H__
#define __API_H__

#include "udt.h"
#include "core.h"
#include "queue.h"
#include "packet.h"
#include "cache.h"
#include "epoll.h"

#include <re_p2p.h>


void p2p_init_handler(int err, struct p2phandle* p2p);
void p2p_connect_handler(int err, const char* reason, struct p2pconnection* p2pcon);
void p2p_receive_handler(const char* data, uint32_t len, struct p2pconnection * p2pcon);
void p2p_request_handler(struct p2phandle *p2p, const char* peername, struct p2pconnection **p2pcon);


class CUDT;
class CUDTSocket
{
public:
	CUDTSocket();
	~CUDTSocket();

	UDTSTATUS m_Status;                       // current socket state

	uint64_t m_TimeStamp;                     // time when the socket is closed

	int m_iIPversion;                         // IP version
	sockaddr* m_pSelfAddr;                    // pointer to the local address of the socket
	sockaddr* m_pPeerAddr;                    // pointer to the peer address of the socket

	UDTSOCKET m_SocketID;                     // socket ID
	UDTSOCKET m_ListenSocket;                 // ID of the listener socket; 0 means this is an independent socket

	UDTSOCKET m_PeerID;                       // peer socket ID
	int32_t m_iISN;                           // initial sequence number, used to tell different connection from same IP:port

	CUDT* m_pUDT;                             // pointer to the UDT entity

	std::set<UDTSOCKET>* m_pQueuedSockets;    // set of connections waiting for accept()
	std::set<UDTSOCKET>* m_pAcceptSockets;    // set of accept()ed connections

	pthread_cond_t m_AcceptCond;              // used to block "accept" call
	pthread_mutex_t m_AcceptLock;             // mutex associated to m_AcceptCond

	unsigned int m_uiBackLog;                 // maximum number of connections in queue

	int m_iMuxID;                             // multiplexer ID

	pthread_mutex_t m_ControlLock;            // lock this socket exclusively for control APIs: bind/listen/connect

private:
	CUDTSocket(const CUDTSocket&);
	CUDTSocket& operator=(const CUDTSocket&);
};

class CUDTUnited
{
	friend class CUDT;
	friend class CRendezvousQueue;

public:
	CUDTUnited();
	~CUDTUnited();

public:
	int startup(const char* ctlip, const uint16_t ctlport, 
		const char* stunip, const uint16_t stunport,
		const char* name, const char* passwd, 
		p2p_init_h* inith, p2p_request_h* requesth);
	int p2pConnect(const UDTSOCKET u, const char* peername);
	int p2pAccept();
	int p2pSend(const UDTSOCKET u, const char* peername, const char* buf, int len);
	int p2pDisconnect(const char* peername);
	int p2pClose();

	int bind(const UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen);
	int listen(const UDTSOCKET u, const sockaddr* peer, int backlog);
	UDTSOCKET newSocket(int af, int type);
	int newConnection(const UDTSOCKET listen, const sockaddr* peer, CHandShake* hs);
	UDTSOCKET connect(const UDTSOCKET u, const UDPSOCKET* udpsaock, const sockaddr* name, int namelen);
	int postRecv(UDTSOCKET u, const sockaddr* addr, const char* buf, uint32_t len);
	CUDT* lookup(const UDTSOCKET u);
	UDTSTATUS getStatus(const UDTSOCKET u);

private:
	void connect_complete(const UDTSOCKET u);
	CUDTSocket* locate(const UDTSOCKET u);
	CUDTSocket* locate(const sockaddr* peer, const UDTSOCKET id, int32_t isn);
	void updateMux(CUDTSocket* s, const sockaddr* addr = NULL, const UDPSOCKET* = NULL);
	void updateMux(CUDTSocket* s, const CUDTSocket* ls);

private:
	volatile bool m_bClosing;
	pthread_mutex_t m_GCStopLock;
	pthread_cond_t m_GCStopCond;

	pthread_mutex_t m_InitLock;
	int m_iInstanceCount;				// number of startup() called by application
	bool m_bGCStatus;					// if the GC thread is working (true)

	pthread_t m_WorkerThread;
#ifndef WIN32
	static void* worker(void* p);
#else
	static DWORD WINAPI worker(LPVOID p);
#endif

private:
	std::map<int64_t, std::set<UDTSOCKET> > m_PeerRec;// record sockets from peers to avoid repeated connection request, int64_t = (socker_id << 30) + isn
	std::map<UDTSOCKET, CUDTSocket*> m_ClosedSockets;   // temporarily store closed sockets
	std::map<UDTSOCKET, CUDTSocket*> m_Sockets;       // stores all the socket structures
	pthread_mutex_t m_ControlLock;                    // used to synchronize UDT API
	pthread_mutex_t m_IDLock;                         // used to synchronize ID generation
	UDTSOCKET m_SocketID;                             // seed to generate a new unique socket ID

	std::map<int, CMultiplexer> m_mMultiplexer;		// UDP multiplexer
	pthread_mutex_t m_MultiplexerLock;
	CCache<CInfoBlock>* m_pCache;			// UDT network information cache

	CEPoll m_EPoll;
	struct p2phandle* m_p2ph;

private:
	CUDTUnited(const CUDTUnited& other);
	const CUDTUnited& operator=(const CUDTUnited& other);
};

#endif	// __API_H__