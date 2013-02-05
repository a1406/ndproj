/*
 * ʵ��һ����ne_engine����������Э�鶼ͳһ��һ�����ò�ӿ�
 * ��Ȼ����һ���̶��ϻή�ͳ���Ч��Ҳ�����c��C++�Ĳ��.
 */
#define NE_IMPLEMENT_HANDLE
typedef struct netui_info *ne_handle;

#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_common/ne_alloc.h"

static NEINT32 _crypt_unit_len ;		//���ܵ�Ԫ����
static ne_netcrypt __net_encrypt, __net_decrypt ;

static NEINT32 _fetch_udt_msg(ne_udt_node *socket_addr, ne_packhdr_t *msgbuf);

//TCP��Ϣ������
//����һ���ӿ�ת������,�� NENET_MSGENTRY ת���� ne_connect_msg_entry 
static NEINT32 stream_data_entry(void*node,ne_packhdr_t *msg,void *param);

/* ���¿ͻ��˵��������� */
NEINT32 ne_connector_update(ne_netui_handle net_handle,netime_t timeout) 
{
	NEINT32 ret =0;
	ne_assert(net_handle ) ;

	if(net_handle->nodetype==NE_TCP){
		NEINT32 read_len ;
		//��tcp����ʽЭ������Ϣģʽ
		struct ne_tcp_node *socket_node = (struct ne_tcp_node *) net_handle ;
		socket_node->myerrno = NEERR_SUCCESS ;

		//������շ��ͻ���
		if(0==ne_tcpnode_trytolock(socket_node)) {
			ne_tcpnode_flush_sendbuf(socket_node) ;
			ne_tcpnode_unlock(socket_node) ;
		}

		if(timeout) {
			ret = ne_socket_wait_read(socket_node->fd,timeout) ;
			if(ret<=0)
				return ret ;
		}
RE_READ:
		read_len = ne_tcpnode_read(socket_node) ;
		if(read_len<=0) {
			if(socket_node->myerrno==NEERR_WUOLD_BLOCK) {
				return 0 ;
			}
			return read_len ;
		}
		else {
			ret += read_len ;
			if(-1==tcpnode_parse_recv_msgex(socket_node,stream_data_entry, NULL) ){
				//ne_tcpnode_close(socket_node,1) ;
				return -1 ;
			}
			if(TCPNODE_READ_AGAIN(socket_node)) {
				/*read buf is to small , after parse data , read again*/
				goto RE_READ;
			}
		}
	}
	else if(net_handle->nodetype==NE_UDT) {
		ne_udt_node *socket_addr =(ne_udt_node *) net_handle ;
		udt_pocketbuf msg_buf ;
		
		socket_addr->myerrno = NEERR_SUCCESS ;

		if(-1==update_socket(socket_addr)  )
			return -1 ;
		ret = _wait_data(socket_addr, &msg_buf,timeout) ;
		if(ret <=0)
			return ret;

		if(PROTOCOL_UDP==POCKET_PROTOCOL(&(msg_buf.pocket))){
			/*����udp��Ϣ,��ǰ������Ϣ*/
			struct neudp_packet *packet = (struct neudp_packet *)&msg_buf ;
			NEINT32 data_len = ret - sizeof(struct neudp_packet) ;
			//if(packet->session_id!=socket_addr->session_id)
			//	return 0;
			return _datagram_entry(socket_addr, packet, data_len,NULL) ;
		}

		if(socket_addr->is_datagram) {
			//ֱ��ʹ��UDP����Ϣģʽ
			//������Ҫ������-1�ر�����
			socket_addr->datagram_entry = _datagram_entry ;
			socket_addr->callback_param = NULL ;
			ret = udt_parse_rawpacket(socket_addr, &msg_buf, ret) ;
		}
		else {
			//��udt��streamģʽ�����Ϣģʽ
			//������Ҫ������-1�ر�����
			if(-1==udt_parse_rawpacket(socket_addr, &msg_buf, ret))
				return -1 ;
			ret = parse_udt_stream(socket_addr, stream_data_entry, NULL) ;
		}
		// update 
		if(-1==update_socket(socket_addr)  )
			return -1 ;
	}
	return ret;	
}

