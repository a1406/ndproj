#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_common/ne_alloc.h"
//#include "ne_net/ne_udt.h"

void _init_unack_queue(ne_udt_node *socket_node);
NEINT32 post_udt_datagram(ne_udt_node* socket_node, void *data, size_t len) ;
NEINT32 post_udt_datagramex(ne_udt_node* socket_node, struct neudt_pocket *packet, size_t len) ;
NEINT32 delay_send_datagram(ne_udt_node* socket_node, struct udt_unack *delaypacket );
NEINT32 flush_send_window(ne_udt_node* socket_node);			//���� �����е�����

void ne_udtnode_init(ne_udt_node *socket_node)
{	
	socket_node->nodetype = NE_UDT ;
	socket_node->length = sizeof(ne_udt_node) ;
	socket_node->level = 0 ;
	
	socket_node->write_entry = (write_packet_entry )udt_connector_send;

	socket_node->session_id = 0;
	socket_node->close_reason = 0;		/* reason be closed */
	socket_node->listen_fd =0 ;	
	socket_node->conn_state = NETSTAT_CLOSED;
	socket_node->myerrno = 0;
	socket_node->send_len = 0 ;					/* already send data length */
	socket_node->recv_len = 0 ;					/* received data length */
	socket_node->is_accept = 0;
	socket_node->is_reset = 0 ;
	socket_node->nonblock = 0 ;
	socket_node->need_ack = 0 ;
	socket_node->iodrive_mod = 0 ;
	socket_node->is_datagram = 0 ;
	bzero(&socket_node->remote_addr,sizeof(socket_node->remote_addr)) ;
	
	socket_node->_rtt.average = RETRANSLATE_TIME ;				//��¼��������ʱ��
	socket_node->_rtt.deviation = 0;
	socket_node->retrans_timeout = RETRANSLATE_TIME*TIME_OUT_BETA ;			//��ʱ�ش�ʱ��͵ȴ��ر�ʱ��(��¼ʱ�������Ǿ���ʱ��)
//	socket_node->ack_delay = 0 ;					//�ӳ�ȷ�ϵ�ʱ��(����ʱ��)
	socket_node->last_active = 0 ;					//��һ�ν���ʱ��(����ʱ�����̫���ͳ�ʱ,���߷���alive��)
	socket_node->last_send = 0 ;					//��һ�η������ݵ�ʱ��,�����ʱ��û�з������ͳ�alive���ݱ�
	socket_node->last_recv = 0 ;
//	INIT_LIST_HEAD(&socket_node->my_list) ;			//��ntb_t�����е�λ��
	
	socket_node->resend_times = 0;
	socket_node->send_sequence = rand() ;			//��ǰ���͵ĵ�ϵ�к�
	socket_node->acknowledged_seq = socket_node->send_sequence ;	//�Ѿ�ȷ�ϵ�ϵ�к�
	socket_node->received_sequence = 0 ;			//�Է����͹�����ϵ�к�
	socket_node->window_len = 0 ;					//�Է����մ��ڵĳ���
	
	socket_node->start_time = 0 ;
	init_crypt_key(&socket_node->crypt_key) ;		//���ܽ�����Կ(�Գ���Կ)

	socket_node->datagram_entry = 0;				//����datagram�Ļص�����
	socket_node->callback_param = 0 ;
	socket_node->srv_root = 0;
	socket_node->user_data = 0 ;
	nelbuf_init(&socket_node->_recv_buf, sizeof(socket_node->_recv_buf.buf)) ;			//�������ݻ�����
	nelbuf_init(&socket_node->_send_buf, sizeof(socket_node->_send_buf.buf)) ;			//�������ݻ�����

	ne_mutex_init(&socket_node->__lock) ;
	socket_node->send_lock = NULL ;
	_init_unack_queue(socket_node) ;
}

void ne_udtnode_reset(ne_udt_node *socket_node)
{
	udt_close( socket_node,0) ;

	socket_node->level = 0 ;

	socket_node->session_id = 0;
	socket_node->listen_fd =0 ;	
	socket_node->conn_state = NETSTAT_CLOSED;
	socket_node->myerrno = 0;
	
	socket_node->send_len = 0 ;					/* already send data length */
	socket_node->recv_len = 0 ;					/* received data length */
//	socket_node->is_accept = 0;
	socket_node->is_reset = 0 ;
//	socket_node->nonblock = 0 ;
	socket_node->need_ack = 0 ;
	socket_node->iodrive_mod = 0 ;
	socket_node->is_datagram = 0 ;
	bzero(&socket_node->remote_addr,sizeof(socket_node->remote_addr)) ;
	
	socket_node->_rtt.average = RETRANSLATE_TIME ;				//��¼��������ʱ��
	socket_node->_rtt.deviation = 0;
	socket_node->retrans_timeout = RETRANSLATE_TIME*TIME_OUT_BETA ;			//��ʱ�ش�ʱ��͵ȴ��ر�ʱ��(��¼ʱ�������Ǿ���ʱ��)
	socket_node->last_active = 0 ;					//��һ�ν���ʱ��(����ʱ�����̫���ͳ�ʱ,���߷���alive��)
	socket_node->last_send = 0 ;					//��һ�η������ݵ�ʱ��,�����ʱ��û�з������ͳ�alive���ݱ�
	socket_node->last_recv = 0 ;
//	INIT_LIST_HEAD(&socket_node->my_list) ;			//��ntb_t�����е�λ��
	
	socket_node->resend_times = 0;
	socket_node->send_sequence = rand() ;			//��ǰ���͵ĵ�ϵ�к�
	socket_node->acknowledged_seq = socket_node->send_sequence ;	//�Ѿ�ȷ�ϵ�ϵ�к�
	socket_node->received_sequence = 0 ;			//�Է����͹�����ϵ�к�
	socket_node->window_len = 0 ;					//�Է����մ��ڵĳ���
	
	socket_node->start_time = 0 ;
	init_crypt_key(&socket_node->crypt_key) ;		//���ܽ�����Կ(�Գ���Կ)

//	socket_node->datagram_entry = 0;				//����datagram�Ļص�����
//	socket_node->callback_param = 0 ;
//	socket_node->srv_root = 0;
	nelbuf_init(&socket_node->_recv_buf, sizeof(socket_node->_recv_buf.buf)) ;			//�������ݻ�����
	nelbuf_init(&socket_node->_send_buf, sizeof(socket_node->_send_buf.buf)) ;			//�������ݻ�����
//	ne_mutex_init(&socket_node->__lock) ;
//	socket_node->send_lock = NULL ;
	_init_unack_queue(socket_node) ;
}

