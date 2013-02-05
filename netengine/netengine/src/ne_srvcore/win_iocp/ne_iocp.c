#include "ne_srvcore/ne_srvlib.h"
#include "ne_iocp.h"
#include "ne_common/ne_alloc.h"

#include <mswsock.h>
#pragma comment(lib, "Mswsock.lib")

struct iocp_lock_info
{
	NEUINT16 session_id ;
	struct cm_manager *pmanager ;
};
static NEINT32 iocp_cm_lock(struct ne_client_map_iocp *pclient, struct iocp_lock_info *lockinfo);

static NEINT32 iocp_session_handle_close(ne_handle h, NEINT32 force) ;
static __INLINE__ void iocp_cm_unlock(struct iocp_lock_info *lockinfo) 
{	
	lockinfo->pmanager->unlock (lockinfo->pmanager,lockinfo->session_id) ;
}

BindIoCPCallback g_BindIOCPEntry = NULL;

NEINT32 __iocp_maxlisten_num = 2000;		//设定最多多少个scoekt在等待listen
neatomic_t __iocp_listen_num;	//当前有多少个

//把数据添加到待发送队列
NEINT32 addto_wait_send_queue(struct ne_client_map_iocp *iocp_map, ne_packhdr_t *msg_buf);
void del_send_node(struct ne_client_map_iocp *iocp_map,struct send_buffer_node *send_node);
void tryto_moveto_send_buf(struct ne_client_map_iocp *iocp_map);
void destroy_send_buffer(struct ne_client_map_iocp *iocp_map) ;

#ifdef USER_THREAD_POLL

extern HANDLE g_hIOCompletionPort ;

//使用自定义线程池
NEINT32 thread_iocp_entry(struct listen_contex *listen_info) ;
NEINT32 ne_start_iocp_listen(struct listen_contex *listen_info)
{
	neprintf(_NET("START iocp listen\n"));
	__iocp_maxlisten_num = ne_cm_allocator_capacity(ne_listensrv_get_cmallocator((ne_handle)listen_info)) + 1;
	return thread_iocp_entry(listen_info);
}
#else 
NEINT32 ne_start_iocp_listen(struct listen_contex *listen_info)
{

	nesocket_t listen_fd = get_listen_fd(listen_info) ;
	NEINT32 num ;
	ne_handle context = ne_thsrv_gethandle(listen_info->listen_id) ;

	__iocp_maxlisten_num = ne_cm_allocator_capacity(ne_listensrv_get_cmallocator((ne_handle)listen_info)) + 1;
	
	neprintf(_NET("START iocp listen\n"));
	if(!g_BindIOCPEntry){
		g_BindIOCPEntry = (BindIoCPCallback)
			GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "BindIoCompletionCallback");
	}
	
	if (!g_BindIOCPEntry((HANDLE)listen_fd, iocp_callback, 0))	{
		ne_showerror() ;
		//ne_logerror("error in BindIoCompletionCallback :%s", ne_last_error() ) ;
		return -1 ;
	}
	
	num = pre_accept(&listen_info->tcp) ;
	if(0==num)
		return -1 ;

	while (!ne_thsrv_isexit(context)){

		if(!ne_thsrv_msghandler(context) )		//处理线程消息
			break ;
		//handle all connect
		if(0==update_iocp_cm(listen_info) )
			ne_sleep(1000);

//		release_dead_cm(listen_info) ;
	}
	return 0;
}

