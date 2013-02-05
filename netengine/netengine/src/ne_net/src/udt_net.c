#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_common/ne_alloc.h"

//#include "ne_net/ne_udt.h"

//回复对方的连接请求
// 返回-1 出错,1 succes
//return 0 read over ,closed by remote peer
NEINT32 _handle_syn(ne_udt_node *socket_node,struct neudt_pocket *pocket)
{
	if(socket_node->conn_state==NETSTAT_ACCEPT || socket_node->conn_state==NETSTAT_ESTABLISHED) {
		//connection create ok , perhaps current data is retransfered
		if(pocket->sequence >= socket_node->received_sequence){
			udt_set_error(socket_node,NEERR_BADPACKET);
			return -1;
		}

		//handle syn ack lost 
		set_pocket_ack(pocket,socket_node->received_sequence) ;
		write_pocket_to_socket(socket_node,pocket, net_header_size(pocket));
		return 1;
	}

	if(socket_node->conn_state==NETSTAT_LISTEN) {
		//first recv syn on server side
		//socket_node->session_id = create_session_id(socket_node->srv_root) ;
		socket_node->send_sequence = rand() ;
		socket_node->acknowledged_seq = socket_node->send_sequence ;
		socket_node->received_sequence = pocket->sequence + 1;		//接受到对方的系列号+1以便回复对方
		//handle syn ack lost 
		set_pocket_ack(pocket,socket_node->received_sequence) ;
		pocket->sequence = socket_node->send_sequence ;
		write_pocket_to_socket(socket_node,pocket, net_header_size(pocket));
		socket_node->conn_state = NETSTAT_SYNRECV ;
		return 1 ;
	}

	if(NETSTAT_SYNSEND==socket_node->conn_state){
		//connect (client side received syn ack
		if(!pocket->header.ack || socket_node->send_sequence+1!=pocket->ack_seq) {
			udt_set_error(socket_node,NEERR_BADPACKET);
			return -1;
		}
		socket_node->session_id = pocket->session_id ;
		socket_node->received_sequence = pocket->sequence +1 ;
		socket_node->acknowledged_seq = pocket->ack_seq ;		
		
		//handle syn ack lost 
		set_pocket_ack(pocket,socket_node->received_sequence) ;
		write_pocket_to_socket(socket_node,pocket, net_header_size(pocket));
		socket_node->conn_state = NETSTAT_ESTABLISHED;
		++ (socket_node->send_sequence) ;
		return 1 ;
	}
	
	if(NETSTAT_SYNRECV==socket_node->conn_state){
		if(!pocket->header.ack || socket_node->send_sequence+1!=pocket->ack_seq) {
//			ne_msgbox_dg("系列号不相等!","message error",MB_OK) ;
			udt_set_error(socket_node,NEERR_BADPACKET);
			ne_assert(0);
			return -1 ;
		}
		socket_node->conn_state = NETSTAT_ACCEPT;
		socket_node->acknowledged_seq = pocket->ack_seq ;
		++ (socket_node->send_sequence) ;		//准备开始下一个信息
		return 1 ;
	}
	udt_set_error(socket_node,NEERR_BADPACKET);

	return -1;
}

/*
 * 处理udt数据包
 * 返回0 读取数据结束(被对方关闭)
 * 出错返回-1 ,需要根据socket::myerro检测相应的错误号,并做出相关处理
 * 否则返回处理数据的长度
 */
NEINT32 _udt_packet_handler(ne_udt_node *socket_node,struct neudt_pocket *pocket,size_t len)
{
	NEINT32 ret = 1;	//默认返回1,因为数据包长度不可能是1
	udt_set_error(socket_node, NEERR_SUCCESS) ;
	if(pocket->window_len > 0)
		socket_node->window_len = pocket->window_len ;
	switch(POCKET_TYPE(pocket)) {
	case NEUDT_DATA:
		ret = _handle_income_data(socket_node, pocket, len) ;
		if(ret <= 0)
			return ret ;
		if(POCKET_ACK(pocket)) {
			_handle_ack(socket_node,pocket->ack_seq);
		}
		break ;
	case NEUDT_SYN:
		ret = _handle_syn(socket_node, pocket) ;	//return -1 on error 0 success
		break ;
	case NEUDT_ALIVE:
		break ;
	case NEUDT_ACK:
		ret = _handle_ack(socket_node,pocket->ack_seq);
		break ;
	case NEUDT_FIN:
		ret = _handle_fin(socket_node, pocket) ;		
		break ;
	case NEUDT_RESET:
		socket_node->conn_state = NETSTAT_RESET ;
		udt_reset(socket_node,0) ;
		break ;
	default :
		udt_set_error(socket_node, NEERR_BADPACKET) ;
		ret = -1;
		break ;
	}
	return ret;
}