void _deinit_udt_socket(ne_udt_node *socket_node) 
{
	udt_release_sendlock(socket_node) ;
	
	ne_mutex_destroy(&socket_node->__lock) ;

	socket_node->conn_state = NETSTAT_CLOSED;
}

NEINT32 udt_init_sendlock(ne_udt_node *socket_node)
{
	socket_node->send_lock = malloc(sizeof(ne_mutex )) ;
	if(!socket_node->send_lock)
		return -1 ;
	if(-1 == ne_mutex_init(socket_node->send_lock)){
		free(socket_node->send_lock) ;
		socket_node->send_lock = NULL ;
		return -1 ;
	}
	return 0 ;
		
}

void udt_release_sendlock(ne_udt_node *socket_node)
{
	if(socket_node->send_lock){
		ne_mutex_destroy(socket_node->send_lock) ;
		free(socket_node->send_lock) ;
		socket_node->send_lock = NULL ;
	}
}

void _init_unack_queue(ne_udt_node *socket_node)
{
	NEINT32 i; 
	struct udt_unack *pack ;
	INIT_LIST_HEAD(&socket_node->unack_queue) ;		//δ��֪ͨ�Ķ���(�Ƚ��ȳ�
	INIT_LIST_HEAD(&socket_node->unack_free) ;		//����ne_unack�Ŀ��нڵ�
	for (i=0,pack=socket_node->__unack_buf; i<UDT_MAX_PACKET; i++,pack++){
		INIT_LIST_HEAD(&pack->my_list) ;
		list_add(&pack->my_list,&socket_node->unack_free) ;
	}
}

//�õ�û��ȷ�ϵĶ���
struct udt_unack *get_unack_packet(ne_udt_node *socket_node)
{
	struct list_head *list ;
	struct udt_unack *pack ;
	list = socket_node->unack_free.next ;
	if(list== &(socket_node->unack_free)) {
		return NULL ;
	}
	list_del(list) ;
	pack = list_entry(list, struct udt_unack, my_list) ;

	_init_udt_header(&pack->_hdr) ;

	return pack;
}
#if 0
ne_udt_node* udt_cli_socket(NEINT32 af,NEINT32 type,NEINT32 protocol) 
{
	nesocket_t listen_fd  ;					/* listen and connect fd */
	ne_udt_node* udt_sock = (ne_udt_node*) malloc(sizeof(ne_udt_node)) ;
	if(!udt_sock){
		ne_showerror() ;
		return NULL;
	}
	//ne_udtnode_init(udt_sock);

	/* zero all socket slot */
	listen_fd = socket(AF_INET, SOCK_DGRAM, 0) ;
	if(-1 == listen_fd){
		return NULL;
	}
	udt_sock->listen_fd = listen_fd ;
	return udt_sock ;
}

NEINT32 udt_cli_connect(ne_udt_node *socket_node, NEINT16 port, NEINT8 *host) 
{
	if(0==socket_node->listen_fd)
		return -1;
	if(-1==get_sockaddr_in(host, port,&socket_node->dest_addr) )
		return -1 ;
	
	if(0== _udt_syn(socket_node) )
		return 0 ; //syn success connect OK!
	else {
		return -1;
	}

}


NEINT32 udt_cli_bind(ne_udt_node* socket_node ,const struct sockaddr *addr, NEINT32 namelen) 
{
	if(!socket_node->listen_fd)
		return -1 ;

	return  bind (socket_node->listen_fd, addr, sizeof(struct sockaddr)) ;
	
}

NEINT32 udt_cli_close(ne_udt_node* socket_node, NEINT32 force )
{
	if(!socket_node->listen_fd)
		return -1;

	udt_close(socket_node,force);

	return 0;
}
#endif

/*ʹ��udp����һ������host �˿�portque
 * realize 3-times handshake
 */
ne_udt_node* udt_connect(NEINT8 *host, NEINT16 port, ne_udt_node *socket_node)
{
	socket_node->listen_fd = ne_socket_udp_connect(host, port,&socket_node->remote_addr);
	if(-1==socket_node->listen_fd) {
		ne_showerror() ;
		return 0;
	}

	if(-1==connect(socket_node->listen_fd,
		(struct sockaddr*)&(socket_node->remote_addr),
		sizeof(socket_node->remote_addr)) )
		return 0;
	
	ne_socket_nonblock(socket_node->listen_fd, 1) ;

	if(0 == _udt_syn(socket_node) ){
		socket_node->start_time = ne_time() ;
		return socket_node ; //syn success connect OK!
	}
	else {
		return NULL;
	}
}

//�رշ������˵�socket
void _close_listend_socket(ne_udt_node* socket_node)
{
	ne_assert(socket_node->is_accept) ;
	if((NETSTAT_ESTABLISHED & socket_node->conn_state) && 
		!(socket_node->conn_state & NETSTAT_FINSEND) ){
		
		ne_udtsrv *root = socket_node->srv_root ;
		_udt_fin(socket_node) ;	
		socket_node->last_active = ne_time();
		
	}
}

//���ͻ����е�����,���ҵȴ��Է��ظ�,ֻ�������Щ�����Źر�����
NEINT32 flush_window_to_socket(ne_udt_node* socket_node)
{
	NEINT32  seq, flag ;
	netime_t start_tm = ne_time() ,interval;
	udt_pocketbuf  pocket ; 

RE_FLUSH:
	flag = 0 ;
	//check data need to be send 
	if(nelbuf_datalen(&socket_node->_send_buf) > 0) {
		//data in send_buf need to be send
		while (0==flush_send_window(socket_node)){}
	}	
	else if (get_socket_ack(socket_node)){
		//send ack packet
		udt_send_ack(socket_node) ;
		set_socket_ack(socket_node,0) ;
	}
	
	seq = socket_node->send_sequence - socket_node->acknowledged_seq;
	if(seq > 0) {
		seq = _wait_data(socket_node, &pocket,UPDATE_TIMEVAL);
		if(seq > 0) {
			udt_parse_rawpacket(socket_node, &pocket, seq) ;
		}
		flag = -1;
	}
	
	if(-1==flag ) {
		interval = ne_time() - start_tm ;
		if(interval < (socket_node->retrans_timeout*3))
			goto RE_FLUSH ;
	}
	return 0 ;
}