#endif
//把节点acceptEx好之后放到完成端口句柄中
NEINT32 pre_accept(struct ne_srv_node *srv_node)
{
	NEINT32 i ,ret = 0 ;
	NEINT32 listen_nums = __iocp_maxlisten_num - ne_atomic_read(&__iocp_listen_num) ;
	
	//struct ne_srv_node *root = get_listen_srvnode()  ;

	for (i=0; i<listen_nums; i++){
		struct ne_client_map_iocp *iocp_map;
		struct ne_client_map *client =
			(struct ne_client_map *)srv_node->conn_manager.alloc (srv_node->cm_alloctor) ;
		if(!client)
			break ;
		iocp_map = list_entry(client, struct ne_client_map_iocp,__client_map) ;
		if(srv_node->conn_manager.init ) {
			srv_node->conn_manager.init (client, (ne_handle)srv_node) ;
		}
		else {
			ne_iocp_node_init(iocp_map,(ne_handle)srv_node) ;
			//ne_tcpcm_init(client,(ne_handle)srv_node) ;
		}
		if(-1==ne_init_iocp_client_map(iocp_map, srv_node->fd) )
			break ;
		client->connect_node.srv_root = (ne_handle )srv_node ;
		++ret ;
	}
	return ret ;
}

NEINT32 check_repre_accept()
{
	return __iocp_maxlisten_num - ne_atomic_read(&__iocp_listen_num) ;
}

//初始化IOCP的节点函数,外部函数使用的
NEINT32 ne_iocp_node_init(struct ne_client_map_iocp *iocp_map,ne_handle h_listen)
{
	ne_tcpcm_init(&iocp_map->__client_map,h_listen) ;
	iocp_map->__client_map.connect_node.length = sizeof(struct ne_client_map_iocp) ;
	iocp_map->__client_map.connect_node.close_entry = (ne_close_callback ) iocp_session_handle_close ;
	return 0 ;
}
NEINT32 ne_init_iocp_client_map(struct ne_client_map_iocp *iocp_map,NEINT32 listen_fd)
{
	nesocket_t fd = INVALID_SOCKET;
	DWORD dwBytesRecvd;
	ne_netbuf_t  *buf_addr;
	
	ne_atomic_set(&TCPNODE_STATUS(iocp_map),ETS_DEAD) ;

	iocp_map->__client_map.connect_node.length = sizeof(struct ne_client_map_iocp) ;
	
	iocp_map->total_send = 0;		//write buf total
	iocp_map->send_len = 0;				// had been send length
		
	buf_addr = iocp_recv_buf(iocp_map) ;
	iocp_map->wsa_readbuf.buf =nelbuf_addr(buf_addr) ;
	iocp_map->wsa_readbuf.len = 0;

	buf_addr = iocp_send_buf (iocp_map) ;
	iocp_map->wsa_writebuf.buf =nelbuf_addr(buf_addr) ; 
	iocp_map->wsa_writebuf.len = 0 ;
	
	memset(&iocp_map->ov_read, 0, sizeof(iocp_map->ov_read));
	memset(&iocp_map->ov_write, 0, sizeof(iocp_map->ov_write));

	iocp_map->ov_read.client_addr = iocp_map ;
	iocp_map->ov_write.client_addr = iocp_map ;
	
	iocp_map->__client_map.connect_node.write_entry =(write_packet_entry)ne_iocp_sendmsg;


	INIT_LIST_HEAD(&iocp_map->wait_send_list) ;
	iocp_map->__wait_buffers =0 ;
	iocp_map->schedule_close_time = 0 ;

	fd =(nesocket_t) WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET==fd){
		ne_showerror() ;
		return -1;
	}
	//iocp_map->__client_map.connect_node.fd = fd ;
	TCPNODE_FD(iocp_map) = fd ;
	
	dwBytesRecvd = nelbuf_capacity(&(iocp_map->__client_map.connect_node.recv_buffer)) - (sizeof(struct sockaddr_in)+16)*2;

	if (!AcceptEx(listen_fd,
		fd,
		iocp_map->wsa_readbuf.buf,
		dwBytesRecvd ,
		sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16,
		&dwBytesRecvd,
		&iocp_map->ov_read.overlapped))
	{
		DWORD dwLastErr = WSAGetLastError() ;
		if(ERROR_IO_PENDING!=dwLastErr )  {
			SetLastError(dwLastErr) ;
			ne_showerror() ;
			ne_socket_close(fd) ;
			return -1 ;
		}
	}

	if(INVALID_SOCKET!=fd) {
		NEINT32 val = 0 ;
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF,(NEINT8*)&val, sizeof(val)) ;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF,(NEINT8*)&val, sizeof(val)) ;
	}
	ne_atomic_set(&TCPNODE_STATUS(iocp_map),ETS_ACCEPT) ;	
	//iocp_map->iocp_status =ISF_ACCEPTING;
	ne_atomic_inc(&__iocp_listen_num) ;
	
	//set close function 
	iocp_map->__client_map.connect_node.close_entry = (ne_close_callback ) iocp_session_handle_close ;
	return 0 ;
}


