#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_app.h"

neatomic_t __current_num = 0 ;
NEINT32 accept_entry(NE_SESSION_T nethandle, SOCKADDR_IN *addr, ne_handle h_listen) 
{
	NEINT32 old_num ;
	NEINT8  *pszTemp = ne_inet_ntoa( addr->sin_addr.s_addr );
#ifdef SINGLE_THREAD_MOD

#else 
	player_header_t *ph = ne_session_getdata((ne_netui_handle )nethandle) ;

	((ne_netui_handle )nethandle)->level = EPL_CONNECT ;		//set privilage
	ph->h_session = (ne_netui_handle )nethandle ;
	ph->id = 0 ;
	ph->status = EPS_CONNECT ;
#endif 
	old_num = ne_atomic_read(&__current_num) ;
	ne_atomic_inc(&__current_num);
	neprintf(_NET("%s %d: Connect from [%s:%d] \tcurrent connect num=%d old_num=%d\n"), 
		__FUNCTION__, __LINE__, pszTemp, htons(addr->sin_port),__current_num,old_num);


	return 0 ;
}

void close_entry(NE_SESSION_T nethandle, ne_handle h_listen) 
{
#ifndef SINGLE_THREAD_MOD
	player_header_t *ph = ne_session_getdata((ne_netui_handle )nethandle) ;
	//NEINT8  *pszTemp = ne_inet_ntoa( cli_map->connect_node.remote_addr.sin_addr.s_addr );
	//neprintf(_NET("%s:%d closed \n"), pszTemp, htons(cli_map->connect_node.remote_addr.sin_port));
	ne_atomic_dec(&__current_num);
	neprintf(_NET("net CLOSED for %s error =%s \t current connect num=%d\n"),
		ne_connect_close_reasondesc((ne_netui_handle )nethandle), ne_object_errordesc(nethandle) , __current_num);

	
	ph->status = EPS_NONE;
	
	((ne_netui_handle )nethandle)->level = EPL_NONE ;		//set privilage
#endif 

}

//关闭预处理函数
NEINT32 pre_close_entry(NE_SESSION_T nethandle, ne_handle h_listen) 
{
#ifndef SINGLE_THREAD_MOD
	player_header_t *ph = ne_session_getdata((ne_netui_handle )nethandle) ;
	//NEINT8  *pszTemp = ne_inet_ntoa( cli_map->connect_node.remote_addr.sin_addr.s_addr );
	//neprintf(_NET("%s:%d closed \n"), pszTemp, htons(cli_map->connect_node.remote_addr.sin_port));
	//ne_atomic_dec(&__current_num);
	neprintf(_NET("before close %d session current connect num=%d\n"),((ne_netui_handle )nethandle)->session_id,	 __current_num);

	
	//ph->status = EPS_NONE;
	
	//((ne_netui_handle )nethandle)->level = EPL_NONE ;		//set privilage
#endif
	return 0 ;
}


MSG_ENTRY_INSTANCE(srv_echo_handler)
{
//	neprintf("echo message %s\n", NE_USERMSG_DATA(msg)) ;
	NE_MSG_SEND(nethandle,msg,h_listen) ;
	return 0;
}

MSG_ENTRY_INSTANCE(srv_broadcast_handler)
{
	NE_BROAD_CAST(h_listen,msg, nethandle) ;

	return 0 ;
	
}

struct cm_manager * get_cm_manager()
{
	ne_handle listen_haneld = get_listen_handle () ;
	return ne_listensrv_get_cmmamager(listen_haneld) ;
}

