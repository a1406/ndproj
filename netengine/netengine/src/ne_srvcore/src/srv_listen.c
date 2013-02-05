#define IMPLEMENT_LISTEN_HANDLE

//#define USER_UPDATE_LIST 1			//把连接好的使用LIST管理

#include "ne_common/ne_common.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_net/ne_netlib.h"
#include "ne_common/ne_alloc.h"

#if defined(WIN32)
extern NEINT32 ne_start_iocp_listen(struct listen_contex *listen_info) ;
#elif defined(__LINUX__)
extern NEINT32 _linux_listen_srv(struct listen_contex *listen_info) ;
#endif
NEINT32 listen_service_common(struct listen_contex *listen_info);
NEINT32 common_update_entry(struct listen_contex *listen_info);
void host_congest(nesocket_t fd) ;
NEINT32 _open_udt(NEINT32 port, struct listen_contex *li_contex) ; 
NEINT32 update_udt_entry(struct listen_contex *listen_info);

#ifdef USER_UPDATE_LIST
static NEINT32 __close_node(struct ne_client_map *client_map , NEINT32 force)
{	
	TCPNODE_SET_CLOSED(client_map) ;
	return 0 ;
}
#endif
NEINT32 ne_listensrv_close(ne_listen_handle handle, NEINT32 flag) 
{
	if(handle->sub_id) {
		ne_thsrv_destroy(handle->sub_id, flag) ;
	}

	if(handle->listen_id) {
		ne_thsrv_destroy(handle->listen_id, flag) ;
	}
#ifdef SINGLE_THREAD_MOD
	if(handle->mqc.thid) {
		ne_thsrv_destroy(handle->mqc.thid, flag) ;
	}
#endif	
	if(_IS_UDT_MOD(handle->tcp.type)) {
		udt_close_srv(&handle->udt,flag) ;		
	}
	else {
		ne_tcpsrv_close(&handle->tcp ) ;
	}

	if(handle->tcp.cm_alloctor) {
		ne_cm_allocator_destroy(ne_listensrv_get_cmallocator(handle) , flag) ;
		handle->tcp.cm_alloctor = 0 ;
	}
	ne_swap_list_destroy(&handle->wait_free) ;
#ifdef USER_UPDATE_LIST
	ne_swap_list_destroy(&handle->wait_add) ;
#endif
	cm_destroy(ne_listensrv_get_cmmamager(handle)) ;
	return 0 ;
}

NEINT32 ne_listensrv_session_info(ne_listen_handle handle, NEINT32 max_client,size_t session_size)
{
	ne_handle cm_handle;
	handle->tcp.myerrno = NEERR_SUCCESS;
	if(0 == session_size) {
		session_size = ne_getclient_hdr_size(handle->io_mod);
	}
	cm_handle = ne_cm_allocator_create(max_client, session_size);
	if(!cm_handle) {
		handle->tcp.myerrno = NEERR_NOSOURCE;
		return -1 ;
	}
	handle->tcp.cm_alloctor = cm_handle;
	return 0;
}