//����fin��,���ҵȴ�ȷ��
//ʵ�ָĽ�3������
//����fin���ҵȴ�ȷ��
NEINT32 udt_close(ne_udt_node* socket_node,NEINT32 force)
{
	if((NETSTAT_ESTABLISHED & socket_node->conn_state) && 
		!(socket_node->conn_state & (NETSTAT_FINSEND +NETSTAT_TRYTOFIN)) ){
			
		if(socket_node->is_accept){
			socket_node->conn_state |= NETSTAT_TRYTOFIN ;			
		}
		else {
			if(NETSTAT_ESTABLISHED == socket_node->conn_state && 
				0==force && 0==socket_node->iodrive_mod) {
				//push data
				if(-1==flush_window_to_socket(socket_node) )
					return -1;
			}
			else{
				socket_node->conn_state |= NETSTAT_TRYTOFIN ;
			}
			_udt_fin(socket_node) ;	
			ne_socket_close(socket_node->listen_fd);
		}
	}
	return 0;
		
}

void udt_conn_timeout(ne_udt_node* socket_node)
{
	NETRAC("connect timeout \n") ;
	udt_reset(socket_node,0);
}

void send_reset_packet(ne_udt_node* socket_node) 
{
	struct neudt_pocket pocket;
	init_udt_pocket(&pocket);
	SET_RESET(&pocket);
	
	/* ����Ҫ���� */
	//udt_send_lock(socket_node) ;
		write_pocket_to_socket(socket_node, &pocket, net_header_size(&pocket));
	//udt_send_unlock(socket_node) ;
}

void udt_reset(ne_udt_node* socket_node, NEINT32 issend_reset) 
{
	if((NETSTAT_RESET+NETSTAT_TRYTORESET) & socket_node->conn_state) {
		return ;
	}

	if(socket_node->is_accept){
		if(issend_reset)
			socket_node->conn_state |= NETSTAT_TRYTORESET ;
		else 
			socket_node->conn_state |= NETSTAT_RESET ;
	}
	else {
		if(issend_reset)
			send_reset_packet(socket_node) ;
		ne_socket_close(socket_node->listen_fd);
		socket_node->conn_state = NETSTAT_RESET ;
		
		//_deinit_udt_socket(socket_node) ;
	}

}

/*��ϵͳ��socket fd �ж�ȡһ��udt��
 * return -1 error, return 0 time out!
 */
NEINT32 _recvfrom_udt_packet(nesocket_t fd , udt_pocketbuf* buf, SOCKADDR_IN* from_addr,netime_t outval)
{
	NEUINT16 checksum,calc_cs;
	NEINT32 ret , readlen = 0,sock_len;
	struct neudt_pocket  *packet ;
	
	ne_assert(buf && from_addr);
	
	if(outval) {
		//wait data until timeout
		fd_set r_fdset ;		
		struct timeval timeoutval = {0,0}, *ptm;
		
		if((netime_t)-1==outval) {
			ptm = NULL ;
		}
		else{
			timeoutval.tv_sec = outval/1000 ; 
			timeoutval.tv_usec = (outval%1000) * 1000;
			ptm = &timeoutval ;
		}
		FD_ZERO(&r_fdset) ;
		FD_SET(fd, &r_fdset) ;

		ret = select (fd+1, &r_fdset, NULL,NULL, ptm) ;
		if(ret <= 0)
			return ret ;
		
		if(FD_ISSET(fd,&r_fdset)){
			//nothing
		}
		else {
			//select time out
			return -1 ;
		}
	}

	//read data
	sock_len = sizeof(*from_addr) ;
	readlen = recvfrom(fd,(NEINT8*)buf, sizeof(udt_pocketbuf), 0, (LPSOCKADDR)from_addr, &sock_len )  ;
	if(readlen <= 0 || readlen >= MAX_UDP_LEN){
		return -1 ;
	}


	packet = (struct neudt_pocket  *)buf ;
	udt_net2host(packet) ;

	checksum = POCKET_CHECKSUM(packet) ;	
	POCKET_CHECKSUM(packet) = 0 ;
	
	calc_cs = ne_checksum((NEUINT16*)buf,readlen) ;
	if(checksum!=calc_cs){
		return -1 ;
	}
	
	return readlen ;

}


//���ݷ���������㳬ʱֵ
netime_t calc_timeouval(ne_udt_node *socket_node, NEINT32 measuerment) 
{
	NEINT32 ret ;
	NEINT32 err = measuerment - (socket_node->_rtt.average>>3) ;
	socket_node->_rtt.average += err ;
	if(err<0) {
		err = -err ;
	}
	err = err - (socket_node->_rtt.deviation >> 2) ;
	socket_node->_rtt.deviation  += err ;

	ret = ((socket_node->_rtt.average>>2) + socket_node->_rtt.deviation ) >> 1 ;
//	NETRAC("calc retranslate timeout measere=%d, tmout=%d\n" AND measuerment AND ret) ;
	if(ret <16)
		ret = 16 ;
	return (netime_t)ret ;
}

//�ѷ��ͨ��socket���ͳ�ȥ, len �ǰ����ܳ���
NEINT32 write_pocket_to_socket(ne_udt_node *socket_node,struct neudt_pocket *pocket, size_t len)
{
	NEINT32 ret ;
	
	POCKET_SESSIONID(pocket) = socket_node->session_id ;

	if(PROTOCOL_UDT==pocket->header.protocol){
		pocket->window_len = send_window(socket_node) ;
	}
	
	//ne_assert(POCKET_CHECKSUM(pocket)==0);

	if(POCKET_CHECKSUM(pocket)==0){
		POCKET_CHECKSUM(pocket) = ne_checksum((NEUINT16*)pocket,len) ;
	}		
	udt_host2net(pocket) ;

	ret = ne_socket_udp_write(socket_node->listen_fd, (NEINT8*)pocket, len, &socket_node->remote_addr) ;
	ne_assert(ret==len);
	udt_net2host(pocket) ;
	if(ret==len) {
		socket_node->last_active = ne_time() ;
		socket_node->send_len += ret ;
	}
	else 
		udt_set_error(socket_node,NEERR_IO) ;
	return ret ;
}