NEINT32 notify_datain(ne_udt_node* socket_node)
{
	NEINT32 ret = 1 ;
	if(socket_node->is_accept) {
		//server socket
		ne_udtsrv *root = socket_node->srv_root ;
		ne_assert(root) ;
		if(root->data_notify_entry) {
			ret = root->data_notify_entry(socket_node, 1) ;
		}
		else if(root->income_entry){
			size_t len ;
			NEINT8 *data; 
			
			data = nelbuf_data(&socket_node->_recv_buf) ;
			len = nelbuf_datalen(&socket_node->_recv_buf) ;
			
			ret = root->income_entry(socket_node,(void*)data, len) ;
			nelbuf_reset(&socket_node->_recv_buf) ;
		}
	}
	else {

	}
	return ret;
}

//处理接受到的数据,并放入缓冲
//return value : 0 read data over , closed by remote peer,
//  on error return -1 check error code
// else return length of valid data 
NEINT32 _handle_income_data(ne_udt_node* socket_node, struct neudt_pocket *pocket, size_t len)
{
	NEINT32 data_len ;
	NEINT8 *data;
	
	ne_assert(socket_node && pocket) ;

	data_len = len - net_header_size(pocket);
	if(data_len <= 0) {
		udt_set_error(socket_node,NEERR_BADPACKET) ;
		return -1 ;	//incoming data error 
	}

	data = pocket_data(pocket) ;
	
	if(socket_node->is_datagram && socket_node->datagram_entry) {
		//handle datagram
		//这里没有保证数据必须要按照一定的顺序到达
		socket_node->received_sequence += data_len ;
		set_socket_ack( socket_node, 1) ;	

		if(-1==socket_node->datagram_entry(socket_node,pocket, data_len,socket_node->callback_param) ) {
			udt_set_error(socket_node,NEERR_USER);
			return -1;
		}
	}
	else {
		if(data_len > nelbuf_free_capacity(& socket_node->_recv_buf) ) {
			udt_set_error(socket_node,NEERR_BADPACKET) ;
			return -1 ;
		}
		//check sequence
		if(socket_node->received_sequence != pocket->sequence){
			if(socket_node->received_sequence > pocket->sequence){
				//ack lost
				set_socket_ack( socket_node, 1) ;
			}
			return 1;
		}

		//write data to buffer
		nelbuf_write(&socket_node->_recv_buf, data, data_len, EBUF_SPECIFIED)  ;
	
		socket_node->received_sequence += data_len ;

		//notified server
		set_socket_ack( socket_node, 1) ;	
		if(-1==notify_datain(socket_node) ) {
			udt_set_error(socket_node,NEERR_USER); 
			return -1;
		}
	}	
	return data_len ;
}

//把原始的udp包分解成UDT协议
//return value :  on error return -1  check error code
//0 connection need to be close
NEINT32 udt_parse_rawpacket(ne_udt_node *socket_node,void *data,size_t len )
{
	struct neudt_pocket *pocket = (struct neudt_pocket *)data ;
	if(POCKET_PROTOCOL(pocket)==PROTOCOL_UDT){		
		return _udt_packet_handler(socket_node,pocket,len);
	}
	else{
		//struct neudp_packet *udp_packet = (struct neudp_packet *)data ;
		if(len <UDP_PACKET_HEAD_SIZE ) {
			//udt_set_error()
			//nothing to be done
			return 1;
		}
		if(socket_node->datagram_entry){
			NEINT32 ret = socket_node->datagram_entry(socket_node,
				data, len-UDP_PACKET_HEAD_SIZE,socket_node->callback_param) ;
			if(-1==ret) {
				udt_set_error(socket_node,NEERR_USER) ;
				return -1;
			}
		}
		return (len-UDP_PACKET_HEAD_SIZE);
	}
}