NEINT32 ne_listensrv_open(NEINT32 port, ne_listen_handle handle) 
{
	NEINT32 ret ,io_mode;
	NEINT32 max_cm = 1024  ;
	struct cm_manager* pmanger;
	struct ne_thsrv_createinfo ls_info = {
		SUBSRV_RUNMOD_STARTUP,	//srv_entry run module (ref e_subsrv_runmod)
		NULL,			//service main entry function address
		handle,					//param of srv_entry 
		NULL ,					//handle received message from other thread!
		NULL,					//clean up when server is terminal
		_NET("listen"),			//service name
		handle					//user data
	};
	ne_assert(handle) ;

	io_mode = handle->io_mod ;
	
	pmanger = ne_listensrv_get_cmmamager(handle) ;
	max_cm = ne_cm_allocator_capacity(ne_listensrv_get_cmallocator(handle)) ;

	if(max_cm <= 0)
		max_cm = MAX_CLIENTS ;

	cm_listen(pmanger,  max_cm) ;

	if(_IS_UDT_MOD(io_mode)) {
		ret = _open_udt(port, handle) ;
		if(-1==ret) {
			handle->udt.myerrno = NEERR_OPENFILE ;
			return -1 ;
		}
		ls_info.srv_entry = (ne_threadsrv_entry )update_udt_entry ;
	}
	else if(NE_LISTEN_OS_EXT==io_mode) {
		ret = ne_tcpsrv_open(port,10,&(handle->tcp) );
		if(-1==ret) {
			handle->tcp.myerrno = NEERR_OPENFILE ;
			return -1 ;
		}
		handle->tcp.status = 1;
#ifdef WIN32
		//user iocp
		ls_info.srv_entry = ne_start_iocp_listen ;
#else 
		ls_info.srv_entry = (ne_threadsrv_entry)_linux_listen_srv ;
#endif
	}
	else {
		struct ne_thsrv_createinfo subth_info = {
			SUBSRV_RUNMOD_LOOP,	//srv_entry run module (ref e_subsrv_runmod)
			(ne_threadsrv_entry)common_update_entry,			//service main entry function address
			handle,		//param of srv_entry 
			NULL ,					//handle received message from other thread!
			NULL,					//clean up when server is terminal
			_NET("update_connect"),			//service name
			handle		//user data
		};
		ret = ne_tcpsrv_open(port,10,&(handle->tcp) );
		if(-1==ret) {
			handle->tcp.myerrno = NEERR_OPENFILE ;
			return -1 ;
		}
#ifdef USER_UPDATE_LIST
		INIT_LIST_HEAD(&handle->conn_list) ;
		ne_swap_list_init(&handle->wait_add) ;
#endif
		handle->sub_id = ne_thsrv_createex(&subth_info,NET_PRIORITY_HIGHT,1) ;
		if(!handle->sub_id )
			return -1;
		handle->tcp.status = 1;
		ls_info.srv_entry = (ne_threadsrv_entry)listen_service_common ;

	}
	
	handle->listen_id = ne_thsrv_createex(&ls_info,NET_PRIORITY_HIGHT,0) ;
	return handle->listen_id?0:-1;
}
#ifndef SINGLE_THREAD_MOD
void ne_listensrv_set_entry(ne_listen_handle handle, accept_callback income,pre_deaccept_callback pre_close, deaccept_callback outcome) 
{
//	ne_assert(handle->single_thread_mod==0) ;
	
	if(income)
		handle->tcp.connect_in_callback = income ;
	if(outcome)
		handle->tcp.connect_out_callback = outcome ;
	if(pre_close)
		handle->tcp.pre_out_callback = pre_close ;
}
#endif
void ne_listensrv_alloc_entry(ne_listen_handle handle, cm_alloc al, cm_dealloc dealloc, cm_init init) 
{	
	if(al) handle->tcp.conn_manager.alloc = al ;
	if(dealloc) handle->tcp.conn_manager.dealloc = dealloc ;
	if(init) handle->tcp.conn_manager.init = init ;
}

NEINT32 listen_service_common(struct listen_contex *listen_info)
{
	NEINT32 ret ;
	//netime_t tm_start ;
	struct ne_client_map *accepted;//, *client;
	//ne_thsrvid_t update_id ;
	ne_handle context = ne_thsrv_gethandle(listen_info->listen_id) ;
	
	ne_assert(context) ;
	if(-1==ne_socket_nonblock(listen_info->tcp.fd,1)) {
		ne_logerror("set socket nonblock ") ;
		return -1 ;
	}
	if(listen_info->sub_id) {
		ne_thsrv_resume(listen_info->sub_id) ;
	}
	while (!ne_thsrv_isexit(context)){
		
		ret = ne_socket_wait_read(listen_info->tcp.fd,100) ;
		if(ret> 0) {
			accepted = accetp_client_connect(listen_info) ;
#ifdef USER_UPDATE_LIST
			if(accepted) {
				accepted->connect_node.close_entry = __close_node ;
				ne_mutex_lock(&listen_info->wait_add.lock) ;
					list_add_tail(&accepted->map_list, &listen_info->wait_add.list) ;
				ne_mutex_unlock(&listen_info->wait_add.lock) ;
			}
#endif 
		}

		if(!ne_thsrv_msghandler(context) )		//处理线程消息
			break ;
	}
	return 0 ;
}

