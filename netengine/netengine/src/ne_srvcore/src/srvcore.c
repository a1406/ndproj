#include "ne_srvcore/ne_srvlib.h"

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")

#ifdef NE_DEBUG
#pragma comment(lib, "ne_common_dbg.lib")
#pragma comment(lib, "ne_net_dbg.lib")
#else 
#pragma comment(lib, "ne_common.lib")
#pragma comment(lib, "ne_net.lib")
#endif 

extern NEINT32 ne_iocp_node_init(struct ne_client_map_iocp *iocp_map,ne_handle h_listen) ;
extern NEINT32 iocp_close_client(struct ne_client_map_iocp *iocp_map, NEINT32 force) ; 
#endif 


NEINT32 register_listensrv(void) ;

NEINT32 ne_srvcore_init(void)
{
	return register_listensrv() ;
}

void ne_srvcore_destroy(void)
{

}

static void srv_tcp_init(struct  listen_contex *lc)
{
	memset(lc, 0, sizeof(*lc)) ;
	ne_tcpsrv_node_init(&lc->tcp);
	lc->io_mod = NE_LISTEN_COMMON ;
	ne_listensrv_alloc_entry((ne_handle)lc,ne_cm_node_alloc,ne_cm_node_free,(cm_init) ne_tcpcm_init ) ;
	lc->operate_timeout = CONNECTION_TIMEOUT * 1000;
//	INIT_LIST_HEAD(&lc->wait_free) ;
//	ne_mutex_init(&lc->wf_lock) ;
//	lc->session_release_entry =(ne_close_callback ) tcp_client_close ;
}

static void srv_ext_init(struct  listen_contex *lc)
{
	memset(lc, 0, sizeof(*lc));
	ne_tcpsrv_node_init(&lc->tcp);
	lc->io_mod = NE_LISTEN_OS_EXT ;
	lc->operate_timeout = CONNECTION_TIMEOUT * 1000;
//	INIT_LIST_HEAD(&lc->wait_free) ;
//	ne_mutex_init(&lc->wf_lock) ;
	
#ifdef WIN32
	ne_listensrv_alloc_entry((ne_handle)lc,ne_cm_node_alloc,ne_cm_node_free,(cm_init) ne_iocp_node_init ) ;
#else 
	ne_listensrv_alloc_entry((ne_handle)lc,ne_cm_node_alloc,ne_cm_node_free,(cm_init) ne_tcpcm_init ) ;
#endif
	
}

static void srv_udt_init(struct  listen_contex *lc)
{
	memset(lc, 0, sizeof(*lc));
	init_udt_srv_node(&lc->udt);
	lc->io_mod = NE_LISTEN_UDT_STREAM ;
	lc->operate_timeout = CONNECTION_TIMEOUT * 1000;
	ne_listensrv_alloc_entry((ne_handle)lc,ne_cm_node_alloc,ne_cm_node_free,(cm_init )udt_clientmap_init) ;
//	INIT_LIST_HEAD(&lc->wait_free) ;
//	ne_mutex_init(&lc->wf_lock) ;
//	lc->session_release_entry =(ne_close_callback ) release_dead_node ;
}

//ע��tcp udtl����
NEINT32 register_listensrv(void)
{
	NEINT32 ret ;
	struct ne_handle_reginfo reginfo ;
	reginfo.object_size = sizeof(struct  listen_contex ) ;
	reginfo.close_entry = (ne_close_callback )ne_listensrv_close ;
	
	//tcp connector register 
	reginfo.init_entry = (ne_init_func )srv_tcp_init ;
	strcpy(reginfo.name, "listen-tcp" ) ;
	
	ret = ne_object_register(&reginfo) ;
	if(-1==ret) {
		ne_logerror("register tcp-listen server error") ;
		return -1 ;
	}

	
	//udt connector register 
	reginfo.init_entry =(ne_init_func ) srv_udt_init ;
	strcpy(reginfo.name, "listen-udt" ) ;
	
	ret = ne_object_register(&reginfo) ;
	if(-1==ret) {
		ne_logerror("register udtlisten server error") ;
		return -1 ;
	}
	
	//ext connector register 
	reginfo.init_entry =(ne_init_func ) srv_ext_init ;
	strcpy(reginfo.name, "listen-ext" ) ;
	
	ret = ne_object_register(&reginfo) ;
	if(-1==ret) {
		ne_logerror("register ext-listen server error") ;
		return -1 ;
	}
	return ret ;
}
