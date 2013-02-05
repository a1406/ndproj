#define NE_IMPLEMENT_HANDLE
typedef struct netui_info *ne_handle;

#include "ne_common/ne_common.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_net/ne_netlib.h"

NEINT32 tcp_client_close_fin(struct ne_client_map* cli_map, NEINT32 force)
{
	NEINT32 ret = 0 ;
	NEUINT16  session = cli_map->connect_node.session_id ;
	struct ne_srv_node *root = (struct ne_srv_node *) (cli_map->connect_node.srv_root);

	ret = root->conn_manager.deaccept (&root->conn_manager, session) ;
	if(0!=ret) {
			//addto_wait_free((ne_session_handle)cli_map) ;
		TCPNODE_STATUS(cli_map) = ETS_DEAD ;
		return ret ;
	}
	ne_tcpnode_close(&(cli_map->connect_node) , force) ;		//关闭socket
	root->conn_manager.dealloc (cli_map, root->cm_alloctor);
	return (ret);
}

NEINT32 tcp_client_close(struct ne_client_map* cli_map, NEINT32 force)
{	
	NEINT32 ret = 0 ;
	NEUINT16  session = cli_map->connect_node.session_id ;
	struct ne_srv_node *root = (struct ne_srv_node *) (cli_map->connect_node.srv_root);


#if 1
	if(root->pre_out_callback) {
		ret = root->pre_out_callback(cli_map,(ne_handle)root) ;
		if(0!=ret) {
			//add to free list
			//addto_wait_free((ne_session_handle)cli_map) ;
			TCPNODE_STATUS(cli_map) = ETS_DEAD ;
			return ret ;
		}
	}
	if(!root->connect_out_callback)	{
		return (tcp_client_close_fin(cli_map, force));
	} else
		root->connect_out_callback(cli_map,(ne_handle)root) ;		
	return 0 ;
#else 
	if(root->connect_in_callback)	
		root->connect_out_callback(cli_map,(ne_handle)root) ;
	ne_tcpnode_close(&(cli_map->connect_node) , force) ;
//	cli_map->status = ESC_CLOSED ;

	root->conn_manager.deaccept (&root->conn_manager, session) ;
	root->conn_manager.dealloc (cli_map, root->cm_alloctor);
#endif //0
	return 0 ;
}

NEINT32 ne_session_close(ne_session_handle cli_handle, NEINT32 force)
{
//	neprintf("close)
	ne_assert(cli_handle && cli_handle->close_entry) ;
	return cli_handle->close_entry(cli_handle, force) ;
}
/*
NEINT32 ne_session_sendex(ne_session_handle cli_handle, ne_packhdr_t *msg_buf, NEINT32 flag) 
{
	return ne_connector_send(cli_handle,msg_buf, flag) ;
}	
*/
NEINT32 ne_session_flush_sendbuf(ne_session_handle cli_handle, NEINT32 flag) 
{
	ne_netui_handle h_header =(ne_netui_handle)cli_handle ;
	ne_assert(h_header) ;
	h_header->myerrno = NEERR_SUCCESS ;
	if(h_header->nodetype==NE_TCP){
		struct ne_client_map *cli_map =(struct ne_client_map *)cli_handle;
		if(0== flag) 
			return ne_tcpnode_flush_sendbuf(&(cli_map->connect_node)) ;
		else if(1==flag)
			return ne_tcpnode_flush_sendbuf_force(&(cli_map->connect_node)) ;
		else if(2==flag)
			return ne_tcpnode_tryto_flush_sendbuf(&(cli_map->connect_node)) ;
		
	}
	/*else if(h_header->nodetype==NE_UDT) {
		struct ne_udtcli_map *cli_map = (struct ne_udtcli_map *) cli_handle;
		ne_assert(h_header->length==sizeof(ne_udt_node));		

	}*/
	return 0 ;	
	
}