/* entry of  listen service */
NEINT32 common_update_entry(struct listen_contex *listen_info)
{
	NEINT32 ret  ,sleep = 0 ;

	struct cm_manager *pmanger  ;
	struct ne_client_map  *client;
	
	NEUINT16 session_id = 0;
#ifdef USER_UPDATE_LIST
	struct list_head *pos ;
	swap_list_t *swap ;
#else 
//	NEINT32 i, num ;
	cmlist_iterator_t cm_iterator ;
#endif 
	
	pmanger = ne_listensrv_get_cmmamager(listen_info) ;	

	if(ne_atomic_read(&pmanger->connect_num) <= 0) {
		ne_sleep(100) ;
		return 0 ;
	}
	
#ifdef USER_UPDATE_LIST
	swap = &listen_info->wait_add ;
	if(0==ne_mutex_trylock(&swap->lock) ) {
		list_join(&swap->list, &listen_info->conn_list) ;
		INIT_LIST_HEAD(&swap->list) ;
		ne_mutex_unlock(&swap->lock) ;
	}
	
	pos = listen_info->conn_list.next ;
	while (pos!=&listen_info->conn_list) {
		client = list_entry(pos,struct ne_client_map , map_list) ;
		pos = pos->next ;
		
		session_id = client->connect_node.session_id ;
		client = pmanger->trylock(pmanger, session_id) ;
		if(client) {
			++sleep;
			ret = tryto_close_tcpsession((ne_session_handle)client, listen_info->operate_timeout ) ;
			if(ret) {
				if(-1==ret)	list_del_init(&client->map_list) ;
				pmanger->unlock(pmanger,session_id) ;
				continue ;
			}
			ret = ne_do_netmsg(client,&listen_info->tcp) ;					
			if(ret > 0) {
				ne_tcpnode_flush_sendbuf(&(client->connect_node)) ;
			}
			else if(ret ==0){
				if(0==tcp_client_close(client,1) ) {
					list_del_init(&client->map_list) ;
				}
			}
			pmanger->unlock(pmanger,session_id) ;
		}
		
	}
#else 
//	num = ne_atomic_read(&pmanger->connect_num);
//	i = 0 ;
	for(client = pmanger->lock_first (pmanger,&cm_iterator) ; client;	client = pmanger->lock_next (pmanger,&cm_iterator) ) {
		if(tryto_close_tcpsession((ne_session_handle)client, listen_info->operate_timeout ) ) {
			continue ;
		}
		ret = ne_do_netmsg(client,&listen_info->tcp) ;
		if(ret>0) {
			ne_tcpnode_flush_sendbuf(&(client->connect_node)) ;
			sleep++ ;
		}
		else if(0==ret) {
			tcp_client_close(client,1) ;
		}
	}
#endif
	if(!sleep ){
		ne_sleep(100);
	}
	return 0;
}

/* accept incoming  connect*/
struct ne_client_map * accetp_client_connect(struct listen_contex *listen_info)
{
	NEUINT16  session_id ;
	nesocket_t newconnect_fd ;
	socklen_t cli_len ;				/* client socket lenght */
	struct sockaddr_in client_addr ;
	
	struct ne_client_map *client_map ;
	struct cm_manager *pmanger = ne_listensrv_get_cmmamager(listen_info) ;

	cli_len = sizeof (client_addr);
	newconnect_fd = accept(get_listen_fd(listen_info), (struct sockaddr*)&client_addr, &cli_len);
	if(newconnect_fd < 0)
		return 0 ;
	
	//alloc a connect node struct 	
	client_map =(struct ne_client_map*) pmanger->alloc (ne_listensrv_get_cmallocator(listen_info)) ;
	if(!client_map){
		host_congest(newconnect_fd) ;
		//ne_logerror("server congest!") ;
		return 0;
	}
	if(pmanger->init )
		pmanger->init (client_map, (ne_handle)listen_info) ;
	else 
		ne_tcpcm_init(client_map,(ne_handle)listen_info);

	if(-1== ne_socket_nonblock(newconnect_fd,1)) {
		ne_socket_close(newconnect_fd);
		pmanger->dealloc (client_map,ne_listensrv_get_cmallocator(listen_info));
		return 0 ;
	}
	_set_socket_addribute(newconnect_fd) ;

	client_map->connect_node.fd = newconnect_fd ;
//	client_map->connect_node.remote_len = cli_len ;
	memcpy(&(client_map->connect_node.remote_addr),&client_addr, sizeof(client_addr)) ;
	TCPNODE_SET_OK(&client_map->connect_node) ;
	client_map->connect_node.srv_root = (ne_handle)&(listen_info->tcp) ;

	client_map->connect_node.session_id = session_id = pmanger->accept (pmanger,client_map);
	if(0==client_map->connect_node.session_id) {
		ne_socket_close(newconnect_fd);
		pmanger->unlock (pmanger,session_id);
		pmanger->dealloc (client_map,ne_listensrv_get_cmallocator(listen_info));
		return 0;
	}

	if(listen_info->tcp.connect_in_callback){		
		if(-1==listen_info->tcp.connect_in_callback(client_map,&client_addr,(ne_handle)listen_info) ){
			ne_socket_close(newconnect_fd);
			pmanger->deaccept (pmanger,client_map->connect_node.session_id);
			
			pmanger->unlock (pmanger,session_id);
			pmanger->dealloc (client_map,ne_listensrv_get_cmallocator(listen_info));
			return 0 ;
		}
	}
	client_map->connect_node.start_time = ne_time();
	client_map->connect_node.last_recv = ne_time(); 
	INIT_LIST_HEAD(&client_map->map_list) ;
	
	pmanger->unlock (pmanger,client_map->connect_node.session_id);
	return client_map ;

}

/*通知client服务器拥挤,并且关闭*/
void host_congest(nesocket_t fd)
{
	ne_socket_close(fd) ;
}