NEINT32 _check_closed(ne_udt_node* socket_node)
{
	if(NETSTAT_RECVCLOSE & socket_node->conn_state)
		return 0 ;
	else if((NETSTAT_TIME_WAIT+NETSTAT_RESET +NETSTAT_TRYTORESET) & socket_node->conn_state )
		return -1 ;
	
	return 1 ;
}

//NEINT32 drive_conn_socket()
NEINT32 udt_recv(ne_udt_node* socket_node,void *buf, NEINT32 len )
{
	NEINT32 ret = nelbuf_datalen(&socket_node->_recv_buf) ;
	
	if(len < 1 || !buf)
		return -1 ;

	if(NETSTAT_RECVCLOSE & socket_node->conn_state && 0==ret) {
		return 0 ;
	}
	else if(NETSTAT_ESTABLISHED!=socket_node->conn_state) {
		return -1 ;
	}
	//ret = nelbuf_read(&socket_node->_recv_buf, buf, len, 0) ; 
	
	if(socket_node->is_accept)
		return nelbuf_read(&socket_node->_recv_buf, buf, len, 0) ; 
	else {
		//connect peer waiting data 
		udt_pocketbuf  pocket ;
		if (socket_node->nonblock){
			if (0==socket_node->iodrive_mod){
				if(-1==update_socket(socket_node) )
					return 0 ;		//to be closed
			}
			ret = _wait_data(socket_node, &pocket,0) ;		
			if(ret <= 0 )
				return -1 ;
			ret = udt_parse_rawpacket(socket_node, &pocket, ret) ;
			if(ret <= 0 )
				return ret ;
			ret = nelbuf_read(&socket_node->_recv_buf, buf, len, 0) ; 
			if(0==ret)
				return -1 ;
			return ret ;
		}
		else {
RE_READ:
			ret = _check_closed(socket_node) ;
			if(ret <= 0)
				return ret ;
			if (0==socket_node->iodrive_mod){
				if(-1==update_socket(socket_node) )
					return 0 ;		//to be closed
			}
			ret = _wait_data(socket_node, &pocket,UPDATE_TIMEVAL) ;		
			if(0==ret)
				goto RE_READ ;
			else if(-1==ret)
				return -1 ;
			ret = udt_parse_rawpacket(socket_node, &pocket, ret) ;
			if(ret <= 0)
				return ret ;
			ret = nelbuf_read(&socket_node->_recv_buf, buf, len, 0) ; 
			if(0==ret)
				goto RE_READ;
			return ret ;
		}		
	}
}

/* �ȴ����ͻ�����ջ��߿��пռ����need_size�����ĳߴ�
 * ����ڹ涨��ʱ����û�гɹ�,�򷵻س���
 * return value > 0  success , return -1 , return 0 connect closed by remote;
 */
NEINT32 _wait_sendbuf_free(ne_udt_node* socket_node, size_t need_size,netime_t timeout)
{
	NEINT32 ret = 0;
	ne_netbuf_t  *sendbuf = &(socket_node->_send_buf) ;
	udt_pocketbuf msg_buf ;
	
	if(need_size>nelbuf_capacity(sendbuf)){
		//ne_assert(0);
		udt_set_error(socket_node,NEERR_INVALID_INPUT) ;
		return -1 ;
	}

	if(timeout<30)
		timeout = 30 ;	//3times at least
	do {		
		
		ret = _wait_data(socket_node, &msg_buf,10) ;
		if(ret > 0){
			ret = udt_parse_rawpacket(socket_node, &msg_buf, ret) ;
			if(ret <= 0 )
				return ret ;
		}		
		
		if (nelbuf_freespace(sendbuf) >= need_size)
			return nelbuf_freespace(sendbuf) ;

		//update udt socket connect
		if(-1==update_socket(socket_node)  ) {
			//ne_assert(0);
			return -1 ;
		}
		
		if(timeout >= 10)
			timeout -= 10 ;
		else 
			timeout = 0 ;
	}while(timeout);

	udt_set_error(socket_node, NEERR_WUOLD_BLOCK) ;
	//ne_assert(0);
	return -1;
}

static __INLINE__ NEINT32 pre_send_update(ne_udt_node* socket_node,size_t needfreelen)
{
	NEINT32 ret = 1 ;
	if(!socket_node->is_accept ){
		if(!socket_node->nonblock && nelbuf_freespace(&socket_node->_send_buf)<needfreelen){
			ret =_wait_sendbuf_free(socket_node,needfreelen,socket_node->retrans_timeout * 5) ;
			
		}
		/*else if (!socket_node->iodrive_mod){
			update_socket(socket_node) ;
		}*/
	}
	return ret ;
}


/*����һ����ʽ���õ���Ϣ*/
NEINT32 udt_sendex(ne_udt_node* socket_node,struct neudt_pocket *packet, NEINT32 data_len ) 
{
	NEINT32 ret ;
	if(data_len < 1) 
		return -1 ;
	ne_assert(packet) ;
	
	udt_set_error(socket_node, NEERR_SUCCESS) ;
	if((NETSTAT_FINSEND+NETSTAT_RECVCLOSE+NETSTAT_TRYTOFIN) & socket_node->conn_state  ) {
		return -1;
	}
	else if(socket_node->conn_state < NETSTAT_ESTABLISHED) {
		return -1 ;
	}
	
	ret = pre_send_update(socket_node,data_len) ;
	if(ret <= 0)
		return -1 ;

	if(socket_node->is_datagram) {
		packet->header.protocol = PROTOCOL_UDT ;
		udt_send_lock(socket_node) ;
			ret = post_udt_datagramex(socket_node, packet,  data_len) ;
		udt_send_unlock(socket_node) ;
	}
	else{
		/* ֻ�ǰ�����д�뷢�ͻ���,Ϊ�˱��ڻ������ڹ����ͳһ���з���*/
		/*����������Ҫ����������̫��,û���㹻����.
		 *���û���㹻�Ļ���ʱ,��Ҫ�ȴ�Զ�̶�ȷ�Ϸ��ͻ���.��ͬʱҲҪ���ⲡ̬�����ۺ�֢
		 */
		udt_send_lock(socket_node) ;
			ret = nelbuf_write(&socket_node->_send_buf, packet->data, data_len, 0) ;
		udt_send_unlock(socket_node) ;
	}
#ifdef NE_DEBUG
	if(ret<1)
		ne_assert(0) ;
#endif
	return ret ;
}

