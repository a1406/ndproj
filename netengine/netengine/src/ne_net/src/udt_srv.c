#include "ne_common/ne_common.h"
#include "ne_common/ne_alloc.h"
#include "ne_net/ne_netlib.h"
//#include "ne_net/ne_udt.h"
//#include "ne_net/ne_udtsrv.h"
//#include <stdlib.h>

ne_udt_node *alloc_listen_socket(ne_udtsrv *root);
NEINT32 pump_insrv_udt_data(ne_udtsrv *root,struct neudt_pocket *pocket, NEINT32 len,SOCKADDR_IN *addr);

void init_udt_srv_node(ne_udtsrv *root)
{
	memset(root,0,sizeof(*root));
	root->size = sizeof(*root);
	root->type = UDP_SERVER_TYPE ;

	ne_mutex_init(&root->send_lock); 
	bzero(&root->self_addr,sizeof(root->self_addr) ); 	//自己的地址
	bzero(&root->conn_manager,sizeof(root->conn_manager));
}

NEINT32 udt_socketex(ne_udtsrv* udt_root )
{
	nesocket_t listen_fd  ;					/* listen and connect fd */
	
	//init_udt_srv_node(udt_root);

	/* zero all socket slot */
	listen_fd = socket(AF_INET, SOCK_DGRAM, 0) ;
	if(-1 == listen_fd){
		return -1;
	}
	udt_root->listen_fd = listen_fd ;
	return 0 ;

}
ne_udtsrv* udt_socket(NEINT32 af,NEINT32 type,NEINT32 protocol )
{
	nesocket_t listen_fd  ;					/* listen and connect fd */
	ne_udtsrv* udt_srv = (ne_udtsrv*) malloc(sizeof(ne_udtsrv)) ;
	if(!udt_srv){
		ne_showerror() ;
		return NULL;
	}
	
	init_udt_srv_node(udt_srv);

	/* zero all socket slot */
	listen_fd = socket(AF_INET, SOCK_DGRAM, 0) ;
	if(-1 == listen_fd){
		return NULL;
	}
	udt_srv->listen_fd = listen_fd ;
	return udt_srv ;
}

NEINT32 udt_bind(ne_udtsrv* listen_node ,const struct sockaddr *addr, NEINT32 namelen)
{
	NEINT32 ret ;
	if(!listen_node->listen_fd)
		return -1 ;

	ret = bind (listen_node->listen_fd, addr, sizeof(struct sockaddr)) ;
	if(ret) {
		memcpy(&listen_node->self_addr,addr,sizeof(*addr)) ;
	}
	return ret ;	
}

NEINT32 udt_listen(ne_udtsrv* listen_node,NEINT32 listen_number)
{
	// nothing to be done
	listen_node->listen_status = 1;
	cm_listen(&(listen_node->conn_manager), listen_number) ;
	return 0;
}