NEINT32 check_operate_timeout(ne_session_handle nethandle, netime_t tmout)
{
	netime_t interval = ne_time() - nethandle->last_recv;
	if(interval >= tmout) {
		return 1 ;
	}
	return 0 ;
}

//return 0 nothing to be done 
// -1 nethandle closed and node freed 
// else socket in error wait to close next time
NEINT32 tryto_close_tcpsession(ne_session_handle nethandle, netime_t connect_tmout )
{
	NEINT32 ret = 0 ;
	if(TCPNODE_STATUS(nethandle)==ETS_DEAD) {
		ret = tcp_release_death_node(nethandle,0) ;		//释放死连接
		if(0==ret) {
			return -1 ;
		}
	}
	else if(TCPNODE_CHECK_RESET(nethandle)) {
		if(nethandle->myerrno==NEERR_IO) {
			NE_CLOSE_REASON(nethandle) = ECR_SOCKETERROR ;
		}
		else {
			NE_CLOSE_REASON(nethandle) = ECR_SENDBLOCK ;
		}
		ne_session_close(nethandle,1) ;
		++ret ;
	}
	else if(TCPNODE_CHECK_CLOSED(nethandle)) {
		NE_CLOSE_REASON(nethandle) = ECR_NORMAL;
		ne_session_close(nethandle,0) ;
		++ret ;
	}
	else if(check_operate_timeout(nethandle, connect_tmout)) {
		NE_CLOSE_REASON(nethandle) = ECR_TIMEOUT ;
		ne_session_close(nethandle,0) ;
		++ret ;
	}
	return ret ;
}

static void broadcast_callback(ne_handle session_handle, void *param)
{
	ne_assert(session_handle) ;
	ne_assert(param) ;
	ne_sessionmsg_sendex(session_handle,param, ESF_NORMAL) ;
}

NEINT32 ne_session_broadcast(ne_handle listen_handle, ne_usermsghdr_t *msg, NEUINT16 send_sid) 
{
	ne_handle client;
	cmlist_iterator_t cm_iterator ;
	struct cm_manager *pmanger  ;
	
	pmanger = ne_listensrv_get_cmmamager(listen_handle) ;
	ne_assert(pmanger);
//	pmanger->walk_node_entry(pmanger, broadcast_callback, (void*)msg) ;
#ifdef SINGLE_THREAD_MOD
//	if(((struct listen_contex*)listen_handle)->single_thread_mod) {		
	for(client = pmanger->lock_first (pmanger,&cm_iterator) ; client; 
		client = pmanger->lock_next (pmanger,&cm_iterator) ) {
		if(((ne_netui_handle)client)->session_id != send_sid ) 
			ne_sessionmsg_sendex(client, msg, ESF_NORMAL) ;
	}
//}
#else
//	else {
#ifndef WIN32
	if(send_sid) {
		pmanger->unlock(pmanger,send_sid) ;
	}
#endif

	for(client = pmanger->lock_first (pmanger,&cm_iterator) ; client; 
		client = pmanger->lock_next (pmanger,&cm_iterator) ) {
		ne_sessionmsg_sendex(client,msg, ESF_NORMAL) ;
	}
					
#ifndef WIN32
	if(send_sid) {
		pmanger->lock(pmanger,send_sid) ;
	}
#endif
#endif  //SINGLE_THREAD_MOD
//	}
	return 0 ;
}
#if 0 
//加入到等待释放队列
void addto_wait_free(ne_session_handle nethandle)
{
	struct list_head *pos ;
	struct listen_contex *lc =(struct listen_contex *) (nethandle->srv_root );
	ne_assert(lc) ;

	if(nethandle->nodetype==NE_UDT) {
		pos = & ((struct ne_udtcli_map *) nethandle)->map_list ;
	}
	else {
		pos = & ((struct ne_client_map *) nethandle)->map_list ;
	}
	ne_mutex_lock(&lc->wf_lock);
		list_add_tail(pos, &lc->wait_free) ;
	ne_mutex_unlock(&lc->wf_lock);
	
}
#endif 