void CALLBACK iocp_callback(DWORD dwErrorCode, DWORD dwByteCount,LPOVERLAPPED lpOverlapped)
{
	struct NE_OVERLAPPED_PLUS *ov = (struct NE_OVERLAPPED_PLUS *)lpOverlapped ;
	struct ne_client_map_iocp *pclient ;

	if(ov==NULL || ne_host_check_exit()) 
		return ;

	pclient = ov->client_addr ;
	ne_assert(pclient);
#if 0
	if(dwErrorCode) {
		SetLastError(dwErrorCode) ;
		ne_showerror() ;
	}
#endif
	if(dwErrorCode && ne_atomic_read(&TCPNODE_STATUS(pclient))==ETS_DEAD ) {
//		SetLastError(dwErrorCode) ;
//		ne_showerror() ;
		return ;
	}

	if(0==dwByteCount ) {
//		neprintf("before close current plyaers =%d\n", ne_atomic_read(&__iocp_listen_num) );
		if(((ne_netui_handle)pclient)->session_id==0 || ne_atomic_read(&TCPNODE_STATUS(pclient)) == ETS_ACCEPT) {
			iocp_close_client(pclient,0) ;
			return ;
		}
		//if(ne_atomic_read(&TCPNODE_STATUS(pclient)) == ETS_CONNECTED) {
		else {
			struct iocp_lock_info lockinfo ;
			if(0==iocp_cm_lock(pclient, &lockinfo) ) {
				iocp_close_client(pclient,0) ;
				iocp_cm_unlock(&lockinfo) ;
			}	
		}

		return ;
	}	
		
	if(ov== &pclient->ov_write) {
		//write data success 
		ne_netbuf_t  *send_buf = iocp_send_buf(pclient) ;
		struct iocp_lock_info lockinfo ;
		if(ne_atomic_read(&TCPNODE_STATUS(pclient)) != ETS_CONNECTED) {
			return ;
		}
		//ne_mutex_lock(&(pclient->lock));
		if(0==iocp_cm_lock(pclient, &lockinfo) ) {
			nelbuf_sub_data(send_buf,(size_t)dwByteCount) ;
			pclient->send_len += dwByteCount ;
			pclient->__client_map.connect_node.send_len += dwByteCount ;

			if(0==iocp_unnotified_length(pclient) && nelbuf_datalen(send_buf)>0)
				iocp_write(pclient) ;
			iocp_cm_unlock(&lockinfo) ;
		}
		//ne_mutex_unlock(&(pclient->lock));
	}
	else if(ov==&pclient->ov_read) {		
		//ne_mutex_lock(&(pclient->lock));
		data_income(pclient, dwByteCount) ;
		//ne_mutex_unlock(&(pclient->lock));
	}
	else {
		ne_assert(0) ;
	}

}
NEINT32 data_income(struct ne_client_map_iocp *pclient, DWORD dwRecvBytes )
{
	NEINT32 read_len ;
	struct iocp_lock_info lockinfo ;
	//if(ISF_ACCEPTING==pclient->iocp_status) {
	if(	ne_atomic_read(&TCPNODE_STATUS(pclient))==ETS_ACCEPT) {
		//receive accept
		if(-1==iocp_accept(pclient)){
			return 0;
		}
	}

	ne_assert(	ne_atomic_read(&TCPNODE_STATUS(pclient))!=ETS_ACCEPT) ;

	//ne_assert(ne_atomic_read(&TCPNODE_STATUS(pclient))==ETS_CONNECTED || 
	//	ne_atomic_read(&TCPNODE_STATUS(pclient))==ETS_TRYTO_CLOSE);
	if(0!=iocp_cm_lock(pclient,&lockinfo))
		return -1 ;

	if((NEINT32)dwRecvBytes>0){
		ne_netbuf_t  *read_buf = iocp_recv_buf(pclient);
		//ne_assert(dwRecvBytes<=pclient->wsa_readbuf.len);
		ne_assert(dwRecvBytes<=nelbuf_freespace(read_buf));
		nelbuf_add_data(read_buf,dwRecvBytes) ;
		pclient->__client_map.connect_node.recv_len += dwRecvBytes ;
		pclient->__client_map.connect_node.last_recv = ne_time() ;
	}

	if(-1==iocp_parse_msgex(pclient,srv_stream_data_entry) ){
		iocp_close_client(pclient,0);
		iocp_cm_unlock(&lockinfo) ;
		return 0;
	}
	
	//再次投递一个请求
	read_len = iocp_read(pclient) ;
	if (read_len==0){
		iocp_close_client(pclient,0);
		//return read_len;
	}
	iocp_cm_unlock(&lockinfo) ;
	return read_len ;

}

