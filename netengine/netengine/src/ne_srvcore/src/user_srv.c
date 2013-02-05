/*
 * 打开一个专门的线程来调用用户函数处理网络消息和定时器事件
 * 这样做的目的可以使整个服务器逻辑看起来和单线程是一样的
 * 但是消息处理函数和发送函数在用法上将有所不同.
 * 使用单独的用户线程处理消息时,每次处理消息将不在锁定每个消息对于的session.
 * 并且在消息处理函数中只能使用sessionid.
 * 两种处理方式在编程习惯用法上将会有一些不相容的地方.
 */

#include "ne_common/ne_common.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_net/ne_netlib.h"
#include "ne_common/ne_alloc.h"

#include "ne_srvcore/ne_userth.h"

struct conn_info{
	size_t session_id ;
	SOCKADDR_IN addr ;
};

//把消息添加到队列中
NEINT32 addto_queue(ne_netui_handle  nethandle, NEUINT16 msgkind, ne_usermsgbuf_t *msg,ne_handle h_listen)
{
//	static NEUINT32 _msg_index =0 ;
	struct _msg_queuq_context *mqc = &((struct listen_contex*)h_listen)->mqc ; 

	if(E_MSG_KINE_DATA==msgkind) {
		NEUINT32  size ;
		
		if(!msg) {
			return -1 ;
		}
		size = NE_USERMSG_LEN(msg) ;

		if(size==0 || size>NE_PACKET_SIZE) {
			return -1 ;
		}
		NE_USERMSG_LEN(msg) = nethandle->session_id ;
		return  ne_thsrv_send(mqc->thid,msgkind,msg, size) ;
	}
	else if(E_MSG_KINE_ACCEPT==msgkind) {
		struct conn_info conn  ;
		conn.session_id = (size_t) nethandle->session_id  ;
		memcpy(&conn.addr, msg, sizeof(SOCKADDR_IN )) ;
		return ne_thsrv_send(mqc->thid,msgkind,&conn, sizeof(conn)) ;
	}
	else if(E_MSG_KINE_CLOSE==msgkind) {
		size_t sid = (size_t) nethandle->session_id  ;
		return ne_thsrv_send(mqc->thid,msgkind,&sid, sizeof(sid)) ;
	}
	return 0 ;
}

static NEINT32 internal_accept(ne_netui_handle nethandle, SOCKADDR_IN *addr, ne_handle h_listen) 
{
	NEINT32 old_num ;
	NEINT8 *pszTemp = ne_inet_ntoa( addr->sin_addr.s_addr );
	struct _msg_queuq_context *mqc = &((struct listen_contex*)h_listen)->mqc ; 
	
	ne_assert(mqc) ;

	nethandle->level = EPL_CONNECT ;		//set privilage
	
	
	old_num = ne_atomic_read(&mqc->connect_num) ;
	ne_atomic_inc(&mqc->connect_num);
	neprintf(_NET("%s %d: Connect from [%s:%d] \tcurrent connect num=%d old_num=%d\n"), 
		__FUNCTION__, __LINE__, pszTemp, htons(addr->sin_port),mqc->connect_num,old_num);

	return addto_queue( nethandle,E_MSG_KINE_ACCEPT, (ne_usermsgbuf_t *)addr, h_listen) ;

}

