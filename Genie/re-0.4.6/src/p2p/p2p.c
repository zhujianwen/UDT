/*
 * p2p.c
 *
 */
#include <re_types.h>
#include <re_mem.h>
#include <re_list.h>
#include <re_fmt.h>
#include <re_mbuf.h>
#include <re_sa.h>
#include <re_net.h>
#include <re_tcp.h>
#include <re_udp.h>
#include <re_p2p.h>
#include <re_ice.h>
#include <re_main.h>

#include "p2p.h"
#include <string.h>

#define UNUSED(x) (void)x 

const uint16_t p2pflag = 0xc000;

/*
 * p2p instance destructor handler
 */
void p2p_destructor(void *arg)
{
	struct p2phandle *p2p = (struct p2phandle*)arg;
	mem_deref(p2p->ltcp);
	mem_deref(p2p->sactl);
	mem_deref(p2p->sastun);
}
static int print_handler(const char *p, size_t size, void *arg);
void tcp_estab_handler(void *arg);
int p2p_sdp_encode(struct ice *ice, struct icem *icem, struct mbuf *mbuf);

/*
 * p2pconnection destructor handler
 */
void p2pconnection_destructor(void *arg);
void p2pconnection_destructor(void *arg)
{
	struct p2pconnection *pc = arg;
	
	list_unlink(&(pc->le));

	mem_deref(pc->p2p);
	mem_deref(pc->ice);
	mem_deref(pc->peername);
}

/*
 * tcp connection established handler
 */
void tcp_estab_handler(void *arg)
{
	int err;

	struct p2phandle *p2p = arg;
	
	err = p2p_send_login(p2p->ltcp, p2p->username, p2p->passwd);
	if (err)
	{
		re_fprintf(stderr, "p2p register to control server error:%s\n",strerror(err));
	}
}

int p2p_sdp_encode(struct ice *ice, struct icem *icem, struct mbuf *mbuf)
{
	struct le *le;
	struct re_printf pt;
	struct pl pl;
	char buf[1024];

	struct list *canlist;
       
	if (!ice || !icem)
		return -1;

	canlist = icem_lcandl(icem);
	
	for(le = canlist->head; le; le = le->next)
	{
		memset(buf,0,1024);
		pl.l = 1024;
		pl.p = buf;
		pt.vph=print_handler;
		pt.arg=&pl;
		ice_cand_encode(&pt,(struct cand *)le->data);
		mbuf_write_u32(mbuf,str_len(buf)+2);
		mbuf_write_str(mbuf,"CA");
		mbuf_write_str(mbuf,buf);
		re_printf("send sdp:%s\n",buf);	
	}
	
	mbuf_write_u32(mbuf,str_len(ice_ufrag(ice))+2);
        mbuf_write_str(mbuf, "UF");
	mbuf_write_str(mbuf, ice_ufrag(ice));
	mbuf_write_u32(mbuf, str_len(ice_pwd(ice))+2);
	mbuf_write_str(mbuf, "PW");
	mbuf_write_str(mbuf, ice_pwd(ice));
	return 0;
}

int p2p_sdp_decode(struct mbuf *mb, struct icem *icem);
int p2p_sdp_decode(struct mbuf *mb, struct icem *icem)
{
	char buf[1024];
	char hd[8];
	uint16_t len;

	while(mb->end > mb->pos)
	{
		memset(buf,0,1024);
		memset(hd, 0, 8);
		len = mbuf_read_u32(mb);
		mbuf_read_str(mb,buf,len < 1024?len:1024);
		str_ncpy(hd,buf,3);
		re_printf("******recieve sdp:%s\n",buf);
		if (str_cmp(hd, "CA")==0)
		        icem_sdp_decode(icem,ice_attr_cand,buf+2);
		else if (str_cmp(hd, "UF")==0)
			icem_sdp_decode(icem,ice_attr_ufrag,buf+2);
		else if (str_cmp(hd, "PW")==0)
			icem_sdp_decode(icem, ice_attr_pwd,buf+2);
	}
	return 0;
}

/*
 * receive tcp data handler
 */