//send data in iocp internal 
//return bytes of send
// return zero no data send or overlapped
// -1 error
NEINT32 iocp_write(struct ne_client_map_iocp *iocp_map)
{
	NEINT32 ret ;
	size_t data_len ;
	ne_netbuf_t  *send_buf;
	DWORD  dwBytesWritten = 0, dwFlags = 0;

	//if (iocp_map->iocp_status == ISF_DEAD || iocp_map->iocp_status == ISF_SHUTTING_DOWN) 
	if(ne_atomic_read(&TCPNODE_STATUS(iocp_map))==ETS_DEAD || ne_atomic_read(&TCPNODE_STATUS(iocp_map))==ETS_TRYTO_CLOSE)
		return -1 ;
	
	ne_object_seterror((ne_handle)iocp_map, NEERR_SUCCESS) ;
	if(!check_send_list_empty(iocp_map)) {
		tryto_moveto_send_buf(iocp_map);
	}

	send_buf = iocp_send_buf(iocp_map) ;
	data_len = nelbuf_datalen(send_buf) ;
	
	ne_assert(data_len>0);

	iocp_map->wsa_writebuf.len = data_len ;
	iocp_map->wsa_writebuf.buf = nelbuf_data(send_buf) ;
	bzero(&iocp_map->ov_write.overlapped,sizeof(iocp_map->ov_write.overlapped));
	iocp_map->total_send += data_len ;
	
	ret = WSASend(IOCP_MAP_FD(iocp_map),
		&iocp_map->wsa_writebuf,1,&dwBytesWritten,dwFlags,(WSAOVERLAPPED*)&iocp_map->ov_write,NULL);
	
	if(0==ret) {
		ne_assert(dwBytesWritten==data_len) ;
		return data_len ;
	}
	else {
		ne_assert(dwBytesWritten==0) ;
		if(WSA_IO_PENDING==GetLastError()) {	
			return 0 ;
		}
		//IOCP_MAP_SET_CLOSE(iocp_map);
		ne_object_seterror((ne_handle)iocp_map, NEERR_BADSOCKET) ; 
		TCPNODE_SET_RESET(iocp_map);
		return -1 ;
	}

}

/*iocp_read
 * 这里使用的异步重叠接受数据,实际上这里只是进行一次异步接受的投递
 * 即便是返回了接受到的数据,依然会在callback函数中被通知 
 */