NEINT32 udt_send(ne_udt_node* socket_node,void *data, NEINT32 len )
{
	NEINT32 ret ;
	if(len < 1 || !data || len >= nelbuf_capacity(&socket_node->_send_buf)) {
		udt_set_error(socket_node, NEERR_INVALID_INPUT );
		return -1 ;
	}
	
	udt_set_error(socket_node, NEERR_SUCCESS) ;
	if((NETSTAT_FINSEND+NETSTAT_RECVCLOSE+NETSTAT_TRYTOFIN) & socket_node->conn_state  ) {
		udt_set_error(socket_node, NEERR_CLOSED) ;
		return -1;
	}
	else if(socket_node->conn_state < NETSTAT_ESTABLISHED) {
		udt_set_error(socket_node, NEERR_BADSOCKET) ;
		return -1 ;
	}

	ret = pre_send_update(socket_node,len) ;
	if(ret <= 0)
		return -1 ;

	if(socket_node->is_datagram) {
		udt_send_lock(socket_node) ;
			ret = post_udt_datagram(socket_node, data,  len) ;
		udt_send_unlock(socket_node) ;
	}
	else{
		/* ֻ�ǰ�����д�뷢�ͻ���,Ϊ�˱��ڻ������ڹ����ͳһ���з���*/
		udt_send_lock(socket_node) ;
			ret = nelbuf_write(&socket_node->_send_buf, data, len, 0) ;
		udt_send_unlock(socket_node) ;
	}
	return ret ;
	
}

NEINT32 udt_set_nonblock(ne_udt_node* socket_node , NEINT32 flag) 
{
	NEINT32 ret = socket_node->nonblock ;
	socket_node->nonblock = flag ? 1: 0 ;
	return ret ;
}


/* �Կɿ��ķ�ʽ����һ������Э��*/
NEINT32 post_udt_datagramex(ne_udt_node* socket_node, struct neudt_pocket *packet, size_t len) 
{
	NEINT32 ret =0;
	ne_netbuf_t  *pbuf = &socket_node->_send_buf ;
	
	size_t header_len ;
	size_t space_len = nelbuf_freespace(pbuf);

	struct udt_unack *wait_packet;
	
	udt_set_error(socket_node, NEERR_SUCCESS) ;
	if(space_len < len ) {
		udt_set_error(socket_node, NEERR_INVALID_INPUT) ;
		return -1 ;
	}
	wait_packet = get_unack_packet(socket_node) ;
	if(!wait_packet) {
		udt_set_error(socket_node, NEERR_LIMITED) ;
		return -1 ;
	}
	
	nelbuf_write(pbuf,packet->data,len,EBUF_SPECIFIED);
	if(socket_node->is_accept) {
		/* ��Ϊ��������Ҫ�����ݷŵ����Ͷ�����ʹ��ͳһ���̼߳��з���.
		 * ʹ�� delay_send_datagram�����ӳٷ���,�����̶߳�socket�ļ���ʹ��
		 */
		wait_packet->data_len = len ;
		wait_packet->sequence = socket_node->send_sequence;
		wait_packet->timeout_val = socket_node->retrans_timeout ;
		wait_packet->resend_times = 0xffff ;
		//if(packet->header.crypt) {
		//	wait_packet->_hdr.crypt =
		//}
		
		copy_udt_hdr(&packet->header, &wait_packet->_hdr) ;	//����Ǽ�����Ϣ���¼������Ϣ
		list_add_tail(&wait_packet->my_list, &socket_node->unack_queue) ;

		socket_node->send_sequence += len ;
		return len ;
	}

	init_udt_pocket(packet) ;		//initialize packet head
	if(get_socket_ack(socket_node)) {
		set_pocket_ack(packet, socket_node->received_sequence) ;
	}
	else {
		/*throw ack_seq*/
		packet =(struct neudt_pocket *)((NEINT8*)packet + sizeof(packet->ack_seq) );
		init_udt_pocket(packet) ;		//initialize packet head
	}
	header_len = net_header_size(packet) ;
	
	packet->sequence = socket_node->send_sequence ;
	ret = write_pocket_to_socket(socket_node,packet, len+header_len) ;
	if(ret < 0)
		return -1 ;

	wait_packet->send_time = socket_node->last_send = ne_time() ;
	
	wait_packet->data_len = len ;
	wait_packet->sequence = socket_node->send_sequence;
	wait_packet->timeout_val = socket_node->retrans_timeout ;
	wait_packet->resend_times = 0 ;
	
	copy_udt_hdr(&packet->header, &wait_packet->_hdr) ;
	list_add_tail(&wait_packet->my_list, &socket_node->unack_queue) ;
		
	socket_node->send_sequence += len ;
	socket_node->window_len -= len ;
	if((NEINT32)(socket_node->window_len) <0)
		socket_node->window_len = 0 ;
	set_socket_ack(socket_node, 0) ;
	
	return len ;
}