//TCP消息处理函数
//这是一个接口转换函数,把 NENET_MSGENTRY 转换成 ne_connect_msg_entry 
NEINT32 srv_stream_data_entry(void *node,ne_packhdr_t *msg,void *param)
{
	NEINT32 ret = 0 ;
	struct listen_contex *lc = (struct listen_contex *) param ;
	ne_assert(node) ;
	ne_assert(param) ;

	
	if(msg->encrypt) {
		NEINT32 new_len ,old_len;
		new_len = ne_packet_decrypt(node, (ne_packetbuf_t*)msg) ;
		if(new_len==0) 
			return -1;
		old_len = (NEINT32) ne_pack_len(msg) ;
		ne_pack_len(msg) = (NEUINT16)new_len ;
//		if(0==lc->single_thread_mod)
#ifndef SINGLE_THREAD_MOD
		ret = ne_srv_translate_message(param,(ne_handle)node, msg ) ;
//		else 
#else
		ret = ne_singleth_msghandler(node, msg,param) ;
#endif		
		ne_pack_len(msg) = old_len ;
	}
	else {
//		if(0==lc->single_thread_mod)
#ifndef SINGLE_THREAD_MOD		
		ret = ne_srv_translate_message(param,(ne_handle)node, msg ) ;
//		else
#else		
		ret = ne_singleth_msghandler(node, msg, param);
#endif			
	}
	return ret ;
}

/*deal with received net message
 * return 0 connect closed
 * return -1 nothing to be done 
 * else return received data length
 */
NEINT32 ne_do_netmsg(struct ne_client_map *cli_map,struct ne_srv_node *srv_node) 
{
	
	NEINT32 read_len,ret = 0;
	ne_assert(cli_map) ;
	ne_assert(check_connect_valid(& (cli_map->connect_node))) ;
//	ne_assert(cli_map->status != ESC_CLOSED ) ;
	
RE_READ:
	read_len = ne_tcpnode_read(& (cli_map->connect_node)) ;
	if(read_len == 0) {
		NE_CLOSE_REASON(cli_map) = ECR_READOVER ;
		return 0 ;
	}
	else if(-1== read_len) {
		if(cli_map->connect_node.myerrno==NEERR_WUOLD_BLOCK) {
			return -1 ;
		}
		else {
			NE_CLOSE_REASON(cli_map) = ECR_SOCKETERROR ;
			return 0 ;		//need closed
		}
	}
	else {
		ret += read_len ;
		if(-1 == tcpnode_parse_recv_msgex(&(cli_map->connect_node), srv_stream_data_entry, (void*)srv_node) ){
			return 0 ;
		}
		if(TCPNODE_READ_AGAIN(&(cli_map->connect_node))) {
			/*read buf is to small , after parse data , read again*/
			/*read_len = ne_tcpnode_read(& (cli_map->connect_node)) ;
			
			if(read_len>0){
				//NE_RUN_HERE();
				ret += read_len ;				
			}*/
			goto RE_READ;
			/*else if(0==read_len){
				return 0;
			}*/
		}
	}
	return ret ;
	
}

ne_handle ne_listensrv_get_cmallocator(struct listen_contex * handle) 
{
	return handle->tcp.cm_alloctor ;
}

//得到连接管理器
struct cm_manager *ne_listensrv_get_cmmamager(struct listen_contex * handle) 
{
	return &(handle->tcp.conn_manager );
}

//在监听器上建立定时器
netimer_t ne_listensrv_timer(ne_listen_handle h_listen,ne_timer_entry func,void *param,netime_t interval, NEINT32 run_type ) 
{
//	if(h_listen->single_thread_mod) {
#ifdef SINGLE_THREAD_MOD	
	return ne_thsrv_timer(h_listen->mqc.thid, func, param, interval, run_type) ;
//	}
//	else {
#else	
	if (h_listen->sub_id) {
		return ne_thsrv_timer(h_listen->sub_id, func, param, interval, run_type) ;
	}
	else if (h_listen->listen_id) {
		return ne_thsrv_timer(h_listen->listen_id, func, param, interval, run_type) ;
	}
//	}
#endif	//SINGLE_THREAD_MOD
	
	return 0 ;
}

void ne_listensrv_del_timer(ne_listen_handle h_listen, netimer_t timer_id ) 
{
#ifdef SINGLE_THREAD_MOD		
//	if(h_listen->single_thread_mod) {
	ne_thsrv_del_timer(h_listen->mqc.thid, timer_id) ;
//	}
//	else {
#else
	if(h_listen->sub_id) {
		ne_thsrv_del_timer(h_listen->sub_id, timer_id) ;
	}
	else if(h_listen->listen_id) {
		ne_thsrv_del_timer(h_listen->listen_id, timer_id) ;
	}
#endif	// SINGLE_THREAD_MOD
//  }
	
}

#undef  IMPLEMENT_LISTEN_HANDLE