NEINT32 iocp_read(struct ne_client_map_iocp *iocp_map)
{
	NEINT32 ret ;
	size_t space_len ;
	ne_netbuf_t  *read_buf;
	DWORD dwLastError = 0, dwBytesRecvd = 0, dwFlags = 0;

	if(ne_atomic_read(&TCPNODE_STATUS(iocp_map))==ETS_DEAD ){
		return -1;
	}
	
	ne_object_seterror((ne_handle)iocp_map, NEERR_SUCCESS) ;
	read_buf = iocp_recv_buf(iocp_map) ;

	if(nelbuf_datalen(read_buf)==0) {
		nelbuf_reset(read_buf);
	}
	else {
		nelbuf_move_ahead(read_buf);
	}
	
	space_len = nelbuf_freespace(read_buf) ;
	ne_assert(space_len>0);
	ne_assert(space_len<=nelbuf_capacity(read_buf));
	if( space_len == 0){ 
		ne_assert(0);
		return 0 ;		//need to be close
	}
	
	iocp_map->wsa_readbuf.len = space_len ;
	iocp_map->wsa_readbuf.buf = nelbuf_addr(read_buf) ;
	bzero(&iocp_map->ov_read.overlapped,sizeof(iocp_map->ov_read.overlapped)) ;

	ret = WSARecv(IOCP_MAP_FD(iocp_map),&(iocp_map->wsa_readbuf),1,
		&dwBytesRecvd,&dwFlags,(WSAOVERLAPPED*)&iocp_map->ov_read,NULL);

	if(0==ret) {
		if(0==dwBytesRecvd) {
			TCPNODE_SET_RESET(iocp_map) ;
			return 0 ;
		}
		return (NEINT32)dwBytesRecvd ;
	}
	else {
		if(WSA_IO_PENDING==GetLastError()) {
			return -1 ;
		}
		ne_object_seterror((ne_handle)iocp_map, NEERR_BADSOCKET) ;
		return 0 ;
	}

}

//利用close handle是调用的函数
NEINT32 iocp_session_handle_close(ne_handle h, NEINT32 force)
{
	if(force) {
		return iocp_close_client((struct ne_client_map_iocp *)h , force) ;
	}
	else {
		struct ne_client_map_iocp *client = (struct ne_client_map_iocp *)h ;
		ne_netbuf_t * send_buf = iocp_send_buf(client) ;
		size_t data_len = nelbuf_datalen(send_buf) ;

		if(0==iocp_unnotified_length(client) &&0==data_len) {
			return iocp_close_client((struct ne_client_map_iocp *)h , 0) ;
		}
		else {
			if(data_len && TCPNODE_CHECK_OK(client)) {
				iocp_write(client) ;
			}
			//avoid pending data lost
			TCPNODE_SET_CLOSED(client) ;
			client->schedule_close_time = ne_time() ;
			return 0;
		}
	}
}

NEINT32 iocp_close_client(struct ne_client_map_iocp *iocp_map, NEINT32 force)
{
	struct ne_srv_node *srv_node =(struct ne_srv_node *) iocp_map->__client_map.connect_node.srv_root ;
	ne_assert(srv_node) ;
//	if (iocp_map->iocp_status == ISF_DEAD || iocp_map->iocp_status == ISF_SHUTTING_DOWN) {
	if(ne_atomic_read(&TCPNODE_STATUS(iocp_map))==ETS_DEAD){
		return -1;
	}
	
	CancelIo((HANDLE)IOCP_MAP_FD(iocp_map));
	if(ETS_ACCEPT==ne_atomic_read(&TCPNODE_STATUS(iocp_map))) {
		ne_atomic_dec(&__iocp_listen_num) ;
		ne_tcpnode_close(&(iocp_map->__client_map.connect_node) , force) ;
		if(ne_host_check_exit()) {
			return 0 ;
		}
		if(-1==ne_init_iocp_client_map(iocp_map, srv_node->fd) ) {
			srv_node->conn_manager.dealloc (iocp_map,srv_node->cm_alloctor);
		} 
		return 0;
		//tcp_client_close()
	}
	else {
		destroy_send_buffer(iocp_map) ;
		/*if(force) {
			ne_socket_close(iocp_map->__client_map.connect_node.fd);
			iocp_map->__client_map.connect_node.fd = 0 ;
			return ;
		}
		*/
		tcp_client_close(&iocp_map->__client_map, force);
		ne_atomic_set(&TCPNODE_STATUS(iocp_map), ETS_DEAD) ;
		//iocp_map->iocp_status = ISF_SHUTTING_DOWN ;
		ne_atomic_dec(&__iocp_listen_num) ;
		if(-1==force)
			return 0;
		if(check_repre_accept()>0) {		
			pre_accept(srv_node) ;
		}
	}

	return 0 ;
	//if(force){
	//	CancelIo((HANDLE)IOCP_MAP_FD(iocp_map));
	//}
	/*else {
		//listen_srv_netclose_callback(&iocp_map->__client_map);
		if(srv_node->connect_out_callback) 
			srv_node->connect_out_callback(&iocp_map->__client_map) ;
	}*/


	//iocp_map->status
}