void ne_connector_set_crypt(ne_netui_handle net_handle, void *key, NEINT32 size)
{
	//struct net_handle_header *h_header =(struct net_handle_header *)net_handle ;
	
	if(net_handle->nodetype==NE_TCP){
		struct ne_tcp_node* socket_addr=(struct ne_tcp_node*)net_handle ;
		if(size>0 && size<=CRYPT_KEK_SIZE && key) {
			socket_addr->crypt_key.size = size ;
			memcpy(&socket_addr->crypt_key.key, key, size) ;
		}		

	}
	else {
		ne_udt_node* socket_addr =(ne_udt_node*)net_handle ;
		
		if(size>0 && size<=CRYPT_KEK_SIZE && key) {
			socket_addr->crypt_key.size = size ;
			memcpy(&socket_addr->crypt_key.key, key, size) ;
		}
	}
}

NEINT32 ne_connector_check_crypt(ne_netui_handle net_handle)
{	
	if(net_handle->nodetype==NE_TCP){
		struct ne_tcp_node* socket_addr=(struct ne_tcp_node*)net_handle ;
		return socket_addr->crypt_key.size ? 1:0 ;
	}
	else {
		ne_udt_node* socket_addr =(ne_udt_node*)net_handle ;
		return socket_addr->crypt_key.size ? 1 :0 ;
	}
}

//�õ����ͻ���Ŀ��г���
size_t ne_connector_sendlen(ne_netui_handle net_handle)
{	
	if(net_handle->nodetype==NE_TCP){
		struct ne_tcp_node* socket_addr=(struct ne_tcp_node*)net_handle ;
		return nelbuf_freespace(&socket_addr->send_buffer);
	}
	else {
		ne_udt_node* socket_addr =(ne_udt_node*)net_handle ;
		return  nelbuf_freespace(&socket_addr->_send_buf);
	}
}

//����tcp���
/*
static NEINT32 _tcp_connector_send(struct ne_tcp_node* socket_addr, ne_packhdr_t *msg_buf, NEINT32 flag) 
{
	NEINT32 len;
	NEINT32 ret ;
	ne_packetbuf_t *pkt;
	if(!TCPNODE_CHECK_OK(socket_addr)) {
		socket_addr->myerrno = NEERR_CLOSED ;
		return -1 ;
	}
	if ((flag & ESF_ENCRYPT_LATER) && !(flag & ESF_WRITEBUF))
		return (-1);
	
	socket_addr->myerrno = NEERR_SUCCESS ;
	ne_tcpnode_lock(socket_addr) ;	//ע������������õ�ʱIOCPģʽ������ʹ��metux lock
	ret = ne_tcpnode_send( socket_addr,	msg_buf, flag) ;

	if(flag & ESF_ENCRYPT_LATER && ne_pack_len(msg_buf) > 0) {
		pkt = (ne_packetbuf_t *)(socket_addr->send_buffer.__end - ne_pack_len(msg_buf));
		len = ne_packet_encrypt((ne_netui_handle)socket_addr, pkt) + NE_PACKET_HDR_SIZE;
		nelbuf_add_data(&socket_addr->send_buffer, len - ne_pack_len(msg_buf));		
	}

	ne_tcpnode_unlock(socket_addr) ;
	return ret ;
}
*/
/* ����������Ϣ 
 * @net_handle �������ӵľ��,ָ��struct ne_tcp_node(TCP����)
 *		����ndudt_socket(UDT)�ڵ�
 * @ne_msgui_buf ������Ϣ����
 * @flag ref send_flag
 * �ⲿ����ʹ��ͳһ�����ݽ��,������ǰ��������Ϣͷ��,
 * ��netsend�����и��ݲ�ͨ������ӿ�,�����ͷ��λ��.
 */
