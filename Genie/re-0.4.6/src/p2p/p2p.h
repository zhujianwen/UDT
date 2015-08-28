#ifndef _P2P_H_
#define _P2P_H_

struct p2phdr
{
        uint16_t flag;    /*p2p flag*/
        uint16_t type;    /*p2p message type */
};



void p2p_destructor(void *arg);
int p2p_alloc(struct p2phandle **p2, const char* user, const char* passwd);
void tcp_close_handler(int err, void *arg);
void tcp_estab_handler(void *arg);

void gather_handler(int err, uint16_t scode, const char* reason,void *arg);
void connchk_handler(int err, bool update, void *arg);

int p2p_send_login(struct tcp_conn *tc, const char* user, const char* passwd);
void tcp_recv_handler(struct mbuf *mb, void* arg);
int p2pconnection_alloc(struct p2phandle *p2p,struct p2pconnection **pconn,const char* name);
struct p2pconnection* find_p2pconnection(struct p2phandle *p2p, const char* name);
int p2p_request(struct p2pconnection *pconn, const char* myname, const char* peername);
int p2p_response(struct p2pconnection *pconn, const char* peername);

#endif
