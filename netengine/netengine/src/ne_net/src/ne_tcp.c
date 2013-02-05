#include "ne_common/ne_os.h"
#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_common/ne_alloc.h"

static NEINT32 __wait_writablity = WAIT_WRITABLITY_TIME ;

NEINT32 ne_set_wait_writablity_time(NEINT32 newtimeval)
{
	NEINT32 ret = __wait_writablity;
	__wait_writablity = newtimeval ;
	return ret ;
}

NEINT32 ne_get_wait_writablity_time()
{
	return __wait_writablity ;
}
static  NEINT32 _socket_send(struct ne_tcp_node *node,void *data , size_t len)
{
	NEINT32 ret ;
	ret = send(node->fd, data, len, 0) ;
	if(ret > 0)
		node->send_len += ret ; 
	else 
		node->myerrno = NEERR_IO ;
	return ret ;
};

//发送一个完整的消息
NEINT32 socket_send_one_msg(struct ne_tcp_node *node,void *data , size_t len)
{
	NEINT32 times = 0 ;
	NEINT32 ret ,send_ok = 0;
	
RE_SEND:
	ret = _socket_send(node, data,len) ;
	if(-1==ret ) {
		if(ne_socket_last_error()==EWOULDBLOCK){		
			if(ne_socket_wait_writablity(node->fd,__wait_writablity) > 0)
				goto RE_SEND ;
			else { 
				node->myerrno = NEERR_TIMEOUT ;
				TCPNODE_SET_RESET(node);
				return -1;
			}
		}
		else {
			node->myerrno = NEERR_IO ;
			TCPNODE_SET_RESET(node) ;
			return -1 ;
		}
	}
	else if(ret>0 && ret < len) {
		if(ne_socket_wait_writablity(node->fd,__wait_writablity) <= 0 || times>4){
			TCPNODE_SET_RESET(node) ;
			node->myerrno = NEERR_TIMEOUT;
			return -1;
		}
		++times ;
		data = (NEINT8*)data + ret ;
		len -= ret ;
		send_ok += ret ;
		goto RE_SEND ;
	}
	else {
		send_ok += ret ;
		return send_ok ;
	}
}
//connect remote host
NEINT32 ne_tcpnode_connect(NEINT8 *host, NEINT32 port, struct ne_tcp_node *node)
{
	ne_assert(node ) ;
	ne_assert(host) ;
	
	node->fd = ne_socket_tcp_connect(host, (NEINT16)port,&(node->remote_addr)) ;
	
	if(node->fd<=0){
		node->myerrno = NEERR_OPENFILE;
		return -1 ;
	}
	TCPNODE_SET_OK(node) ;
	ne_socket_nonblock(node->fd,1);
	node->start_time = ne_time() ;
//	_set_socket_addribute(node->fd) ;
	return 0 ;
}

NEINT32 ne_tcpnode_close(struct ne_tcp_node *node,NEINT32 force)
{
	//ne_assert(0);
	ne_assert(node) ;
	if(node->fd==0)
		return 0 ;
	ne_socket_close(node->fd) ;
	node->fd = 0 ;
	node->status = ETS_DEAD ;

	return 0 ;
}

/*
 *	send data throught ne_tcp_node
 */