void tcp_recv_handler(struct mbuf *mb, void* arg)
{
	uint16_t flag;
	uint16_t type,len;
	struct p2phandle *p2p = arg;
	char peername[512], buf[512];
	struct p2pconnection *pconn;
	struct icem *icem;
	struct list *mlist;
	uint16_t uret;
	
	flag = mbuf_read_u16(mb);
	if (flag != p2pflag)
		return;

	type = mbuf_read_u16(mb);
	/* p2p connect request */
	if (type == P2P_CONNREQ || type == P2P_CONNREP)
	{
		memset(peername, 0, 512);
		len = mbuf_read_u16(mb);
		mbuf_read_str(mb, peername, len);
		len = mbuf_read_u16(mb);
		mbuf_read_str(mb, buf, len);

		pconn = find_p2pconnection(p2p, peername);
		if (type == P2P_CONNREQ)
		{
			if (!pconn)
			{
				if (p2p->requesth)
					p2p->requesth(p2p, peername, &pconn);
			}
		}
		if (pconn)
		{
			mlist = ice_medialist(pconn->ice);
			icem = list_ledata(list_head(mlist));
			p2p_sdp_decode(mb, icem);
			
			
			if (type == P2P_CONNREQ && pconn->status != P2P_INIT && pconn->status != P2P_ACCEPT)
			{
				p2p_response(pconn, peername);
				pconn->status = P2P_CONNECTING;
				ice_conncheck_start(pconn->ice);
			}

			if (type == P2P_CONNREP)
			{
				/*start connect check */
				ice_conncheck_start(pconn->ice);
			}
		}
	}else if (type == P2P_LOGINREP)
	{
		/* login response message */
		uret = mbuf_read_u16(mb);
		if (p2p->status == P2P_INIT && p2p->inith)
		{
			p2p->inith(uret, p2p);
			if (!uret)
				p2p->status = P2P_CONNECTED;
		}
	}

}

struct p2pconnection* find_p2pconnection(struct p2phandle *p2p, const char* name)
{
	struct le *le;
	struct list *lstconn = &(p2p->lstconn);
	
	for(le = list_head(lstconn); le; le=le->next)
	{
		struct p2pconnection *pconn = le->data;
		if (str_cmp(pconn->peername,name)==0)
		{
			return pconn;
		}
	}
	return NULL;
}

/*
 * receive udp data handler
 */
void udp_recv_handler(const struct sa *src, struct mbuf *mb, void *arg);
void udp_recv_handler(const struct sa *src, struct mbuf *mb, void *arg)
{
	char *name;
	uint16_t type,flag,len;
	struct p2pconnection *pconn = arg;
	
	UNUSED(src);

	flag = mbuf_read_u16(mb);
	if (flag != p2pflag)
	{
		re_printf("not p2p package!\n");
		return ;
	}

	type = mbuf_read_u16(mb);
	if (type == P2P_DATA)
	{
		len = mbuf_read_u16(mb);
		name = mem_zalloc(len+1, NULL);
		mbuf_read_str(mb, name, len);
		if (pconn->rh)
			pconn->rh(name, len, pconn);

		mem_deref(name);
	}else
	{
		re_printf("message type not process:%d\n", type);
	}


}

/*
 * tcp close handler
 */
void tcp_close_handler(int err, void *arg)
{
	struct p2phandle *p2p = arg;
	if (!err && p2p->inith)
		p2p->inith(err, p2p);

	re_printf("tcp closed error:%s(%d)\n", strerror(err),err);
}

/*
 * gather infomation handler
 */
void gather_handler(int err, uint16_t scode, const char* reason,
		void *arg)
{
	struct p2pconnection *p2pconn = arg;
	
	UNUSED(scode);
	UNUSED(reason);
	
	re_printf("gather_handler:%s\n",strerror(err));
	if (p2pconn->status == P2P_INIT)
		p2p_request(p2pconn, p2pconn->p2p->username, p2pconn->peername);
	else if (p2pconn->status == P2P_ACCEPT){
		p2p_response(p2pconn, p2pconn->peername);
		p2pconn->status = P2P_CONNECTING;
		ice_conncheck_start(p2pconn->ice);
		 
	}

}

/*
 * connect check handler
 */
void connchk_handler(int err, bool update, void *arg)
{
	struct p2pconnection *p2pconn = arg;

	UNUSED(update);

	re_printf("connchk_handler:%s", strerror(err));
	if (!err)
	{
		p2pconn->status = P2P_CONNECTED;
	}
	if (p2pconn->ch)
		p2pconn->ch(err,"", p2pconn);
	

}



/*
 * re_printf_h 
 */