NEINT32 iocp_accept(struct ne_client_map_iocp *node)
{
	NEUINT16 session_id;
	NEINT32 local_len, remote_len ;
	SOCKADDR_IN *local_addr, *remote_addr ;
	struct ne_srv_node *srv_root ;
	ne_netbuf_t  *recv_buf;
	ne_assert(ne_atomic_read(&TCPNODE_STATUS(node))==ETS_ACCEPT) ;
	if(ne_atomic_read(&TCPNODE_STATUS(node))!=ETS_ACCEPT)
		return -1 ;
	recv_buf = iocp_recv_buf(node) ;
	GetAcceptExSockaddrs(node->wsa_readbuf.buf,
		nelbuf_capacity(recv_buf) - (sizeof(struct sockaddr_in)+16)*2,
		sizeof(struct sockaddr_in)+16,
		sizeof(struct sockaddr_in)+16,
		(struct sockaddr **)&local_addr, &local_len,
		(struct sockaddr **)&remote_addr, &remote_len) ;

	memcpy(&node->__client_map.connect_node.remote_addr,remote_addr,sizeof(SOCKADDR_IN)) ;
//	node->__client_map.connect_node.remote_len = sizeof(SOCKADDR_IN) ;
#ifdef USER_THREAD_POLL
	if(NULL==CreateIoCompletionPort((HANDLE)IOCP_MAP_FD(node),g_hIOCompletionPort,(DWORD) node, 0)){
		ne_showerror() ;
		iocp_close_client(node, 0);
		return -1 ;

	}
#else 
	if(!g_BindIOCPEntry((HANDLE)IOCP_MAP_FD(node),iocp_callback,0)) {
		ne_showerror() ;
		iocp_close_client(node, 0);
		return -1 ;
	}
#endif
#if 0//defined(NE_DEBUG)
	else {
		NEINT8  *pszTemp = ne_inet_ntoa( remote_addr->sin_addr.s_addr );
		printf(_NET("Connect from %s:%d"), pszTemp, htons(remote_addr->sin_port));
		
	}
#endif

	srv_root =(struct ne_srv_node *) (node->__client_map.connect_node.srv_root) ;
	ne_assert(srv_root) ;

	//accept this client
	node->__client_map.connect_node.session_id = session_id =
				srv_root->conn_manager.accept (&srv_root->conn_manager,node);
	ne_assert(node->__client_map.connect_node.session_id) ;
	if(node->__client_map.connect_node.session_id==0) {
		iocp_close_client(node, 0);
		return -1 ;
	}

	if(srv_root->connect_in_callback){
		if(-1==srv_root->connect_in_callback(&node->__client_map,remote_addr,(ne_handle)srv_root) ){
			NE_CLOSE_REASON(node) = ECR_USERCLOSE;
			iocp_close_client(node, 0);
			srv_root->conn_manager.unlock (&srv_root->conn_manager,session_id);
			return -1 ;
		}
	}
	
	ne_atomic_set(&TCPNODE_STATUS(node),ETS_CONNECTED) ;

	node->__client_map.connect_node.start_time = ne_time();
	node->__client_map.connect_node.last_recv = ne_time(); 
	srv_root->conn_manager.unlock (&srv_root->conn_manager,session_id);
	return 0 ;
}