/* �Կɿ��ķ�ʽ����һ������Э��*/
NEINT32 post_udt_datagram(ne_udt_node* socket_node, void *data, size_t len) 
{
	NEINT32 ret =0;
	ne_netbuf_t  *pbuf = &socket_node->_send_buf ;
	
	size_t header_len ;
	size_t space_len = nelbuf_freespace(pbuf);

	NEINT8 *send_addr;
	struct udt_unack *wait_packet;
	
	udt_pocketbuf send_buf ;
	
	udt_set_error(socket_node, NEERR_SUCCESS) ;
	if(space_len < len ) {
		udt_set_error(socket_node, NEERR_INVALID_INPUT) ;
		return -1 ;
	}
	wait_packet = get_unack_packet(socket_node) ;
	if(!wait_packet) {
		udt_set_error(socket_node, NEERR_LIMITED) ;		
		return -1 ;
	}
	nelbuf_write(pbuf,data,len,EBUF_SPECIFIED);
	if(socket_node->is_accept) {
		/* ��Ϊ��������Ҫ�����ݷŵ����Ͷ�����ʹ��ͳһ���̼߳��з���.
		 * ʹ�� delay_send_datagram�����ӳٷ���,�����̶߳�socket�ļ���ʹ��
		 * ��������Ǽ��ܺõ�,������Ҫ���ܵ������������֧��.��Ҫʹ��post_udt_datagram ����
		 */
		wait_packet->data_len = len ;
		wait_packet->sequence = socket_node->send_sequence;
		wait_packet->timeout_val = socket_node->retrans_timeout ;
		wait_packet->resend_times = 0xffff ;
		list_add_tail(&wait_packet->my_list, &socket_node->unack_queue) ;

		socket_node->send_sequence += len ;
		return len ;
	}

	init_udt_pocket(&(send_buf.pocket)) ;
	if(get_socket_ack(socket_node)) {
		set_pocket_ack(&(send_buf.pocket), socket_node->received_sequence) ;
	}	
	header_len = net_header_size(&(send_buf.pocket)) ;
	send_addr = pocket_data(&(send_buf.pocket)) ;
	memcpy(send_addr, data,len) ;

	send_buf.pocket.sequence = socket_node->send_sequence ;
	ret = write_pocket_to_socket(socket_node,&(send_buf.pocket), len+header_len) ;
	if(ret < 0)
		return -1 ;

	wait_packet->send_time = socket_node->last_send = ne_time() ;
	
	wait_packet->data_len = len ;
	wait_packet->sequence = socket_node->send_sequence;
	wait_packet->timeout_val = socket_node->retrans_timeout ;
	wait_packet->resend_times = 0 ;
	
	copy_udt_hdr(&(send_buf.pocket.header), &wait_packet->_hdr) ;
	list_add_tail(&wait_packet->my_list, &socket_node->unack_queue) ;
		
	socket_node->send_sequence += len ;
	socket_node->window_len -= len ;
	if((NEINT32)(socket_node->window_len) <0)
		socket_node->window_len = 0 ;
	set_socket_ack(socket_node, 0) ;
	
	return len ;
}

//����udt �����е�����
NEINT32 flush_send_window(ne_udt_node* socket_node)
{
	NEINT32 ret =0;
	ne_netbuf_t  *pbuf = &socket_node->_send_buf ;
	
	size_t header_len, len ;
	size_t data_len = nelbuf_datalen(pbuf);

	NEINT8 *data ,*send_addr;
	struct udt_unack *wait_packet;
	
	udt_pocketbuf send_buf ;

	send_addr = send_window_start(socket_node, &len) ;
	if(!send_addr || len ==0){	
		return -1;
	}

	init_udt_pocket(&(send_buf.pocket)) ;

	if(get_socket_ack(socket_node)) {
		set_pocket_ack(&(send_buf.pocket), socket_node->received_sequence) ;
	}
	
	data = pocket_data(&(send_buf.pocket)) ;
	header_len = net_header_size(&(send_buf.pocket)) ;

	wait_packet = get_unack_packet(socket_node) ;
	if(!wait_packet) {
		return -1 ;
	}
	memcpy(data,send_addr, len) ;
	
	/*�������к�,��ǰ���͵��������ڵ�λ��*/
	send_buf.pocket.sequence = socket_node->send_sequence ;
	
	ret = write_pocket_to_socket(socket_node,&(send_buf.pocket), len+header_len) ;
	if(ret > 0) {
		wait_packet->data_len = len ;
		wait_packet->sequence = socket_node->send_sequence;
		wait_packet->send_time = socket_node->last_send = ne_time() ;
		wait_packet->timeout_val = socket_node->retrans_timeout ;		
		copy_udt_hdr(&(send_buf.pocket.header), &wait_packet->_hdr) ;
		
		list_add_tail(&wait_packet->my_list, &socket_node->unack_queue) ;
		socket_node->send_sequence += len ;
		socket_node->window_len -= len ;
		if((NEINT32)(socket_node->window_len) <0)
			socket_node->window_len = 0 ;
		set_socket_ack(socket_node, 0) ;
	}
	else {
		list_add(&wait_packet->my_list, &socket_node->unack_free) ;
		return -1 ;
	}
	return ((NEINT32)(data_len -len) > 0) ? 0:-1 ;
}

/* �ӳٷ���һ�����ݰ�
 * �ӳٷ��͵�Ŀ��ʱΪ�˷�ֹ����̶߳�ʹ��һ��public��socket
 * on error return -1 current socket need to be closed
 * else return 0
 */
NEINT32 delay_send_datagram(ne_udt_node* socket_node, struct udt_unack *delaypacket )
{
	NEINT32 ret,header_len ;
	NEINT8 *addr ;
	size_t data_len = nelbuf_datalen(&socket_node->_send_buf) ;
	NEINT32 offset = (NEINT32)(delaypacket->sequence - socket_node->acknowledged_seq) ;
	
	udt_pocketbuf send_buf ;

	if(offset < 0 ) {
		ne_assert(0) ;
		//���յ���ϵ�кų�����,�������Ӧ���ǲ��ᷢ����
		udt_set_error(socket_node, NEERR_BADPACKET) ;
		return -1;
	}
	data_len -= offset ;	
	if(delaypacket->data_len > data_len) {
		udt_set_error(socket_node, NEERR_BADPACKET) ;
		return -1 ;
	}

	init_udt_pocket(&(send_buf.pocket)) ;
	
	if(delaypacket->_hdr.crypt) {
		//�õ�������Ϣ
		send_buf.pocket.header.crypt = 1;
		send_buf.pocket.header.stuff = delaypacket->_hdr.stuff ;
	}
	
	if(get_socket_ack(socket_node)) {
		set_pocket_ack(&(send_buf.pocket), socket_node->received_sequence) ;
	}

	addr = (NEINT8 *) nelbuf_data( & socket_node->_send_buf) ;
	addr += offset ;

	memcpy(pocket_data(&send_buf.pocket),addr,delaypacket->data_len) ;

	send_buf.pocket.sequence = delaypacket->sequence ;	
	header_len = net_header_size(&(send_buf.pocket)) ;
	ret = write_pocket_to_socket(socket_node,&(send_buf.pocket), 	delaypacket->data_len+header_len) ;
	if(ret <= 0) {
		return -1 ;
	}

	delaypacket->send_time = socket_node->last_send = ne_time() ;	
	delaypacket->timeout_val = socket_node->retrans_timeout ;

	delaypacket->resend_times = 0 ;
	socket_node->window_len -= delaypacket->data_len ;

	
	copy_udt_hdr(&(send_buf.pocket.header), &delaypacket->_hdr) ;
	set_socket_ack(socket_node, 0) ;
	return 0 ;
}