NEINT32 ne_tcpnode_send(struct ne_tcp_node *node, ne_packhdr_t *msg_buf,NEINT32 flag)
{
	NEINT32 ret ;
	//size_t datalen =(size_t)(NENET_DATALEN(msg_buf) + NE_PACKET_HDR_SIZE) ;
	//ne_msgbuf_t
	size_t datalen = (size_t)ne_pack_len( msg_buf) ;
	
	ne_assert(node) ;
	ne_assert(msg_buf) ;
	ne_assert(datalen<nelbuf_capacity(&node->send_buffer)) ;

	packet_hton(msg_buf) ;//把网络消息的主机格式变成网络格式
	
	if((ESF_WRITEBUF | ESF_POST)&flag ) {	//写入发送缓冲
		size_t space_len = nelbuf_free_capacity(&(node->send_buffer)) ;
		if(space_len<datalen) {
			if(ESF_POST&flag) {
				node->myerrno = NEERR_LIMITED ;
				return -1 ;
			}
			if(-1==ne_tcpnode_flush_sendbuf_force(node)) {
				ne_assert(0);
				return -1 ;
			}
		}
		ret = nelbuf_write(&(node->send_buffer), (void*)msg_buf,datalen,EBUF_SPECIFIED) ;
//		ne_assert(ret == datalen);
		return ret ;
		
	}
	else if(ESF_URGENCY&flag) { //紧急发送
		if(-1 == ne_tcpnode_flush_sendbuf_force(node) ){
//			ne_assert(0) ;
			return -1 ;
		}
		ret = socket_send_one_msg(node,msg_buf,datalen) ;
//		ne_assert(ret==datalen);
		return ret ;
	}
	else {			//正常发送
		/*
		 * 这里会对发送进行优化,如果数据少则放到缓冲中,如果缓冲中数据多,则一起发送出起
		 * 需要对缓冲上限进行限制,达到一定程度强制发送出去!
		 * 发送过程:
			1. 如果有数据在缓冲,并且数据能够放入缓则只是写入缓冲,
				写入缓冲以后尝试发送(主要是为了减少WRITE次数,并且保证缓冲区数据在前
			2. 缓冲没有数据,若达到发送下限则直接发送,否则写入缓冲
			3. 缓冲有数据,数据必须按先后顺序发送
		 */
		
		if(nelbuf_datalen(&(node->send_buffer))>0) {
			//处理缓冲数据
			size_t space_len = nelbuf_free_capacity(&(node->send_buffer)) ;
			if(space_len >= datalen) {
				ret = nelbuf_write(&(node->send_buffer),(void*)msg_buf,datalen,EBUF_SPECIFIED) ;
				ne_assert(ret == datalen);
				ne_tcpnode_tryto_flush_sendbuf(node);
				return datalen ;
			}
			else {
				if(-1 == ne_tcpnode_flush_sendbuf_force(node) ){ //清空缓冲
	//				ne_assert(0);
					return -1 ;
				}
	//			ne_assert(0==nelbuf_datalen(&node->send_buffer)) ;
			}
		}
		//now send buffer is empty
		if(datalen>ALONE_SENE_SIZE) {
			//数据需要单独发送,不适用缓冲too long
			ne_assert(nelbuf_datalen(&(node->send_buffer))==0) ;
			ret = _socket_send(node,msg_buf,datalen) ;
			if(-1==ret ) {
				if(ne_socket_last_error()!=EWOULDBLOCK){
	//				ne_assert(0) ;
					return -1 ;
				}
				else {
					//数据不能发送,尝试写入缓冲
					ret = nelbuf_write(&(node->send_buffer),(void*)msg_buf,datalen,EBUF_SPECIFIED) ;
		//			ne_assert(ret==datalen) ;
					return ret ;
				}
			}
			else if(ret==datalen)
				return datalen ;
			else {
				NEINT32 wlen ;
				NEINT8 *padd = (NEINT8*) msg_buf ;
				padd += ret ;
				wlen = nelbuf_write(&(node->send_buffer),padd,datalen-ret,EBUF_SPECIFIED) ;
	//			ne_assert(wlen==(datalen-ret)) ;
				return datalen ;
			}
		}
		else {
			//数据太少写入缓冲
			ret = nelbuf_write(&(node->send_buffer),(void*)msg_buf,datalen,EBUF_SPECIFIED) ;
	//		ne_assert(ret==datalen) ;
			return ret ;
		}
	}	
}

NEINT32 ne_tcpnode_read(struct ne_tcp_node *node)
{
	NEINT32 read_len ;
	NEINT8 *addr = nelbuf_tail(&(node->recv_buffer));
	size_t space_len = nelbuf_freespace(&(node->recv_buffer)) ;
	ne_assert(space_len > 0);
	TCPNODE_READ_AGAIN(node) = 0;
	if(space_len<=0){
		node->myerrno = NEERR_LIMITED ;
		return -1;
	}

	read_len = ne_socket_tcp_read(node->fd, addr,space_len);
	if(read_len > 0) {
		node->last_recv = ne_time();
		node->recv_len += read_len ;
		nelbuf_add_data(&(node->recv_buffer),(size_t)read_len) ;
		if(read_len>=space_len)
			TCPNODE_READ_AGAIN(node) = 1;
	}
	else if(read_len==0) {
		node->myerrno = NEERR_CLOSED ;
	}
	else {
		if(ne_socket_last_error()==EWOULDBLOCK) {
			node->myerrno = NEERR_WUOLD_BLOCK ;
		}
		else
			node->myerrno = NEERR_IO ;
	}
	return read_len ;
}