//NEINT32 ne_connector_send(ne_netui_handle net_handle,ne_msgui_buf *msg_buf, NEINT32 flag) 
NEINT32 ne_connector_send(ne_netui_handle net_handle, ne_packhdr_t *msg_buf, NEINT32 flag) 
{
	NEINT32 len;
	NEINT32 ret;
	struct ne_tcp_node* socket_addr;	
	ne_packetbuf_t *pkt;
	ne_assert(net_handle) ;
	ne_assert(msg_buf) ;
	ne_assert(net_handle->write_entry) ;

	if ((flag & ESF_ENCRYPT_LATER) && !(flag & ESF_WRITEBUF))
		return (-1);

	if(flag & ESF_ENCRYPT && ne_pack_len(msg_buf) > 0) {
		ne_packet_encrypt(net_handle, (ne_packetbuf_t*)msg_buf) ;
	}
	ret = net_handle->write_entry(net_handle,msg_buf, flag) ;
/*  is it right? check it */
	if(flag & ESF_ENCRYPT_LATER && ne_pack_len(msg_buf) > 0) {
		ne_assert(net_handle->nodetype==NE_TCP);
		socket_addr = (struct ne_tcp_node *)net_handle;
		pkt = (ne_packetbuf_t *)(socket_addr->send_buffer.__end - ne_pack_len(msg_buf));
		len = ne_packet_encrypt(net_handle, pkt) + NE_PACKET_HDR_SIZE;
		nelbuf_add_data(&socket_addr->send_buffer, len - ne_pack_len(msg_buf));
	}
	return ret;

}

NEINT32 ne_connector_openex(ne_handle net_handle, NEINT8 *host, NEINT32 port)
{
	if(net_handle->nodetype == NE_TCP){
		struct ne_tcp_node* socket_addr=(struct ne_tcp_node*)net_handle ;
		socket_addr->myerrno = NEERR_SUCCESS ;
		if(-1 == ne_tcpnode_connect(host, port,socket_addr ) ) {
			socket_addr->myerrno = NEERR_OPENFILE ;
			return -1;
		}	
//		ne_socket_nonblock(socket_addr->fd,0);
		ne_tcpnode_sendlock_init(socket_addr) ;
//		socket_addr->write_entry = (write_packet_entry)_tcp_connector_send ;
		return 0 ;
	}
	else {
		ne_udt_node* socket_addr = (ne_udt_node*)net_handle ;
		ne_netui_handle n_handle = (ne_netui_handle) udt_connect(host, (NEINT16)port,socket_addr) ;

		if(n_handle){
			u_32 val = 1 ;
			udt_ioctl(socket_addr,UDT_IOCTRL_DRIVER_MOD,&val);

			//val =(NE_UDT_DATAGRAM==protocol) ;
			val = 0 ;
			udt_ioctl(socket_addr,UDT_IOCTRL_SET_STREAM_TPYE,&val) ;

			udt_init_sendlock(socket_addr) ;		//client socket need send lock

	//		udt_set_nonblock(socket_addr,0);
			return 0 ;
		}
		socket_addr->myerrno = NEERR_OPENFILE ;
		return -1 ;
	}
}

NEINT32 ne_connector_valid(ne_netui_handle net_handle)
{
	ne_assert(net_handle) ;
	//ne_msgtable_destroy(net_handle) ;
	if(net_handle->nodetype==NE_TCP){
		if(TCPNODE_CHECK_OK(net_handle) && ((struct ne_tcp_node*)net_handle)->fd) 
			return 1 ;
		return  0;
	}
	else if(net_handle->nodetype==NE_UDT) {
		ne_udt_node *socket_addr = (ne_udt_node*)net_handle ;
		return socket_addr->conn_state == NETSTAT_ESTABLISHED ;
	}
	return 0;
}