static int print_handler(const char *p, size_t size, void *arg)
{
	struct pl *pl = (struct pl*)arg;
	if (size > pl->l)
		return ENOMEM;
	memcpy((void *)pl->p, p, size);
	pl_advance(pl, size);
	return 0;
}

/*
 * alloc p2p instance
 *
 */
int p2p_alloc(struct p2phandle **p2p, const char* username, const char* passwd)
{
	struct p2phandle *p;

	p = mem_zalloc(sizeof(*p),p2p_destructor);
	if (!p)
		return ENOMEM;
	
	p->sactl = mem_zalloc(sizeof(struct sa),NULL);
	p->sastun = mem_zalloc(sizeof(struct sa), NULL);

	if (username)
	{
		p->username = mem_zalloc(str_len(username)+1, NULL);
		str_ncpy(p->username, username, str_len(username)+1);
	}

	if (passwd)
	{
		p->passwd = mem_zalloc(str_len(passwd)+1, NULL);
		str_ncpy(p->passwd, passwd, str_len(passwd)+1);
	}

	sa_init(p->sactl, AF_INET);
	sa_init(p->sastun, AF_INET);
	*p2p = p;
		
	return 0;
}

/*
 * alloc a p2pconnection
 *
 */
int p2pconnection_alloc(struct p2phandle *p2p, struct p2pconnection **ppconn, const char* name)
{
	int err;
	struct p2pconnection *pconn;
	
	pconn = mem_zalloc(sizeof(*pconn), p2pconnection_destructor);
	if (!pconn)
		return ENOMEM;
	
	pconn->peername = mem_zalloc(str_len(name)+1, NULL);
	str_ncpy(pconn->peername, name, str_len(name)+1);

	pconn->p2p = mem_ref(p2p);
	list_append(&(p2p->lstconn), &(pconn->le), pconn);

	err = ice_alloc(&(pconn->ice), ICE_MODE_FULL, false);
	if (err)
	{
		re_fprintf(stderr, "ice alloc error:%s\n", strerror(err));
		goto out;
	}
	*ppconn = pconn;
	return 0;
out:
	mem_deref(pconn->ice);
	mem_deref(pconn->p2p);
	mem_deref(pconn);
	return err;
}

/*
 *send p2p login message to control server
 */
int p2p_send_login(struct tcp_conn *tc, const char* user, const char* passwd)
{
	int err;

	struct mbuf *mb;

	mb = mbuf_alloc(512);
        mbuf_write_u16(mb, p2pflag);
        mbuf_write_u16(mb, P2P_LOGINREQ);
	mbuf_write_u16(mb, str_len(user));
	mbuf_write_str(mb, user);
	mbuf_write_u16(mb, str_len(passwd));
	mbuf_write_str(mb, passwd);
	
	mb->pos = 0;
		
	err = tcp_send(tc, mb);
	if (err)
	{
		re_fprintf(stderr, "send p2p login error:%s\n", strerror(err));
	}
	mem_deref(mb);
	return err;
}

/*
 * send p2p connect request to control server
 *
 */
int p2p_request(struct p2pconnection *pconn, const char* myname, const char* peername)
{
	int err;
	struct mbuf *mb;
	struct icem *icem;

	mb = mbuf_alloc(1024);
	mbuf_write_u16(mb, p2pflag);
	mbuf_write_u16(mb, P2P_CONNREQ);
	mbuf_write_u16(mb, str_len(myname));
	mbuf_write_str(mb, myname);
	mbuf_write_u16(mb, str_len(peername));
	mbuf_write_str(mb, peername);
	
	icem = list_ledata(list_head(ice_medialist(pconn->ice)));
	p2p_sdp_encode(pconn->ice, icem, mb);

	mb->pos = 0;
	err = tcp_send(pconn->p2p->ltcp, mb);
	if (err)
	{
		re_fprintf(stderr, "send p2p request error:%s\n", strerror(err));	
	}
	mem_deref(mb);
	return err;
}

/*
 *
 *
 */
