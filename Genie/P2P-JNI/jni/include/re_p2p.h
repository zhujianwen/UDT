
#ifndef _RE_P2P_H_
#define _RE_P2P_H_

struct ice;
struct udp_sock;
struct sa;
struct tcp_conn;

#define USERNAME_SIZE 512
#define PASSWD_SIZE 512
#define P2P_KEY_SIZE 64

enum
{
        P2P_LOGINREQ = 1,
	P2P_LOGINREP,
        P2P_CONNREQ,
        P2P_CONNREP,
        P2P_DATA
};

extern const uint16_t p2pflag;

enum
{
	P2P_INIT = 1,
	P2P_ACCEPT,
	P2P_CONNECTING,
	P2P_CONNECTED,
	P2P_CONNECTERR
};
struct p2phandle;
struct p2pconnection;

typedef void (p2p_init_h)(int err, struct p2phandle* p2p);
typedef void (p2p_connect_h)(int err,const char* reason, struct p2pconnection* p2pcon);
typedef void (p2p_receive_h)(const char* data, uint32_t len, struct p2pconnection* p2pcon);
typedef void (p2p_request_h)(struct p2phandle* p2p, const char* peername, struct p2pconnection **p2pconn);

struct p2phandle
{
	struct tcp_conn *ltcp; /* control tcp socket */
	char *username;
	char *passwd;
	struct sa* sactl;
	struct sa* sastun;
	struct list lstconn;   /* connection list */
	char key[P2P_KEY_SIZE];
	uint16_t status;
	p2p_request_h *requesth;
	p2p_init_h *inith;
};


struct p2pconnection
{
	struct le le;
	char *peername;
	struct ice *ice;
	struct p2phandle *p2p;
	struct udp_sock *ulsock;
	int udtsock;
	uint16_t status;
	p2p_connect_h *ch;
	p2p_receive_h *rh;
};


int p2p_init(struct p2phandle **pp, const char* ctlip, const uint16_t ctlport,
		const char* stunip, const uint16_t stunport,
		const char* name, const char* passwd, 
		p2p_init_h *inith, p2p_request_h *requesth);

int p2p_connect(struct p2phandle *p2p, struct p2pconnection **ppconn, const char* peername, 
		p2p_connect_h *connecth, p2p_receive_h *receiveh, void *arg);
int p2p_accept(struct p2phandle *p2p, struct p2pconnection **ppconn, const char* peername,
		p2p_receive_h *receiveh, void *arg);
int p2p_send(struct p2pconnection *pconn, const char* buf, uint32_t size);
int p2p_disconnect(struct p2pconnection *pconn);
int p2p_close(struct p2phandle *p2p);
int p2p_run(struct p2phandle *p2p);

#endif