NEINT32 ne_connector_close(ne_netui_handle net_handle, NEINT32 flag)
{	
	//struct net_handle_header *h_header =(struct net_handle_header *)net_handle ;
	ne_assert(net_handle) ;
	//ne_msgtable_destroy(net_handle) ;
	if(net_handle->nodetype==NE_TCP){

		ne_tcpnode_close((struct ne_tcp_node*)net_handle,flag);
		
		ne_tcpnode_sendlock_deinit((struct ne_tcp_node*)net_handle) ;
		return 0 ;
	}
	else if(net_handle->nodetype==NE_UDT) {
		ne_udt_node *socket_addr = (ne_udt_node*)net_handle ;
		
		if(socket_addr->listen_fd>0) {
			udt_close(socket_addr,flag);
			udt_release_sendlock(socket_addr) ;		//client socket need send lock
		}
		return 0;
	}
	return -1;
}

/* reset connector
 * �ر��������Ӳ����³�ʼ������״̬,�������û�������Ϣ(��Ϣ������,������Կ)
 */
NEINT32 ne_connector_reset(ne_handle net_handle) 
{
	ne_assert(net_handle) ;
	ne_connector_close( net_handle, 0 ) ;

	if(net_handle->nodetype==NE_TCP){
		ne_tcpnode_reset((struct ne_tcp_node *)net_handle) ;
	}
	else if(net_handle->nodetype==NE_UDT){
		ne_udtnode_reset((ne_udt_node*)net_handle) ;
	}

	return 0 ;

}

/* ����������
 * ͨ��ne_object_destroy���ô˺���
 */
NEINT32 _connector_destroy(ne_handle net_handle, NEINT32 force) 
{
	ne_assert(net_handle) ;
	//ne_msgtable_destroy(net_handle) ;
	if(net_handle->nodetype==NE_TCP){
		struct ne_tcp_node *socket_addr = (struct ne_tcp_node*)net_handle ;
		
		ne_tcpnode_close(socket_addr,force);
		ne_msgtable_destroy(net_handle) ;
		ne_tcpnode_deinit((struct ne_tcp_node*)net_handle) ;
		return 0 ;
	}
	else if(net_handle->nodetype==NE_UDT) {
		ne_udt_node *socket_addr = (ne_udt_node*)net_handle ;
		
		if(socket_addr->listen_fd>0) {
			udt_close(socket_addr,force);
		}
		
		ne_msgtable_destroy(net_handle) ;
		_deinit_udt_socket(socket_addr) ;		//client socket need send lock
		return 0;
	}
	return -1;
}


//�ȴ�UDT���ӵ�����
//�����������Ϣ�����򷵻���Ϣ�ĳ���(������Ϣ�ĳ���,���������ݳ���)
//��ʱ,������-1,��ⷵ��ֵ.����0û�����ݳ�ʱ
static NEINT32 _wait_udt(ne_udt_node *socket_addr, ne_packetbuf_t *out_buf, netime_t tmout)
{
	NEINT32 ret ,data_len;
	udt_pocketbuf msg_buf ;
	
	if(-1==update_socket(socket_addr)  )
		return -1 ;
	ret = _wait_data(socket_addr, &msg_buf,tmout) ;
	if(ret <=0)
		return ret ;

	if(PROTOCOL_UDP==POCKET_PROTOCOL(&(msg_buf.pocket))){
		struct neudp_packet *packet = (struct neudp_packet *)&msg_buf ;
		//ne_packhdr_t *tmp = (ne_packhdr_t*)(packet->data) ;
		data_len = ret - sizeof(struct neudp_packet) ;
		//if(packet->session_id!=socket_addr->session_id)
		//	return 0;
		
		if(data_len==ne_pack_len(((ne_packhdr_t*)(packet->data)))) {
			memcpy(out_buf,packet->data, data_len) ;
			return data_len ;
		}
		return 0 ;
	}

	if(socket_addr->is_datagram) {
		socket_addr->datagram_entry = NULL ;
		if(-1==udt_parse_rawpacket(socket_addr, &msg_buf, ret) )
			return -1 ;	
		
		data_len = ret - sizeof(struct neudt_pocket) ;
		
		if(data_len==ne_pack_len(((ne_packhdr_t*)(msg_buf._buffer)))) {
			memcpy(out_buf,msg_buf._buffer, data_len) ;
			return data_len ;
		}
		return 0 ;
	}
	else {
		if(-1==udt_parse_rawpacket(socket_addr, &msg_buf, ret))
			return -1 ;
		ret = _fetch_udt_msg(socket_addr,&out_buf->hdr) ;
	}
	return ret ;
}