int p2p_response(struct p2pconnection *pconn, const char* peername)
{
	int err;
	struct mbuf *mb;
	struct icem *icem;
	
	mb = mbuf_alloc(1024);
	mbuf_write_u16(mb, p2pflag);
	mbuf_write_u16(mb, P2P_CONNREP);
	mbuf_write_u16(mb, str_len(pconn->p2p->username));
	mbuf_write_str(mb, pconn->p2p->username);
	mbuf_write_u16(mb, str_len(peername));
	mbuf_write_str(mb, peername);

	icem = list_ledata(list_head(ice_medialist(pconn->ice)));
	p2p_sdp_encode(pconn->ice, icem, mb);
	
	mb->pos = 0;
	err = tcp_send(pconn->p2p->ltcp, mb);
	if (err)
	{		
		re_fprintf(stderr, "send p2p response error:%s\n", strerror(err));	
	}
	return err;

}

static void signal_handler(int sig)
{
	re_printf("terminating on signal %d...\n", sig);
	libre_close();
}

/*
 * start p2p instance 
 */
int p2p_init(struct p2phandle **p2p, 
	const char *ctlsrv, const uint16_t ctlport, 
	const char* stunsrv, const uint16_t stunport, 
	const char* name, const char* passwd, 
	p2p_init_h *inith, p2p_request_h *requesth)
{
	int err;
	struct p2phandle *p;
	
	err = libre_init();
	if (err) 
	{
		re_fprintf(stderr, "re init failed: %s\n", strerror(err));
		goto out;
	}

	err = p2p_alloc(&p, name, passwd);
	if (!p)
	{
		return ENOMEM;
	}

	p->requesth = requesth;
	p->inith = inith;
	p->username = mem_zalloc(str_len(name)+1, NULL);
	str_ncpy(p->username, name, str_len(name)+1);
	p->passwd = mem_zalloc(str_len(passwd)+1, NULL);
	str_ncpy(p->passwd, passwd, str_len(passwd)+1);
	/* set init status */
	p->status = P2P_INIT;

	sa_set_str(p->sactl, ctlsrv, ctlport);
	sa_set_str(p->sastun, stunsrv, stunport);
	
	err = tcp_conn_alloc(&(p->ltcp), p->sactl,tcp_estab_handler,
		       tcp_recv_handler, tcp_close_handler, p);
	if (err)
	{
		re_fprintf(stderr, "tcp connect alloc error:%s\n", strerror(err));
		goto out;

	}

	err = tcp_conn_connect(p->ltcp, p->sactl);
	if (err)
	{
		re_fprintf(stderr, "tcp connect error:%s\n", strerror(err));
		goto out;
	}
	*p2p = p;

	//err = re_main(signal_handler); 

	return 0;
out:
	mem_deref(p);
	libre_close();
	return err;
}

/*
 * connect to anthor peer node
 */
int p2p_connect(struct p2phandle *p2p, struct p2pconnection **ppconn, const char* peername,
		p2p_connect_h *connecth, p2p_receive_h *receiveh, void *arg)
{
	int err;
	struct p2pconnection *pconn;
	struct sa laddr;
	struct icem *icem;

	err = p2pconnection_alloc(p2p, &pconn, peername);
	if (err)
	{
		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}

	pconn->ch = connecth;
	pconn->rh = receiveh;
	
	err = icem_alloc(&(icem), pconn->ice, IPPROTO_UDP, 3, 
			gather_handler, connchk_handler, pconn);
	if (err)
	{

		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}
  	err = net_default_source_addr_get(AF_INET, &laddr);
	if (err) 
	{
		re_fprintf(stderr, "local address error: %s\n", strerror(err));
		goto out;
	}
	 
	sa_set_port(&laddr, 0);
	err = udp_listen(&(pconn->ulsock), &laddr, udp_recv_handler, pconn);
	if (err)
	{
		re_fprintf(stderr, "udp listen error:%s\n", strerror(err));
		goto out;
	}

	err = icem_comp_add(icem,ICE_COMPID_RTP, pconn->ulsock);
	if (err)
	{
		re_fprintf(stderr, "icem compnent add error:%s\n", strerror(err));
		goto out;
	}
	err = icem_cand_add(icem,ICE_COMPID_RTP, 0, NULL, &laddr);
	if (err)
	{
		re_fprintf(stderr,"icem cand add error:%s\n", strerror(err));
		goto out;
	}
	
	/*set status to init */
	pconn->status = P2P_INIT;

	
	err = icem_gather_srflx(icem, p2p->sastun);
	if(err)
	{
		re_fprintf(stderr, "gather server filrex error:%s\n",strerror(err));
		goto out;
	}
	
	err = icem_gather_relay(icem, p2p->sastun, ice_ufrag(pconn->ice), ice_pwd(pconn->ice));
	if(err)
	{
		re_fprintf(stderr, "gather server relay error:%s\n",strerror(err));
		goto out;
	}
	
	*ppconn = pconn;
	return 0;
out:
	pconn->ch = NULL;
	pconn->rh = NULL;
	mem_deref(pconn);
	return err;
}