//创建并绑定一个udp服务端口
NEINT32 udt_open_srvex(NEINT32 port, ne_udtsrv *listen_root)
{	
	NEINT32 ret;
	SOCKADDR_IN serv_addr ={0} ;		/* socket address */
	
	if(-1==udt_socketex(listen_root)){
		return -1;
	}
	ne_sourcelog(listen_root, "udt_open_srv", "open udt port operate!") ;

	ne_socket_nonblock(listen_root->listen_fd,1) ;

	serv_addr.sin_family = AF_INET ;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
	serv_addr.sin_port = htons((NEUINT16)port);
	
	ret = udt_bind (listen_root, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ;	
	if(ret <0){
		udt_close_srvex(listen_root,1);
		return -1;
	}
	/*
	if(-1==udt_listen(listen_root, max_clients)) {
		udt_close_srvex(listen_root,1);
		return -1;
	}
	*/
	return 0 ;
}
//创建并绑定一个udp服务端口
ne_udtsrv* udt_open_srv(NEINT32 port)
{	
	ne_udtsrv *listen_root ;
	NEINT32 ret;
	SOCKADDR_IN serv_addr ={0} ;		/* socket address */
	
	listen_root = udt_socket(AF_INET, SOCK_DGRAM, 0) ;
	if(!listen_root){
		return NULL;
	}
	ne_sourcelog(listen_root, "udt_open_srv", "open udt port operate!") ;

	ne_socket_nonblock(listen_root->listen_fd,1) ;

	serv_addr.sin_family = AF_INET ;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
	serv_addr.sin_port = htons((NEUINT16)port);
	
	ret = udt_bind (listen_root, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ;	
	if(ret <0){
		udt_close_srv(listen_root,1);
		return NULL;
	}
	
	return listen_root ;
}

void udt_close_srv(ne_udtsrv *root , NEINT32 force)
{
	ne_source_release(root) ;

	cm_destroy(&root->conn_manager) ;
	
	ne_socket_close(root->listen_fd) ;
	ne_mutex_destroy(&root->send_lock); 
	
	//free(root);
}

void udt_close_srvex(ne_udtsrv *root , NEINT32 force)
{
	ne_source_release(root) ;

	cm_destroy(&root->conn_manager) ;
	
	ne_socket_close(root->listen_fd) ;
	ne_mutex_destroy(&root->send_lock); 
	
}

//设置数据到达通知函数
void udt_set_notify_entry(ne_udtsrv *root,data_notify_callback entry)
{
	ne_assert(root);
	root->data_notify_entry = entry ;
}
//read income data and accept new connect
NEINT32 udt_bindio_handler(ne_udtsrv *root, accept_callback accept_entry, data_recv_callback recv_entry,deaccept_callback release_entry)
{
	if(!root->listen_fd || !root->listen_status)
		return -1 ;
	
	if(-1==ne_socket_nonblock(root->listen_fd, 1) )
		return -1 ;

	root->connect_in_callback = accept_entry ;
	root->income_entry = recv_entry ;
	root->connect_out_callback = release_entry ;
	return 0;
}

/* 安装连接节点管理程序 */
void udt_install_connect_manager(ne_udtsrv *root,cm_alloc alloc_entry ,cm_dealloc dealloc_entry) 
{
	root->conn_manager.alloc = alloc_entry ;
	root->conn_manager.dealloc = dealloc_entry ;
}

//更新每一个节点
// return -1 ,closed , source already release
static NEINT32 _update_node(ne_udt_node *node)
{	
	if(NETSTAT_ESTABLISHED==node->conn_state){
		if (-1==update_socket(node)) {
			udt_reset(node,0) ;
			ne_assert(0) ;
			return 0 ;
		}
		//这里需要检测超时
	}
	
	if(node->conn_state < NETSTAT_ESTABLISHED /*&& node->conn_state>=NETSTAT_SYNSEND*/){
		//not accepted 
		netime_t val = ne_time() - node->last_recv ;
		if(val > node->retrans_timeout) {
			//timeout, need to be release
			release_dead_node(node,1) ;
			ne_assert(0) ;
			return -1 ;		
		}	
	}
	else if(NETSTAT_TIME_WAIT & node->conn_state) {
		release_dead_node(node,1) ;
		ne_assert(0) ;
		return -1 ;
	}

	else if(NETSTAT_TRYTORESET & node->conn_state){
		send_reset_packet(node) ;
		release_dead_node(node,1) ;
		node->conn_state &= (~NETSTAT_TRYTORESET) ;
		ne_assert(0) ;
		return -1;
	}
	else if(NETSTAT_RESET & node->conn_state) {
		release_dead_node(node,1) ;
		node->conn_state &= (~NETSTAT_RESET) ;
		ne_assert(0) ;
		return -1 ;
	}
	else if (NETSTAT_TRYTOFIN & node->conn_state) {
		_close_listend_socket(node) ;
	}

	else if(NETSTAT_FINSEND & node->conn_state && 
		!(NETSTAT_SENDCLOSE&node->conn_state)) {
		if(node->resend_times<MAX_ATTEMPT_SEND) {
			netime_t now = ne_time() ;
			if((now - node->last_active) > node->retrans_timeout) {
				_udt_fin(node);
			}
		}
		else {
			release_dead_node(node,1) ;
			ne_assert(0) ;
			return -1 ;
		}
	}
	else if(NETSTAT_RECVCLOSE & node->conn_state) {
		netime_t now = ne_time() ;
		if((now - node->last_recv) > (node->retrans_timeout *5)) {
			//_udt_fin(node);
			release_dead_node(node,1) ;
			ne_assert(0) ;
			return -1 ;
		}
	}
	return 0;
}

void update_all_socket(ne_udtsrv *root) 
{
	//check_timeout(root) ;
	//update_accept_socket(root) ;
	//update_connected_socket(root);
	cmlist_iterator_t cm_iter;
//	NEINT32 i,ret;
	//struct udt_connmgr_node *node =(struct udt_connmgr_node *) root->connmgr_addr ;
	struct cm_manager *conn_manager = &root->conn_manager ;
	ne_udt_node *node ;
	
	for(node = conn_manager->lock_first(conn_manager,&cm_iter) ; node; 
	node = conn_manager->lock_next(conn_manager,&cm_iter) ) {
		if(-1==_update_node(node) ) {
			conn_manager->deaccept(conn_manager,cm_iter.session_id) ;
		}
	}
}

//如果有数据进来就返回0
NEINT32 doneIO(ne_udtsrv *root, netime_t timeout)
{
	update_all_socket(root) ;
	return read_udt_handler(root, timeout) ;
}

/*处理输入的数据,如果有数据进来就返回0*/
NEINT32 read_udt_handler(ne_udtsrv *root, netime_t timeout)
{
	NEINT32 read_len ;
	struct neudt_pocket  *pocket ;
	SOCKADDR_IN  addr ;
	udt_pocketbuf readbuf ;
	
	//read from root listen socket
	read_len = _recvfrom_udt_packet(root->listen_fd, &readbuf, &addr, timeout) ;
	if(read_len <= 0)
		return -1 ;

	pocket =&(readbuf.pocket );
	if(POCKET_PROTOCOL(pocket) == PROTOCOL_UDT) {
		pump_insrv_udt_data(root, pocket, read_len,&addr);
		return 0;
	}
	else if(POCKET_PROTOCOL(pocket) == PROTOCOL_UDP){
		u_16 s_id ;
		struct neudp_packet *udp_packet;
		ne_udt_node *socket_node ;
		//ne_udt_node *socket_node = connmgr_find(root, POCKET_SESSIONID(pocket)) ;
		s_id = POCKET_SESSIONID(pocket) ;
		
		socket_node = root->conn_manager.trylock(&root->conn_manager, s_id) ;
		if(!socket_node) {
			return 0;
		}
		udp_packet = (struct neudp_packet *)pocket ;
		if(socket_node->datagram_entry){
			socket_node->datagram_entry(socket_node,
				udp_packet, read_len-UDP_PACKET_HEAD_SIZE,socket_node->callback_param) ;
		}
		root->conn_manager.unlock(&root->conn_manager,s_id) ;

		return 0;
	}
	else {
		ne_assert(0);
		return 0;		//incoming data error
	}
}

//处理进入服务器的消息
NEINT32 pump_insrv_udt_data(ne_udtsrv *root,struct neudt_pocket *pocket, NEINT32 len,SOCKADDR_IN *addr)
{
	ne_udt_node *socket_node = NULL;
	u_16 session_id = POCKET_SESSIONID(pocket) ;
	if(NEUDT_SYN==POCKET_TYPE(pocket)) {
		if(0==session_id) {
			//这里是否需要根据IP和端口来判断是否已经建立了在这个IP和端口上的连接
			//来防止一个类似syn的攻击
			socket_node = alloc_listen_socket(root) ;
			if(!socket_node)
				return -1 ;
			socket_node->session_id = root->conn_manager.accept(
							&root->conn_manager,socket_node);

			if(!socket_node->session_id) {
				release_dead_node(socket_node, 1) ;
				return -1 ;
			}
			//root->conn_manager.lock(&root->conn_manager,socket_node->session_id) ;		//accept 已经锁定了

			ne_assert(socket_node->conn_state==NETSTAT_LISTEN);
			memcpy(&socket_node->remote_addr,addr,sizeof(*addr)) ;
			socket_node->last_recv = ne_time() ;
			session_id = socket_node->session_id ;
		}
		else {
			socket_node = root->conn_manager.lock(&root->conn_manager,session_id) ;
			if(!socket_node)
				return -1;
			
		}
		
		if(-1==_handle_syn(socket_node,pocket) ) {
			udt_reset( socket_node, 0) ;
			//root->conn_manager.unlock_entry(&root->conn_manager,session_id) ;		
			//return -1 ;
		}
		else if(NETSTAT_ACCEPT==socket_node->conn_state){
			if(root->connect_in_callback){				
				socket_node->start_time = ne_time() ;
				root->connect_in_callback(socket_node,addr,(ne_handle)root) ;
				socket_node->conn_state = NETSTAT_ESTABLISHED ;
			}
		}	

	}
	else{
		
		socket_node = root->conn_manager.lock(&root->conn_manager,session_id) ;
		if(!socket_node){
			return -1 ;
		}
		if(-1==udt_parse_rawpacket(socket_node,pocket,len) ){
			udt_close(socket_node,1) ;
			//root->conn_manager.unlock_entry(&root->conn_manager,session_id) ;
			//return -1 ;
		}
		else {
			socket_node->last_recv = ne_time() ;
			update_socket(socket_node) ;		//update socket 
		}

	}
	if(socket_node) {
		socket_node->recv_len += len ;
		root->conn_manager.unlock(&root->conn_manager,session_id) ;
	}
	return 0 ;
}

ne_udt_node *alloc_listen_socket(ne_udtsrv *root)
{
	ne_udt_node* node ;
	ne_assert(root->conn_manager.alloc);
	node = root->conn_manager.alloc(root->cm_alloctor) ;
	if (!node){
		return NULL;
	}
	if(root->conn_manager.init ) {
		root->conn_manager.init(node,(ne_handle) root) ;
	}
	else 
		ne_udtnode_init(node);

	node->conn_state = NETSTAT_LISTEN;
	node->is_accept = 1;
	node->srv_root = root;
	node->listen_fd = root->listen_fd ;		//use same socket fd with root
	return node ;
}


//释放accept端一个已经关闭的连接
void release_dead_node(ne_udt_node *socket_node,NEINT32 needcallback)
{
	ne_udtsrv *root = socket_node->srv_root ;
	struct cm_manager *cm = &root->conn_manager ;
	
	ne_assert(root && cm) ;
	if(!root || 0==socket_node->session_id)
		return ;
#if 1
	if(root->pre_out_callback) {
		if(0!=root->pre_out_callback(socket_node,(ne_handle)root)) {
			socket_node->conn_state = NETSTAT_TIME_WAIT ;
			return ;
		}
	}
	cm->deaccept(cm, socket_node->session_id);
	
	if( root->connect_out_callback)
		root->connect_out_callback(socket_node,(ne_handle)root) ;

	_deinit_udt_socket(socket_node) ;
	cm->dealloc ((void*)socket_node,root->cm_alloctor);

#else 

	if(needcallback && root->connect_out_callback)
		root->connect_out_callback(socket_node,(ne_handle)root) ;

	_deinit_udt_socket(socket_node) ;

	ne_assert(cm->deaccept );
	cm->deaccept(cm, socket_node->session_id);

	ne_assert(cm->dealloc );
	cm->dealloc ((void*)socket_node,root->cm_alloctor);
#endif
}