/*等待一个网络消息消息
 *如果有网络消息到了则返回消息的长度
 *超时,出错返回-1,need to be close,check error code
 * RETURN 0 time out, NOT read data 
 */
NEINT32 tcpnode_wait_msg(struct ne_tcp_node *node, netime_t tmout)
{
	NEINT32 ret,read_len;
	if(tmout) {
		fd_set rfds;
		struct timeval tmvel ;
		
		FD_ZERO(&rfds) ;
		FD_SET(node->fd,&rfds) ;
		
		if(-1==(NEINT32)tmout){
			ret = select (node->fd + 1, &rfds, NULL,  NULL, NULL) ;
		}
		else {
			tmvel.tv_sec = tmout/1000 ; 
			tmvel.tv_usec = (tmout%1000) * 1000;
			ret = select (node->fd + 1,  &rfds, NULL, NULL, &tmvel) ;
		}
		if(ret==-1) {
			node->myerrno = NEERR_IO ;
			return -1 ;
		}
		else if(ret==0){
			return 0;
		}
		if(!FD_ISSET(node->fd, &rfds)){
			node->myerrno = NEERR_IO ;
			return -1;
		}
	}

	read_len = ne_tcpnode_read(node) ;

	if(read_len<=0) {
		return -1 ;
	}
	else {		
		NEINT32 used_len = 0 ;
		size_t data_len;
		//struct ndnet_msg *msg_addr ;
		ne_packhdr_t *msg_addr ;
		
		data_len = nelbuf_datalen(&(node->recv_buffer) );
		if(data_len < NE_PACKET_HDR_SIZE){
			nelbuf_move_ahead(&(node->recv_buffer));
			return 0 ;
		}

		msg_addr = (ne_packhdr_t *)nelbuf_data(&(node->recv_buffer)) ;
		used_len = ne_pack_len(msg_addr) ;
		if(used_len >= NE_PACKET_SIZE) {
			node->myerrno = NEERR_BADPACKET ;
			return -1 ;
		}
		if(used_len<=data_len){
			return used_len ;
		}
	}
	return 0;
}

/*  如果成功等待一个网络消息,
 * 那么现在可以使用get_net_msg函数从消息缓冲中提取一个消息
 */
ne_packhdr_t* tcpnode_get_msg(struct ne_tcp_node *node)
{
	NEINT32 used_len = 0 ;
	size_t data_len;
	ne_packhdr_t *msg_addr ;
	
	data_len = nelbuf_datalen(&(node->recv_buffer) );
	if(data_len < NE_PACKET_HDR_SIZE){
		return NULL ;
	}

	msg_addr = (ne_packhdr_t *)nelbuf_data(&(node->recv_buffer)) ;
	packet_ntoh(msg_addr) ;
	used_len = ne_pack_len(msg_addr);
	if(used_len >= NE_PACKET_SIZE) {
		packet_hton(msg_addr) ;
		return NULL ;
	}
	else if(used_len<=data_len){
		return msg_addr ;
	}
	return NULL ;
}

/*删除已经处理过的消息*/
void tcpnode_del_msg(struct ne_tcp_node *node, ne_packhdr_t *msgaddr)
{
	size_t used_len ;
	ne_packhdr_t tmp_hdr ;
	ne_packhdr_t *buf_msg ;

	buf_msg = nelbuf_data(&(node->recv_buffer)) ;
	NE_HDR_SET(&tmp_hdr, buf_msg) ;
	used_len = ne_pack_len(&tmp_hdr) ;

	if(msgaddr==buf_msg && used_len <= nelbuf_datalen(&(node->recv_buffer))) {
		
		if(used_len == ne_pack_len(msgaddr)) {
			nelbuf_sub_data(&(node->recv_buffer),used_len) ;
		}
	}

}