/*request connect with server*/
NEINT32 _udt_syn(ne_udt_node *socket_node)
{
	NEINT32 i,ret;
	size_t len ;
	struct neudt_pocket syn_pocket ;
	udt_pocketbuf pocket ;
	init_udt_pocket(&syn_pocket) ;
	SET_SYN(&syn_pocket) ;
	syn_pocket.sequence = socket_node->send_sequence ;
	
	len = net_header_size(&syn_pocket);
	for (i=0; i<1; i++){
		if(-1==write_pocket_to_socket(socket_node,&syn_pocket, len) ) {
			return -1 ;
		}
		socket_node->conn_state = NETSTAT_SYNSEND ;
		ret = _wait_data(socket_node, &pocket,WAIT_CONNECT_TIME) ;
		if(ret>0) {
			if(udt_parse_rawpacket(socket_node, &pocket, ret) > 0) {
				if(NETSTAT_ESTABLISHED==socket_node->conn_state)
					return 0 ;
			}
		}
	}
	return -1;
}


void set_socket_ack(ne_udt_node *socket_node, NEINT32 flag)
{
	if(flag) {
		socket_node->need_ack = 1 ;
		if(!socket_node->is_accept){
			udt_send_ack(socket_node) ;
			socket_node->need_ack = 0 ;
		}
	}
	else {
		socket_node->need_ack = 0 ;
	}
}

NEINT32 udt_send_ack(ne_udt_node *socket_node)
{
	struct neudt_pocket ack_pocket;
	init_udt_pocket(&ack_pocket);
	set_pocket_ack(&ack_pocket, socket_node->received_sequence) ;
	SET_ACK(&ack_pocket);

	return write_pocket_to_socket(socket_node, &ack_pocket, net_header_size(&ack_pocket));
}

//解析并处理ack
NEINT32 _parse_ack(ne_udt_node *socket_node, u_32 ack_seq)
{
	NEINT32 ret = -1 ;
	u_32 unack_seq ;
	struct list_head *pos ;
	struct udt_unack *unpacket ;
	
	pos = socket_node->unack_queue.next ;

	while(pos != &socket_node->unack_queue) {
		unpacket = list_entry(pos,struct udt_unack,my_list) ;
		pos = pos->next ;

		unack_seq = unpacket->sequence + unpacket->data_len ;
		if(unack_seq==ack_seq) {
			//calculate time out val 
			if(0==unpacket->resend_times) {
				//netime_t measuerment_tm = socket_node->last_recvpacket -unpacket->send_time ;
				netime_t measuerment_tm = ne_time() -unpacket->send_time ;
				socket_node->retrans_timeout = calc_timeouval(socket_node, measuerment_tm) ; 
			}
			list_del(&unpacket->my_list) ;
			list_add(&unpacket->my_list, &socket_node->unack_free) ;
			ret = 0 ;
		}
		else if(unack_seq < ack_seq) {
			list_del(&unpacket->my_list) ;
			list_add(&unpacket->my_list, &socket_node->unack_free) ;
			ret = 0 ;
		}
	}
	return ret ;
}

NEINT32 _handle_ack(ne_udt_node *socket_node, u_32 ack_seq)
{
	NEINT32 data =(NEINT32)( ack_seq - socket_node->acknowledged_seq) ;
	NEINT32 data2 = (NEINT32)(socket_node->send_sequence - ack_seq ) ;
	size_t datalen = nelbuf_datalen(&socket_node->_send_buf) ;

	if(data < 0 ){
		udt_set_error(socket_node,NEERR_BADPACKET);
		return -1;
	}

	if((socket_node->conn_state & NETSTAT_FINSEND) && socket_node->send_sequence==ack_seq){
		socket_node->conn_state |= NETSTAT_SENDCLOSE ;
	}
	//calculate slide window 
	if(datalen >0 && (data2+data) >0) {
		if(0==_parse_ack(socket_node, ack_seq)){
			data = min(data,datalen) ;
			nelbuf_sub_data(&socket_node->_send_buf, data) ;
		}
	}
	
	socket_node->acknowledged_seq = ack_seq ;
	
	if(socket_node->is_accept) {
		if((NETSTAT_SENDCLOSE & socket_node->conn_state) &&
			(NETSTAT_RECVCLOSE &socket_node->conn_state)) {
			release_dead_node(socket_node,1);
		}
	}
	return 1;
}