int p2p_accept(struct p2phandle* p2p, struct p2pconnection** ppconn, const char* peername,
		p2p_receive_h *receiveh, void *arg)
{

	int err;
	struct p2pconnection *pconn;
	struct sa laddr;
	struct icem *icem;

	err = p2pconnection_alloc(p2p, &pconn, peername);
	if (err)
	{
		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}

	pconn->rh = receiveh;
	
	err = icem_alloc(&(icem), pconn->ice, IPPROTO_UDP, 3, gather_handler, connchk_handler, pconn);
	if (err)
	{

		re_fprintf(stderr, "p2pconnection alloc error:%s\n", strerror(err));
		goto out;
	}
  	err = net_default_source_addr_get(AF_INET, &laddr);
	if (err) 
	{
		re_fprintf(stderr, "local address error: %s\n", strerror(err));
		goto out;
	}
	 
	sa_set_port(&laddr, 0);
	err = udp_listen(&(pconn->ulsock), &laddr, udp_recv_handler, pconn);
	if (err)
	{
		re_fprintf(stderr, "udp listen error:%s\n", strerror(err));
		goto out;
	}

	err = icem_comp_add(icem,ICE_COMPID_RTP, pconn->ulsock);
	if (err)
	{
		re_fprintf(stderr, "icem compnent add error:%s\n", strerror(err));
		goto out;
	}
	err = icem_cand_add(icem, ICE_COMPID_RTP, 0, NULL, &laddr);
	if (err)
	{
		re_fprintf(stderr,"icem cand add error:%s\n", strerror(err));
		goto out;
	}
	
	/*set status to ACCEPT */
	pconn->status = P2P_ACCEPT;
/*
	err = icem_gather_srflx(icem, p2p->sastun);
	if(err)
	{
		re_fprintf(stderr, "gather server filrex error:%s\n",strerror(err));
		goto out;
	}
*/		
	err = icem_gather_relay(icem, p2p->sastun, ice_ufrag(pconn->ice), ice_pwd(pconn->ice));
	if(err)
	{
		re_fprintf(stderr, "gather server relay error:%s\n",strerror(err));
		goto out;
	}
	
	*ppconn = pconn;
	return 0;
out:
	pconn->ch = NULL;
	pconn->rh = NULL;
	mem_deref(pconn);
	return err;
}


/*
 * send data via p2p connection
 *
 */
int p2p_send(struct p2pconnection *pconn, const char* buf, uint32_t size)
{
	
	int err;
	struct mbuf *mb;
	const struct sa *sa;
	struct list *list;
	struct icem *icem;
	uint8_t compid = ICE_COMPID_RTP;

	mb = mbuf_alloc(1024);
	mbuf_write_u16(mb, p2pflag);
	mbuf_write_u16(mb, P2P_DATA);
	mbuf_write_u16(mb, size);
	mbuf_write_str(mb, buf);
	
	mb->pos = 0;
	
	list = ice_medialist(pconn->ice);
	icem = (struct icem *)list_ledata(list_head(list));
	
	sa = icem_selected_raddr((const struct icem*)icem, compid);
	if (!sa){
		re_printf("select default cand.\n");
		sa = icem_cand_default(icem, compid);
	}

	if (sa)
	{
		err = udp_send(pconn->ulsock, sa, mb);
	}else
	{
		err = ENOENT;
		
	}
	
	mem_deref(mb);
	return err;
}

/*
 * close p2p connection
 * 
 */
int p2p_disconnect(struct p2pconnection* pconn)
{
	mem_deref(pconn->ulsock);
	mem_deref(pconn);

	return 0;
}

/*
 * close p2p
 */
int p2p_close(struct p2phandle* p2p)
{
	struct le *le;
	le = list_head(&(p2p->lstconn));
	for(le=list_head(&(p2p->lstconn)); le; le=le->next)
	{
		p2p_disconnect(le->data);
	}
	mem_deref(p2p);
	libre_close();

	return 0;
}

/*
 *
 *
 */
int p2p_run(struct p2phandle* p)
{
	UNUSED(p);

	re_main(signal_handler); 
	return 0;
}