NEINT32 ne_iocp_sendmsg(struct ne_client_map_iocp *iocp_map,ne_packhdr_t *msg_buf, NEINT32 flag) 
{
	NEINT32 ret = 0;
	//NEINT32 ret = ne_tcpnode_send(&iocp_map->__client_map.connect_node,msg_buf,ESF_WRITEBUF) ;
	ne_netbuf_t  *send_buf = iocp_send_buf(iocp_map) ;
	size_t free_space = nelbuf_freespace(send_buf) ;
	
	size_t send_len =(size_t) ne_pack_len(msg_buf) ;

	if(send_len>nelbuf_capacity(send_buf)){
		ne_assert(0);
		return -1;
	}

	//if(IOCP_MAP_CHECK_CLOSED(iocp_map))
	if(ne_atomic_read(&TCPNODE_STATUS(iocp_map))!= ETS_CONNECTED)
		return -1;

	if(!check_send_list_empty(iocp_map) ||
		free_space <send_len) {
		ret = addto_wait_send_queue(iocp_map,msg_buf);
	}
	else {
		ret = nelbuf_write(send_buf,msg_buf, send_len,EBUF_SPECIFIED) ;
	}
		
	//iocp_map->total_write_buf += send_len ;
	if(0==iocp_unnotified_length(iocp_map))
		if(-1==iocp_write(iocp_map) )
			return -1;

	return ret ;
}

//IOCP模式下处理接收到的数据
//return value : 0 message has been done , 1 nothing to be done , -1 connect would be closed
//需要使用单独的处理方法,因为如果结束数据时,不能移动缓冲区
NEINT32 iocp_parse_msgex(struct ne_client_map_iocp *iocp_map,NENET_MSGENTRY msg_entry )
{
	NEINT32 ret = 1,used_len = 0 ;
	size_t data_len;
	ne_packhdr_t *msg_addr ;
	ne_netbuf_t  *recv_buf = iocp_recv_buf(iocp_map) ;

	ne_assert(msg_entry);
	data_len = nelbuf_datalen(recv_buf );
RE_MESSAGE:
	if(data_len < NE_PACKET_HDR_SIZE){
		return ret ;
	}

	msg_addr = (ne_packhdr_t *)nelbuf_data(recv_buf) ;
	used_len = ne_pack_len(msg_addr);
	if(used_len <= data_len){
			
		ret = msg_entry(&iocp_map->__client_map.connect_node,msg_addr,iocp_map->__client_map.connect_node.srv_root) ; 
		//recv_buf->__start += used_len ;	
		nelbuf_sub_data(recv_buf, used_len);

		if(-1==ret) 
			return -1 ;
		data_len = nelbuf_datalen(recv_buf);
		if(data_len==0){
			return ret ;
		}
		else {
			goto RE_MESSAGE ;
		}
	}
	return ret;
}

//把数据添加到待发送队列
NEINT32 addto_wait_send_queue(struct ne_client_map_iocp *iocp_map, ne_packhdr_t *msg_buf)
{
	size_t msg_len =(size_t) ne_pack_len(msg_buf) ;
	struct send_buffer_node *send_node = 
		(struct send_buffer_node *) malloc(msg_len+sizeof(struct list_head)) ;
	
	if(!send_node){
		ne_assert(0);
		return -1 ;
	}
	INIT_LIST_HEAD(&(send_node->list)) ;
	memcpy(&(send_node->msg_buf),msg_buf,msg_len) ;
	list_add_tail(&(send_node->list),_SENE_LIST(iocp_map));
	
	ne_atomic_inc(&iocp_map->__wait_buffers) ;
	return msg_len;
}

/* 从发送队列中删除一个已经发送的消息节点*/
void del_send_node(struct ne_client_map_iocp *iocp_map,struct send_buffer_node *send_node)
{
	size_t msg_len = ne_pack_len(&(send_node->msg_buf)) + sizeof(struct list_head);
	list_del(&send_node->list) ;
	//ne_atomic_dec(&iocp_map->__wait_buffers) ;
	free(send_node);
}