/* �ش�һ�����
 * return 1 current packet need to be delete
 * -1 socket need to be close 
 * 0 success
 */
NEINT32 resend_packet(ne_udt_node* socket_node, struct udt_unack *unpacket )
{

	NEINT8 *addr ;
	size_t data_len = nelbuf_datalen(&socket_node->_send_buf) ;
	NEINT32 offset = (NEINT32)(unpacket->sequence - socket_node->acknowledged_seq) ;
	
	udt_pocketbuf send_buf ;

	if(offset < 0 ) {
		return 1;
	}

	data_len -= offset ;	
	if(unpacket->data_len > data_len) {
		udt_set_error(socket_node, NEERR_BADPACKET) ;
		return -1 ;
	}

	init_udt_pocket(&(send_buf.pocket)) ;
	addr = (NEINT8 *) nelbuf_data( & socket_node->_send_buf) ;
	addr += offset ;
	
	//�õ���ͷ,��Ҫ��Ϊ�˱���checksum�ļ���
	copy_udt_hdr( &unpacket->_hdr, &(send_buf.pocket.header)) ;

	if(get_socket_ack(socket_node)) {
		set_pocket_ack(&(send_buf.pocket), socket_node->received_sequence) ;
	}

	memcpy(pocket_data(&send_buf.pocket),addr,unpacket->data_len) ;
	
	send_buf.pocket.sequence = unpacket->sequence ;		
	if(write_pocket_to_socket(socket_node,&(send_buf.pocket), 
		unpacket->data_len+net_header_size(&(send_buf.pocket))) > 0) {

		unpacket->send_time = socket_node->last_send = ne_time() ;
		unpacket->timeout_val = socket_node->retrans_timeout ;
		set_socket_ack(socket_node,0) ;
	}
	else {
		return -1 ;
	}

	return 0 ;
}

/* ����Ƿ�ʱ,�����ʱ�ش����ݰ�
 * on error return -1 the connect need to be close
 */
NEINT32 udt_retranslate(ne_udt_node* socket_node)
{
	NEINT32 ret = 0 ;
	netime_t now ;
	struct list_head *pos ;
	struct udt_unack *unpacket ;
	
	//ne_mutex_trylocklock
	if(udt_send_trytolock(socket_node) )
		return 0 ;

	pos = socket_node->unack_queue.next ;
	while(pos != &socket_node->unack_queue) {
		unpacket = list_entry(pos,struct udt_unack,my_list) ;
		pos = pos->next ;
		if(0xffff==unpacket->resend_times) {
			if(-1==delay_send_datagram( socket_node, unpacket )){
				
				//udt_reset(socket_node,0) ;
				//return -1 ;
				ne_assert(0) ;
				ret = -1 ;break ;
			}
			continue ;
		}
		now = ne_time() ;
		if((now - unpacket->send_time) > unpacket->timeout_val) {
			//if()
			if(unpacket->resend_times >= MAX_ATTEMPT_SEND) {
				//udt_conn_timeout(socket_node) ;
				//return -1 ;
				ne_assert(0) ;
				udt_set_error(socket_node,NEERR_TIMEOUT) ;
				ret = -1 ;break ;
			}
			else {
				NEINT32 resend_val = resend_packet( socket_node, unpacket );
				if(-1== resend_val) {
					//udt_reset(socket_node,0) ;
					//return -1 ;
					ne_assert(0) ;
					ret = -1 ;break ;
				}
				else if(1==resend_val) {
					list_del(&unpacket->my_list) ;
					list_add(&unpacket->my_list, &socket_node->unack_free) ;
				}
				else
					++(unpacket->resend_times) ;
			}
		}
	}

	udt_send_unlock(socket_node);
	return ret;
}

/*
 * update_socket() 
 * send data in background
 * retranslate, active ,ack etc..
 * return -1 error check error code
 * else return 1 
 */
NEINT32 update_socket(ne_udt_node* socket_node)
{
	netime_t now = ne_time() ;
	
	udt_set_error(socket_node,NEERR_SUCCESS) ;
	if(-1== udt_retranslate(socket_node) ){
		ne_assert(0) ;
		//udt_reset(socket_node,0) ;
		return -1 ;
	}
	
	//��ⳬʱ
	if((now - socket_node->last_recv) > (ACTIVE_TIME *2)) {
		//connect time out, need to be close 
		//udt_reset(socket_node) ;
		udt_set_error(socket_node,NEERR_TIMEOUT) ;
		//udt_conn_timeout(socket_node);
		ne_assert(0) ;
		return -1 ;
	}

	//check data need to be send 
	if(0==socket_node->is_datagram && nelbuf_datalen(&socket_node->_send_buf) > 0) {
		//data in send_buf need to be send
		udt_send_lock(socket_node) ;
			while (0==flush_send_window(socket_node)){}
		udt_send_unlock(socket_node) ;
	}
	else if (get_socket_ack(socket_node)){
		//send ack packet
		udt_send_ack(socket_node) ;
		set_socket_ack(socket_node,0) ;
	}
	else if((now-socket_node->last_active) > ACTIVE_TIME){
		//time out send active pocket 
		struct neudt_pocket alive_pocket;
		init_udt_pocket(&alive_pocket);
		set_pocket_ack(&alive_pocket, socket_node->received_sequence) ;
		SET_ALIVE(&alive_pocket);

		if(-1==write_pocket_to_socket(socket_node, &alive_pocket, net_header_size(&alive_pocket)) ) {
			ne_assert(0) ;
			return -1 ;
		}
	}
	

	return 1 ;
}