static void internal_close(ne_netui_handle nethandle, ne_handle h_listen) 
{	
	struct _msg_queuq_context *mqc = &((struct listen_contex*)h_listen)->mqc ; 

	//NEINT8  *pszTemp = ne_inet_ntoa( cli_map->connect_node.remote_addr.sin_addr.s_addr );
	//neprintf(_NET("%s:%d closed \n"), pszTemp, htons(cli_map->connect_node.remote_addr.sin_port));
	ne_atomic_dec(&mqc->connect_num);
	neprintf(_NET("net CLOSED for %s error =%s \t current connect num=%d\n"),
		ne_connect_close_reasondesc(nethandle), ne_object_errordesc((ne_handle)nethandle) , mqc->connect_num);

	addto_queue( nethandle,E_MSG_KINE_CLOSE, NULL, h_listen) ;
	
	nethandle->level = EPL_NONE ;		//set privilage

}
/*
//关闭预处理函数
static NEINT32 internal_pre_close(ne_handle nethandle, ne_handle h_listen) 
{
	return 0 ;
}
*/
static NEINT32 _msg_func(struct ne_thread_msg *msg)
{
	
//static NEINT32 _msg_index = 0;
//	struct cm_manager *pmanger  ;
	struct listen_contex *lc = (struct listen_contex *) msg->th_userdata ;
#ifndef SINGLE_THREAD_MOD
	return (0);
#endif
	if(!lc) {	
		return 0 ;
	}
	if(E_MSG_KINE_DATA==msg->msg_id) {
		NEUINT16  session_id  ;
		ne_usermsgbuf_t *net_msg ;
		if(msg->data_len < NE_USERMSG_HDRLEN) {
			return 0 ;
		}
		net_msg = (ne_usermsgbuf_t *)msg->data ;

		session_id = NE_USERMSG_LEN(net_msg) ;
		NE_USERMSG_LEN(net_msg) = (NEUINT16) msg->data_len ;

		ne_srv_translate_message1( (ne_handle)lc, session_id, (ne_packhdr_t*) net_msg ) ;
		return 0 ;
	}
	else if(E_MSG_KINE_ACCEPT==msg->msg_id) {
		struct conn_info *conn = (struct conn_info*)(msg->data) ;		
		if(lc->mqc.income){
			NEINT32 ret = lc->mqc.income((NEUINT16)conn->session_id, &conn->addr, (ne_handle)lc)   ;
			if(-1==ret) {
				ne_st_close((NEUINT16)conn->session_id , 0, (ne_handle)lc);
			}
		}
	}
	else if(E_MSG_KINE_CLOSE==msg->msg_id) {
		size_t sid = *((size_t*)(msg->data)) ;
		if(lc->mqc.outcome)
			lc->mqc.outcome((NEUINT16)sid,(ne_handle)lc) ;
	}
	return 0 ;

}
#ifdef SINGLE_THREAD_MOD
/*设置为单线程模式*/
NEINT32 ne_listensrv_set_single_thread(ne_handle h_listen )
{
	struct ne_thsrv_createinfo srv_info = {0} ;
	struct listen_contex *lc = (struct listen_contex *) h_listen ;
	ne_mqc_t *mqc = &lc->mqc ;

	ne_assert(lc) ;

	if(lc->tcp.user_msg_entry) {
		ne_logfatal("use ne_listensrv_set_single_thread() before  ne_srv_msgtable_create() \n") ;
		return -1 ;
	}
	
	if(lc->tcp.connect_in_callback) {
		ne_logfatal("use ne_listensrv_set_single_thread() before  ne_listensrv_set_entry() \n") ;
		return -1 ;
	
	}
//	lc->single_thread_mod = 1 ;
	

	lc->tcp.connect_in_callback = (accept_callback )internal_accept;
	lc->tcp.connect_out_callback = (deaccept_callback ) internal_close;
	lc->tcp.pre_out_callback = NULL;

	//create thread 
	srv_info.run_module = SUBSRV_MESSAGE;					//srv_entry run module (ref e_subsrv_runmod)
	srv_info.msg_entry = _msg_func;				//handle received message from other thread!
	srv_info.data = h_listen ;
	nestrcpy(srv_info.srv_name, "UserMsgThread") ;

	mqc->thid = ne_thsrv_create(&srv_info) ;
	if(-1 == mqc->thid) {
		ne_logfatal("create UserMsgThread server error !") ;
		return -1 ;
	}
	return 0 ;
}


NEINT32 ne_listensrv_set_entry_st(ne_handle h_listen, st_accept_entry income, st_deaccept_entry outcome) 
{
	struct listen_contex *lc = (struct listen_contex *) h_listen ;
//	ne_assert(lc->single_thread_mod) ;
//	if(0 == lc->single_thread_mod) {
//		ne_logfatal("please call ne_listensrv_set_single_thread() before ne_listensrv_set_entry_st()!\n") ;
//		
//		return -1 ;
//	}
	
	lc->mqc.income = income ;
	lc->mqc.outcome = outcome ;
	return 0 ;
}

NEINT32 ne_st_send(NEUINT16 sessionid, ne_usermsghdr_t *msg, NEINT32 flag, ne_handle h_listen)
{
	NEINT32 ret = -1;
	ne_handle session ;
	struct listen_contex *lc = (struct listen_contex *) h_listen ;
	struct cm_manager *mamager = ne_listensrv_get_cmmamager(h_listen);
	
	ne_assert(mamager) ;

	session = mamager->lock(mamager, sessionid) ;
	if(session) {
		if(ne_connector_valid((ne_netui_handle) session) && sessionid == ((ne_netui_handle) session)->session_id)
			ret = ne_sessionmsg_sendex(session, msg, flag) ;
		mamager->unlock(mamager, sessionid) ;
	}
	return ret ; 	

}

NEINT32 ne_st_close(NEUINT16 sessionid ,  NEINT32 flag, ne_handle h_listen )
{
	NEINT32 ret = -1 ;
	ne_handle session ;
	struct listen_contex *lc = (struct listen_contex *) h_listen ;
	struct cm_manager *mamager = ne_listensrv_get_cmmamager(h_listen);
	
	ne_assert(mamager) ;
	session = mamager->lock(mamager, sessionid) ;
	if(session) {
		
		if(sessionid == ((ne_netui_handle)session)->session_id) 
			ret = ne_session_close(session, flag) ;
		mamager->unlock(mamager, sessionid) ;
	}
	return ret ; 	

}

void ne_st_set_crypt(NEUINT16 sessionid, void *key, NEINT32 size, ne_handle h_listen)
{
	ne_handle session ;
	struct listen_contex *lc = (struct listen_contex *) h_listen ;
	struct cm_manager *mamager = ne_listensrv_get_cmmamager(h_listen);
	
	ne_assert(mamager) ;

	session = mamager->lock(mamager, sessionid) ;
	if(session) {
		ne_connector_set_crypt(session,key, size) ;
		//ret =ne_session_close(session, flag) ;
		mamager->unlock(mamager, sessionid) ;
	}	
}
#endif //SINGLE_THREAD_MOD