//处理接受到的消息
//return value : 0 nothing to be done  , -1 connect would be closed,else return handled message length
NEINT32 tcpnode_parse_recv_msgex(struct ne_tcp_node *node,NENET_MSGENTRY msg_entry , void *param)
{
	NEINT32 ret =0;
	NEINT32 used_len = 0 ;
	size_t data_len;
	ne_packhdr_t *msg_addr ;

	node->myerrno = NEERR_SUCCESS ;
	
RE_MESSAGE:
	data_len = nelbuf_datalen(&(node->recv_buffer) );
	if(data_len < NE_PACKET_HDR_SIZE){
		nelbuf_move_ahead(&(node->recv_buffer));
		return ret ;
	}

	msg_addr = (ne_packhdr_t *)nelbuf_data(&(node->recv_buffer)) ;
	packet_ntoh(msg_addr) ;
	used_len = ne_pack_len(msg_addr) ;

	if(used_len >= NE_PACKET_SIZE) {
		packet_hton(msg_addr) ;
		node->myerrno = NEERR_LIMITED ;
		return -1 ;
	}
	if(used_len<=data_len){
		NEINT32 user_ret = 0 ;
		if (msg_entry){
			user_ret = msg_entry(node,msg_addr,param) ; 
		}
		nelbuf_sub_data(&(node->recv_buffer),used_len) ;
		if(-1==user_ret) {
			node->myerrno = NEERR_USER ;
			return -1 ;
		}
		ret += used_len ;
		data_len = nelbuf_datalen(&(node->recv_buffer) );
		if(data_len==0){
			nelbuf_reset(&(node->recv_buffer)) ;
			return ret ;
		}
		else {
			goto RE_MESSAGE ;
		}
	}
	nelbuf_move_ahead(&(node->recv_buffer));
	return ret;
}


NEINT32 ne_tcpnode_tryto_flush_sendbuf(struct ne_tcp_node *conn_node) 
{
	if(nelbuf_datalen(&(conn_node->send_buffer)) >=SENDBUF_PUSH ||
		(ne_time() - conn_node->last_push) >= SENDBUF_TMVAL) {
		return _tcpnode_push_sendbuf(conn_node, 0);
	}
	return 0;
}

NEINT32 _tcpnode_push_sendbuf(struct ne_tcp_node *conn_node,NEINT32 force) 
{
	NEINT32 ret = 0;
	
	ne_netbuf_t *pbuf = &(conn_node->send_buffer) ;
	size_t data_len = nelbuf_datalen(pbuf) ;
	if(data_len==0)
		return 0;
	
	if(force)
		ret = (NEINT32)socket_send_one_msg(conn_node,nelbuf_data(pbuf),data_len) ;
	else 
		ret = (NEINT32)_socket_send(conn_node,nelbuf_data(pbuf),data_len) ;
	if(ret>0) {
		ne_assert(ret<= data_len) ;
		nelbuf_sub_data(pbuf,(size_t)ret) ;
	}
	return ret ;
}

void ne_tcpnode_reset(struct ne_tcp_node *conn_node) 
{
	ne_assert(conn_node) ;

	if(conn_node->fd){
		ne_socket_close(conn_node->fd) ;
		conn_node->fd = 0 ;
	}

	conn_node->level = 0 ;
	conn_node->session_id = 0;
	conn_node->close_reason = 0;		/* reason be closed */
	conn_node->status = ETS_DEAD;				/*socket state in game 0 not read 1 ready*/
	TCPNODE_READ_AGAIN(conn_node) = 0;
	conn_node->fd = 0 ;				/* socket file description */
	conn_node->send_len =0 ;			/* already send data length */
	conn_node->recv_len = 0;			/* received data length */
	conn_node->start_time = ne_time() ;		
	conn_node->last_push = ne_time() ;
	conn_node->last_recv = 0 ;

	nelbuf_init(&(conn_node->recv_buffer), sizeof(conn_node->recv_buffer.buf)) ;		/* buffer store data recv from net */
	nelbuf_init(&(conn_node->send_buffer),sizeof(conn_node->send_buffer.buf)) ;		/* buffer store data send from net */
	
	//conn_node->send_lock = NULL;
	//conn_node->user_msg_entry = 0 ;
}