/*request connect with server*/
/*这个fin只能作为connect一方的,而不能作为listen一方的*/
NEINT32 _udt_fin(ne_udt_node *socket_node)
{
	NEINT32 ret;
	size_t len ;
	struct neudt_pocket fin_pocket ;
	udt_pocketbuf pocket ;
	init_udt_pocket(&fin_pocket) ;
	SET_FIN(&fin_pocket) ;
	
	len = net_header_size(&fin_pocket);
	
	socket_node->conn_state |= NETSTAT_FINSEND ;
	
	fin_pocket.sequence = ++(socket_node->send_sequence) ;
	++(socket_node->resend_times) ;
	ret = write_pocket_to_socket(socket_node,&fin_pocket, len) ;
	if(socket_node->is_accept || ret<=0){
		return ret ;
	}
	else if(socket_node->is_accept) {
		return 0 ;
	}

	ret = -1 ;

	if((ret=_wait_data(socket_node, &pocket,RETRANSLATE_TIME)) >0 ) {
		if(udt_parse_rawpacket(socket_node, &pocket, ret) > 0) {
			if(NETSTAT_SENDCLOSE & socket_node->conn_state){
				ret = 0 ;
			}
		}
	}

	if(NETSTAT_RECVCLOSE & socket_node->conn_state) {
		return 0 ;
	}

	else {
		if((ret=_wait_data(socket_node, &pocket,RETRANSLATE_TIME)) >0) {
			udt_parse_rawpacket(socket_node, &pocket, ret) ;
			if(NETSTAT_RECVCLOSE & socket_node->conn_state)
				return 0 ;
		}
	}

	return -1;
}

//handle income fin pocket
NEINT32 _handle_fin(ne_udt_node* socket_node, struct neudt_pocket *pocket)
{
	NEINT32 ret = 1;
	if(NETSTAT_RECVCLOSE&socket_node->conn_state){
		//岑寂接收过关闭消息
		u_32 old_seq = 	socket_node->received_sequence ;
		socket_node->received_sequence = pocket->sequence ;
		udt_send_ack(socket_node) ;
		socket_node->received_sequence = old_seq ;
		return 1 ;
	}

	if((NETSTAT_ACCEPT & socket_node->conn_state)|| 
		(NETSTAT_ESTABLISHED &socket_node->conn_state)){
		
		if(socket_node->received_sequence >= pocket->sequence) {
			UDTSO_SET_RESET(socket_node) ;
		}
		
		socket_node->received_sequence = pocket->sequence ;
		socket_node->conn_state |= NETSTAT_RECVCLOSE ;
		udt_send_ack(socket_node) ;
	}
	else {
		udt_set_error(socket_node,NEERR_BADPACKET);
		return -1 ;
	}
	
	if(socket_node->is_accept && !(socket_node->conn_state & NETSTAT_SENDCLOSE)) {
		ne_udtsrv *root = socket_node->srv_root ;
		if(root) {
			if(root->data_notify_entry) {
				root->data_notify_entry(socket_node, 0) ;
			}
			else if(root->income_entry) {
				root->income_entry(socket_node, NULL, 0);
			}
		}
		
		//notify connection closed by remote peer!
		/* 当服务器程序接收到关闭以后需要显示的调用关闭程序,
		 * 因为客户端被 设计成忙等待fin-ack 
		 */
		update_socket(socket_node) ;
		_close_listend_socket(socket_node) ;
		//udt_close(socket_node,0) ;
	}
	return 0;
	
}


NEINT32 udt_connector_send(ne_udt_node* socket_addr, ne_packhdr_t *msg_buf, NEINT32 flag)
{
	NEINT32 ret ;
	socket_addr->myerrno = NEERR_SUCCESS ;
	if(ESF_POST & flag) {
		return ndudp_sendto(socket_addr, msg_buf, ne_pack_len(msg_buf)) ;			
	}
	else {		//用udt发送可靠消息
		ret = udt_send(socket_addr, msg_buf, ne_pack_len(msg_buf)) ;
		if(ret <= 0) {
			return ret ;
		}
		if(!(flag & ESF_WRITEBUF) ) {
			update_socket(socket_addr) ;			//send buf data to socket
		}
	}
	return ret ;
}