void destroy_send_buffer(struct ne_client_map_iocp *iocp_map)
{
	size_t msg_len;
	struct list_head *pos ;
	struct send_buffer_node *send_node ;
	if(check_send_list_empty(iocp_map) )
		return  ;

	pos = _SENE_LIST(iocp_map);
	pos = pos->next ;
	while(pos != _SENE_LIST(iocp_map)) {
		send_node = list_entry(pos, struct send_buffer_node ,list) ;
		pos = pos->next ;
		msg_len = ne_pack_len(&send_node->msg_buf) ;
		del_send_node(iocp_map, send_node);
	}
}

/* 把数据从待发送队列移动到发送缓冲
 */
void tryto_moveto_send_buf(struct ne_client_map_iocp *iocp_map)
{
	size_t msg_len;
	struct list_head *pos ;
	struct send_buffer_node *send_node ;
	ne_netbuf_t  *send_buf;
	if(check_send_list_empty(iocp_map) )
		return  ;

	send_buf = iocp_send_buf(iocp_map) ;
	pos = _SENE_LIST(iocp_map);
	pos = pos->next ;
	while(pos != _SENE_LIST(iocp_map)) {
		send_node = list_entry(pos, struct send_buffer_node ,list) ;
		pos = pos->next ;
		msg_len = ne_pack_len(&send_node->msg_buf) ;
		if(msg_len <= nelbuf_free_capacity(send_buf)) {
			if(nelbuf_write(send_buf,&send_node->msg_buf,msg_len,EBUF_SPECIFIED) )
				del_send_node(iocp_map, send_node);
		}
		else {
			break ;
		}
	}
	
}

NEINT32 iocp_cm_lock(struct ne_client_map_iocp *pclient, struct iocp_lock_info *lockinfo)
{
	NEUINT16 session_id =iocp_session_id(pclient);
	struct ne_srv_node *srv_node;
	struct cm_manager *pmanager ;
	void *addr ;
	//struct iocp_lock_info lock_info ;

	ne_assert(session_id) ;
	srv_node = (struct ne_srv_node *) pclient->__client_map.connect_node.srv_root ;
	ne_assert(srv_node) ;
	pmanager = &srv_node->conn_manager ;
	lockinfo->session_id = session_id ;
	lockinfo->pmanager = pmanager ;
	addr = pmanager->lock (pmanager,session_id) ;
	if(NULL==addr){
		//need to cancel iocp socket
		return -1 ;
	}
	else if(addr != (void*)pclient) {
		pmanager->unlock (pmanager,session_id) ;
		ne_assert(0);
		return -1 ;
	}
	
	return 0 ;
}

NEINT32 update_iocp_cm(struct listen_contex *ls_info)
{
	NEINT32 ret = 0;
	cmlist_iterator_t cm_iterator ;
	struct ne_client_map_iocp *client;
	struct ne_srv_node *srv_root = &(ls_info->tcp) ;
	struct cm_manager *pmanger = &srv_root->conn_manager ;

	for(client = pmanger->lock_first (pmanger,&cm_iterator) ; client; 	client = pmanger->lock_next (pmanger,&cm_iterator) ) {
		if(TCPNODE_STATUS(client)==ETS_DEAD) {
			tcp_release_death_node(client,0) ;		//释放死连接
		}
		else if(TCPNODE_CHECK_RESET(client)) {
			if(ne_object_lasterror((ne_handle)client)==NEERR_BADSOCKET) {
				NE_CLOSE_REASON(client) = ECR_SOCKETERROR;
			}
			iocp_close_client(client,1) ;
			++ret ;
		}
		else if(TCPNODE_CHECK_CLOSED(client)) {
			if(iocp_unnotified_length(client)==0) {		//check pending data is send ok				
				iocp_close_client(client,0) ;
			}
			else {
				netime_t dist = ne_time() - client->schedule_close_time ;
				if(dist >= IOCP_DELAY_CLOSE_TIME) {

					iocp_close_client(client,0) ;
				}
			}
			++ret ;
		}
		else if(check_operate_timeout((ne_handle)client, ls_info->operate_timeout)) {
			NE_CLOSE_REASON(client) = ECR_TIMEOUT;
			iocp_close_client(client,0) ;
		}
	}
	return ret ;
}