void ne_tcpnode_init(struct ne_tcp_node *conn_node) 
{
	ne_assert(conn_node) ;

	conn_node->nodetype = NE_TCP ;
	conn_node->length = sizeof(struct ne_tcp_node) ;
	conn_node->close_entry = NULL;
	conn_node->write_entry =(write_packet_entry ) ne_tcpnode_send ;
	conn_node->level = 0 ;
	conn_node->session_id = 0;
	conn_node->close_reason = 0;		/* reason be closed */
	conn_node->status = ETS_DEAD;				/*socket state in game 0 not read 1 ready*/
	TCPNODE_READ_AGAIN(conn_node) = 0;
	conn_node->fd = 0 ;				/* socket file description */
	conn_node->send_len =0 ;			/* already send data length */
	conn_node->recv_len = 0;			/* received data length */
	conn_node->start_time = ne_time() ;		
	conn_node->last_push = ne_time() ;
	conn_node->last_recv = 0 ;
	conn_node->srv_root = 0 ;
	conn_node->user_data = 0 ;
	//conn_node->crypt = NULL ;			/* crypt key*/
	init_crypt_key(&conn_node->crypt_key);

	nelbuf_init(&(conn_node->recv_buffer), sizeof(conn_node->recv_buffer.buf)) ;		/* buffer store data recv from net */
	nelbuf_init(&(conn_node->send_buffer), sizeof(conn_node->send_buffer.buf)) ;		/* buffer store data send from net */
	
	conn_node->send_lock = NULL;
	conn_node->user_msg_entry = 0 ;
	
}

void ne_tcpnode_deinit(struct ne_tcp_node *conn_node) 
{
	ne_tcpnode_sendlock_deinit(conn_node)  ;
	conn_node->status = ETS_DEAD ;
}

NEINT32 ne_tcpnode_sendlock_init(struct ne_tcp_node *conn_node)
{
	conn_node->send_lock = (ne_mutex *)malloc(sizeof(ne_mutex));

	if(conn_node->send_lock) {
		ne_mutex_init(conn_node->send_lock) ;
		return 0 ;
	}
	return -1;

}
void ne_tcpnode_sendlock_deinit(struct ne_tcp_node *conn_node) 
{
	if(conn_node->send_lock){
		ne_mutex_destroy(conn_node->send_lock) ;
		free(conn_node->send_lock);
		conn_node->send_lock = 0 ;
	}
}
/* set socket attribute */
NEINT32 _set_socket_addribute(nesocket_t sock_fd)
{
#if 0
	NEINT32 sock_bufsize = 0 , new_bufsize = NE_PACKET_SIZE *2 ;
	NEINT32 output_len ;
	struct timeval timeout ;
	NEINT32 ret ;
	NEINT32 keeplive = 1 ;
	NEINT32 value ;
	
	/* 设置接收和发送的缓冲BUF*/
	output_len = sizeof(sock_bufsize) ;
	ret = getsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF,(void*)&sock_bufsize,(socklen_t*)&output_len) ;
	
	if(sock_bufsize < new_bufsize)
		setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, (void*)&new_bufsize, sizeof(new_bufsize)) ;
	
	ret = getsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF,(void*)&sock_bufsize,(socklen_t*)&output_len) ;
	if(sock_bufsize < new_bufsize)
		setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, (void*)&new_bufsize, sizeof(new_bufsize)) ;
	
//#ifdef __LINUX__	
	/*设置接收和发送的超时*/
	timeout.tv_sec = 1 ;			// one second 
	timeout.tv_usec = 0 ;		
	
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout,sizeof(timeout)) ;
	setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&timeout,sizeof(timeout)) ;

	
	/*设置保持活动选项*/
	ret = setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keeplive,sizeof(keeplive)) ;
	//if(ret )
	//	PERROR("setsockopt") ;		//only for test
	
//#endif 
	/* 设置接收下限*/
	value = NE_PACKET_HDR_SIZE ;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVLOWAT, (void*)&value,sizeof(value)) ;
	
#endif
	return 0 ;
}