//�õ����ʹ�����ʼ��ַ
NEINT8 *send_window_start(ne_udt_node* socket_node, NEINT32 *sendlen)
{
	NEINT8 *addr ;
	size_t data_len = nelbuf_datalen(&socket_node->_send_buf) ;
	size_t un_ack = socket_node->send_sequence - socket_node->acknowledged_seq ;
	if(data_len <= un_ack){
		return NULL ;
	}
	addr = (NEINT8 *) nelbuf_data( & socket_node->_send_buf) ;
	*sendlen = data_len - un_ack ;
	*sendlen = min(*sendlen, NEUDT_FRAGMENT_LEN) ;
	*sendlen = min(socket_node->window_len,*sendlen) ;
	return (addr + un_ack ) ;
}

NEINT32 udt_ioctl(ne_udt_node* socket_node, NEINT32 cmd, u_32 *val)
{
	NEINT32 ret = 0 ;
	switch(cmd)
	{
	case UDT_IOCTRL_NONBLOCK :
		if(socket_node->is_accept)
			ret = -1 ;
		else {
			socket_node->nonblock = *val ? 1:0 ;
		}
		break ;
	case UDT_IOCTRL_GET_SENDBUF:
		*val = nelbuf_capacity(&socket_node->_send_buf) ;
		break ;
	case UDT_IOCTRL_GET_RECVBUF:
		*val = nelbuf_capacity(&socket_node->_recv_buf) ;
		break ;
	case UDT_IOCTRL_DRIVER_MOD:
		if(socket_node->is_accept)
			ret = -1 ;
		else {
			socket_node->iodrive_mod = *val ? 1:0 ;
		}
		break ;
	case UDT_IOCTRL_SET_STREAM_TPYE:
		socket_node->is_datagram = *val ? 1:0 ;
		break ;
		
	case UDT_IOCTRL_SET_DATAGRAM_ENTRY:
		socket_node->datagram_entry =(datagram_callback) val ;
		break ;
	case UDT_IOCTRL_SET_DATAGRAM_PARAM:
		socket_node->callback_param = (void*)val ;
		break ;
	default :
		ret =-1 ;
		break ;
	}
	return ret ;
}

#if 0
//����udt�˿ڷ���udpЭ��(���ɿ��ı���Э��)
NEINT32 ndudp_recvfrom(ne_udt_node* socket_node,void *buf, NEINT32 len ) 
{
#define NOT_IMPLETION
	ne_assert(0) ;
	return -1 ;
}
#endif 

static __INLINE__ void _init_udp_header(struct neudp_packet *packet)
{
	u_16 *p =(u_16 *) packet ;
	*p++=0;*p++=0;*p++=0;
}

NEINT32 ndudp_sendto(ne_udt_node* socket_node,void *data, NEINT32 len ) 
{
	struct neudp_packet *packet ;
	udt_pocketbuf send_buff ;

	
	udt_set_error(socket_node, NEERR_SUCCESS) ;
	if (get_socket_ack(socket_node)){
		/*��ֹ��������UDP������û����Դ������ȷ����Ϣ*/
		udt_send_ack(socket_node) ;
		set_socket_ack(socket_node,0) ;
	}

	if(len < 1 || !data || len >(NEUDT_BUFFER_SIZE-UDP_PACKET_HEAD_SIZE)){
		udt_set_error(socket_node, NEERR_INVALID_INPUT) ;
		return -1 ;
	}
	if(socket_node->conn_state != NETSTAT_ESTABLISHED) {
		udt_set_error(socket_node, NEERR_BADSOCKET) ;
		return -1 ;
	}

	packet = (struct neudp_packet *)&send_buff ;
	_init_udp_header(packet) ;
	packet->header.protocol = PROTOCOL_UDP;

	memcpy(packet->data, data, len) ;
	
	len += UDP_PACKET_HEAD_SIZE ;
	return write_pocket_to_socket(socket_node,&(send_buff.pocket), len) ;

}

NEINT32 ndudp_sendtoex(ne_udt_node* socket_node,struct neudp_packet *packet, NEINT32 len ) 
{
	if (get_socket_ack(socket_node)){
		//send ack packet
		udt_send_ack(socket_node) ;
		set_socket_ack(socket_node,0) ;
	}

	if(len < 1 ||  len >(NEUDT_BUFFER_SIZE-UDP_PACKET_HEAD_SIZE)){
		udt_set_error(socket_node, NEERR_INVALID_INPUT) ;
		return -1 ;
	}
	if(socket_node->conn_state != NETSTAT_ESTABLISHED) {
		udt_set_error(socket_node, NEERR_BADSOCKET) ;
		return -1 ;
	}

	_init_udp_header(packet) ;
	packet->header.protocol = PROTOCOL_UDP;
	
	len += UDP_PACKET_HEAD_SIZE ;
	return write_pocket_to_socket(socket_node,(struct neudt_pocket*) packet, len) ;
	
}


//Wait and read data from ne_udt_node
//return 0 no data in coming, -1 error check error code ,else return lenght of in come data
NEINT32 _wait_data(ne_udt_node *socket_node,udt_pocketbuf* buf,netime_t outval) 
{
	NEINT32 ret = _recvfrom_udt_packet(socket_node->listen_fd, buf, &socket_node->remote_addr,outval) ;
	//if(ret<=0)
	//	return ret ;
	if(ret==-1) {
		socket_node->myerrno = NEERR_IO;
		return -1 ;
	}
	else if(0==ret) {
		return ret ;
	}
	socket_node->recv_len += ret ;

	if(PROTOCOL_UDT==POCKET_PROTOCOL(&buf->pocket)){
		if(socket_node->session_id ) {
			if(socket_node->session_id!=POCKET_SESSIONID(&buf->pocket)){
				socket_node->myerrno = NEERR_BADPACKET ;
				return -1 ;
			}
		}
		else {
			if(NEUDT_SYN!=POCKET_TYPE(&buf->pocket)) {
				socket_node->myerrno = NEERR_BADPACKET ;
				return -1 ;
			}
		}
	}
	else if(PROTOCOL_UDP==POCKET_PROTOCOL(&buf->pocket)) {
		struct neudp_packet *udp_packet =(struct neudp_packet *) buf ;
		if(socket_node->session_id!=udp_packet->session_id)
			return 0 ;
	}
	else {
		return 0 ;
	}
	socket_node->last_recv = ne_time() ;
	return ret ;
}