/* �ȴ�������Ϣ,����ͬne_connect_waitmsg,
 * ʹ����������ĺô���ֱ�Ӱ���Ϣ�ŵ�msgbuf��,
 * ������ʹ��ne_connect_getmsg��ne_connect_delmsg
 * ����ʱ����-1 ������Ҫ�ر�
 * ���򷵻صȴ��������ݳ���(0��ʾû������)
 */
/*   note:  do not use it, it will parse only one msg in one recv, if the sender send two
 *   or more msg at one time, it will not correct.
 *   you should use ne_connector_update instead
 */
NEINT32 ne_connector_waitmsg(ne_netui_handle net_handle, ne_packetbuf_t *msgbuf, netime_t tmout)
{
	NEINT32 ret = 0;

	if(net_handle->nodetype==NE_TCP){
		ne_packhdr_t *msg_addr ;
		struct ne_tcp_node *socket_node = (struct ne_tcp_node*)net_handle ;
		socket_node->myerrno = NEERR_SUCCESS ;
		//������շ��ͻ���
		if(0==ne_tcpnode_trytolock(socket_node)) {
			ne_tcpnode_flush_sendbuf(socket_node) ;
			ne_tcpnode_unlock(socket_node) ;
		}

		ret = tcpnode_wait_msg(socket_node, tmout) ;
		if(ret <= 0) {
			return ret ;
		}
		msg_addr = tcpnode_get_msg(socket_node);
		if(!msg_addr)
			return 0;

		ne_hdr_ntoh(msg_addr) ;
		memcpy(msgbuf, msg_addr, ne_pack_len(msg_addr) );
		ne_hdr_hton(msg_addr) ;

		tcpnode_del_msg(socket_node, msg_addr) ;
	}
	else {
		NEINT32 left = tmout ;
		netime_t tmstart ;
		ne_udt_node *socket_addr = (ne_udt_node *)net_handle ;
		socket_addr->myerrno = NEERR_SUCCESS ;
		if(!socket_addr->is_datagram){
			ret = _fetch_udt_msg(socket_addr, &msgbuf->hdr) ;
			if(ret>0)
				return ret;
		}
		tmstart = ne_time() ;
REWAIT:
		ret = _wait_udt(socket_addr,msgbuf,  left);
		if(0==ret) {
			left -=(NEINT32)(ne_time() - tmstart) ;
			if(left>0)
				goto REWAIT;
		}
		else if(-1==ret && socket_addr->myerrno==NEERR_WUOLD_BLOCK) {
			ne_assert(0) ;
			return 0;
		}
	}

	if(ret>0 ) {
		ne_assert(ret == ne_pack_len(&msgbuf->hdr)) ;
		if(msgbuf->hdr.encrypt) {
			NEINT32 new_len ;
			new_len = ne_packet_decrypt(net_handle, msgbuf) ;
			if(new_len==0) {
				net_handle->myerrno = NEERR_BADPACKET ;
				return -1;
			}
			ne_pack_len(&msgbuf->hdr) = (NEUINT16)new_len ;			
		}
	}
	return ret ;

}


/*����UDT�еı���(datagram)Э��*/
NEINT32 _datagram_entry(ne_udt_node *socket, void *data, size_t len,void *param) 
{
	ne_packhdr_t *packet = data;
	if(len >0 && len == (size_t)ne_pack_len(packet)) {
		return stream_data_entry(socket,packet,param) ;
	}
	return 0;
}


//TCP��Ϣ������
//����һ���ӿ�ת������,�� NENET_MSGENTRY ת���� ne_connect_msg_entry 
NEINT32 stream_data_entry(void *node,ne_packhdr_t *msg,void *param)
{
	NEINT32 ret = 0 ;
	ne_assert(node) ;
	if(msg->encrypt) {
		NEINT32 new_len ,old_len;
		new_len = ne_packet_decrypt(node, (ne_packetbuf_t*)msg) ;
		if(new_len==0) {			
			return -1;
		}
		old_len = (NEINT32) ne_pack_len(msg) ;
		ne_pack_len(msg) = (NEUINT16)new_len ;
		
		ret = ne_translate_message((ne_netui_handle)node, msg ) ;
		ne_pack_len(msg) = old_len ;
		return ret ;
	}
	else {
		return ne_translate_message((ne_netui_handle)node, msg ) ;
	}
}

/*��udt-stream�Ļ�������ȡһ��������udt��Ϣ
 *�˺������ڵȴ���Ϣ��ģ����
 *�ڵȴ���Ϣ֮ǰ��ȡһ��,��ֹ�ϴε���©,
 *�ȴ��������ݵ����Ժ�����ȡһ��
 */
NEINT32 _fetch_udt_msg(ne_udt_node *socket_addr, ne_packhdr_t *msgbuf)
{
	NEINT32 data_len , valid_len;
	ne_packhdr_t  tmp_hdr ;
	ne_netbuf_t  *pbuf;
	ne_packhdr_t *stream_data ;


	pbuf = &(socket_addr->_recv_buf) ;
	data_len = nelbuf_datalen(pbuf) ;
	
	if(data_len < NE_PACKET_HDR_SIZE)
		return 0;
	
	stream_data = (ne_packhdr_t *)nelbuf_data(pbuf) ;
	
	NE_HDR_SET(&tmp_hdr, stream_data) ;
	ne_hdr_ntoh(&tmp_hdr) ;
	
	//check incoming data legnth
	valid_len = ne_pack_len(&tmp_hdr) ;
	
	if(valid_len >NE_PACKET_SIZE) {
		socket_addr->myerrno = NEERR_BADPACKET ;
		return -1 ;	//incoming data error 
	}
	else if(valid_len > data_len )
		return 0 ;	//not enough
	
	memcpy(msgbuf, stream_data, valid_len );
	ne_hdr_ntoh(msgbuf) ;

	nelbuf_sub_data(pbuf, valid_len) ;
	return valid_len ;
}

//parse udt stream protocol , convert to datagram
NEINT32 parse_udt_stream(ne_udt_node *socket_addr,NENET_MSGENTRY msg_entry, void* param)
{
	NEINT32 data_len ,ret = 0, valid_len,handle_len=0;
	ne_netbuf_t  *pbuf;
	ne_packhdr_t *hdr_addr ;
	ne_packhdr_t hdr_tmp ;

	ne_assert(socket_addr) ;

	pbuf = &(socket_addr->_recv_buf) ;
	data_len = nelbuf_datalen(pbuf) ;
	
	if(data_len < NE_PACKET_HDR_SIZE )
		return ret;
	
RE_PARSE:
	hdr_addr = (ne_packhdr_t *)nelbuf_data(pbuf) ;
	
	NE_HDR_SET(&hdr_tmp, hdr_addr) ;
	ne_hdr_ntoh(&hdr_tmp) ;

	valid_len = ne_pack_len(&hdr_tmp) ;
	if(valid_len > NE_PACKET_SIZE )
		return -1 ;	//incoming data error 
	else if(valid_len > data_len )
		return ret ;
	

	ne_hdr_ntoh(hdr_addr) ;
	if(-1== msg_entry((ne_netui_handle)socket_addr,hdr_addr,param)){
		return -1;
	}
	ret += valid_len ;

	//ret += valid_len + NETUI_PARAM_SIZE;

	//data_len += sizeof(struct _udt_stream_user_data);
	data_len = valid_len ;
	nelbuf_sub_data(pbuf, data_len) ;
	data_len = nelbuf_datalen(pbuf) ;
	if(data_len >= NE_PACKET_HDR_SIZE)
		goto RE_PARSE ;
	return ret ;
}


void ne_net_set_crypt(ne_netcrypt encrypt_func, ne_netcrypt decrypt_func,NEINT32 crypt_unit) 
{
	__net_encrypt = encrypt_func ;
	__net_decrypt = decrypt_func ;
	_crypt_unit_len = crypt_unit ;
}

/*�������ݰ�,���ؼ��ܺ��ʵ�ʳ���,�����޸��˷����ʵ�ʳ���*/
NEINT32 ne_packet_encrypt(ne_netui_handle net_handle, ne_packetbuf_t *msgbuf)
{
	NEINT32 datalen ;
	ne_cryptkey *pcrypt_key ;

	ne_assert(net_handle) ;
	ne_assert(msgbuf);

	datalen = (NEINT32) ne_pack_len(&msgbuf->hdr) - NE_PACKET_HDR_SIZE;	
	if(datalen<=0 || datalen> (NE_PACKET_DATA_SIZE - _crypt_unit_len))
		return 0;
	
	//get net handle
	pcrypt_key = &(net_handle->crypt_key );
	
	//crypt 
	if(__net_encrypt && is_valid_crypt(pcrypt_key)) {

		NEINT32 new_len  ;
		
		new_len = __net_encrypt(msgbuf->data, datalen, pcrypt_key->key) ;
		if(0==new_len) {
			return 0 ;
		}

		msgbuf->hdr.encrypt = 1 ;
		if(new_len> datalen) {
			msgbuf->hdr.stuff =1 ;
			msgbuf->hdr.stuff_len = (new_len -datalen) ; 
			msgbuf->hdr.length += msgbuf->hdr.stuff_len ;
		}
		return new_len ;
	}
	return 0 ;
}

/*�������ݰ�,���ؽ��ܺ��ʵ�ʳ���,���ǲ��޸ķ����ʵ�ʳ���*/
NEINT32 ne_packet_decrypt(ne_netui_handle net_handle, ne_packetbuf_t *msgbuf)
{
	NEINT32 datalen ;
	ne_cryptkey *pcrypt_key ;

	ne_assert(net_handle) ;
	ne_assert(msgbuf);

	datalen = (NEINT32) ne_pack_len(&msgbuf->hdr) - NE_PACKET_HDR_SIZE;	
	if(datalen<=0 || datalen> (NE_PACKET_DATA_SIZE - _crypt_unit_len))
		return 0;
	
	//get net handle
	pcrypt_key = &(net_handle->crypt_key );
	//decrypt 
	if(__net_decrypt && is_valid_crypt(pcrypt_key)) {

		NEINT32 new_len  ;
		
		new_len = __net_decrypt(msgbuf->data, datalen, pcrypt_key->key) ;
		if(new_len==0) {
			return 0 ;
		}
		ne_assert(new_len==datalen);

		if(msgbuf->hdr.stuff) {
			ne_assert(msgbuf->hdr.stuff_len==msgbuf->data[datalen-1]) ;
			if(ne_pack_len(&msgbuf->hdr) > msgbuf->data[datalen-1]) 
				return (ne_pack_len(&msgbuf->hdr) - msgbuf->data[datalen-1]) ;
			else 
				return 0;
		}
	}
	return ne_pack_len(&msgbuf->hdr) ;
}

NEINT8 *ne_connect_close_reasondesc(ne_netui_handle net_handle)
{
	NEINT8 * perr[] = {
		"ECR_NORMAL" 
	,"ECR_READOVER"
	,"ECR_SOCKETERROR"
	,"ECR_TIMEOUT"
	,"ECR_DATAERROR"
	,"ECR_MSGERROR"
	,"ECR_USERCLOSE"
	,"ECR_SENDBLOCK"
	} ;
	if(net_handle->close_reason<=ECR_SENDBLOCK) 
		return perr[net_handle->close_reason] ;
	else 
		return "unknow" ;
}

#undef  NE_IMPLEMENT_HANDLE
