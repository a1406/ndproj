/* ����ͳһ�������û��ӿ�,��Ҫ�����ÿ�����ӵ�
 * �������˵����������ӿڲ��ڴ˷�Χ.
 * Ŀ����Ϊ����udt��tcpʹ��ͳһ�Ľӿں���
 * ʹ��ͳһ����Ϣ�ṹ.
 * 
 * ����Ķ��嶼������������,�û���ʹ����Ϣ���ͺ���ʱ,
 * ����ֱ��ʹ�����ﶨ�� ne_connector_send() ����,���﷢�͵��Ƿ��,
 * ��ʹ�� ne_msgentry.h ����� ne_connectmsg_send** ϵ�к���
 */
#ifndef _NE_NETUI_H_
#define _NE_NETUI_H_

#include "ne_net/byte_order.h"
#include "ne_net/ne_sock.h"
#include "ne_common/ne_common.h"
#include "ne_net/ne_tcp.h"
#include "ne_net/ne_udthdr.h"
#include "ne_net/ne_udt.h"

#include "ne_net/ne_netobj.h"
enum NE_NET_PROTOCOL
{
	NE_TCP_STREAM = 0 ,		//ʹ��tcpЭ������
	NE_UDT_STREAM 	//,		//ʹ��udt��streamЭ��
	//NE_UDT_DATAGRAM			//ʹ��udt��datagramЭ��
};

/*�������ӻ�滰���,��Ա����ֻ��*/
typedef struct netui_info
{
	NE_NETOBJ_BASE ;
#if 0
	NEUINT32	length ;					//self lenght sizeof struct ne_tcp_node
	NEUINT32	nodetype;					//node type must be 't'	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			//�رպ���,Ϊ�˺�handle����
	NEUINT32	level ;						//Ȩ�޵Ǽ�
	size_t		send_len ;					/* already send valid data length */
	size_t		recv_len ;					/* received valid data length */
	netime_t	start_time ;				/* net connect or start time*/
	netime_t	last_recv ;					/* last recv packet time */
	NEUINT16	session_id;		
	NEUINT16	close_reason;				/* reason be closed */
	write_packet_entry	write_entry;		/* send packet entry */	
	void *user_msg_entry ;					//�û���Ϣ���
	void *srv_root;
	void *user_data ;						//user data 
	ne_cryptkey	crypt_key ;					/* crypt key*/
	struct sockaddr_in 	remote_addr ;	
#endif //0
}*ne_netui_handle;

/*�����Ӿ���õ�����ӿ���Ϣ*/
static __INLINE__ struct netui_info *ne_get_netui(ne_handle handle)
{
	ne_assert(((struct tag_ne_handle*)handle)->type==NE_TCP || ((struct tag_ne_handle*)handle)->type==NE_UDT) ;	
	return (struct netui_info *)handle ;
};

//��udt��stream���nd��������Ϣ
//NE_NET_API 

/* ����������Ϣ 
 * @net_handle �������ӵľ��,ָ��struct ne_tcp_node(TCP����)
 *		����ndudt_socket(UDT)�ڵ�
 * @ne_msgui_buf ������Ϣ����
 * @flag ref send_flag
 * return value: 
				on error return -1 ,else return send data len ,
				send-data-len = msg_buf->data_len+sizeof(msgid + param).
				It is data length in effect
 */
NE_NET_API NEINT32 ne_connector_send(ne_handle net_handle, ne_packhdr_t *msg_hdr, NEINT32 flag) ;

/* connect remote host 
 * @port remote port
 * @host host name
 * @protocol connect protocol(TCP/UDT) reference enum NE_NET_PROTOCOL
 * on error return NULL ,else return net connect handle 
 */
//NE_NET_API ne_handle ne_connector_open(NEINT8 *host,NEINT32 port, NEINT32 protocol) ;

/*
 * ���Ѿ������ľ�����ӵ�����host�Ķ˿� port��
 * �÷�: 
			ne_handle connector = ne_object_create("tcp-connector") // or  ne_object_create("udt-connector") ;
			if(!connector) {
				//error ;
			}
			if(-1==ne_connector_openex(connector, host, port) )
				// error
			ne_connector_close(connector, 0 ) ; // or ne_object_destroy(connector) ;

 */
NE_NET_API NEINT32 ne_connector_openex(ne_handle net_handle, NEINT8 *host, NEINT32 port);
/*close connect (only used in client (connect)) */
NE_NET_API NEINT32 ne_connector_close(ne_handle net_handle, NEINT32 force) ;

NE_NET_API NEINT32 ne_connector_valid(ne_netui_handle net_handle) ;
/* reset connector
 * �ر��������Ӳ����³�ʼ������״̬,�������û�������Ϣ(��Ϣ������,������Կ)
 */
NE_NET_API NEINT32 ne_connector_reset(ne_handle net_handle) ;

/*����������*/
NEINT32 _connector_destroy(ne_handle net_handle, NEINT32 force) ;
/*����,��������ģ��, ����������Ϣ
 * ��Ҫ�����ڴ���connect��,server�˲��ڴζ���
 * ����Ż�-1,������Ҫ���ر�
 * ����0�ȴ���ʱ
 * on error return -1,check errorcode , 
 * return nothing to be done
 * else success
 * if return -1 connect need to be closed
 */
NE_NET_API NEINT32 ne_connector_update(ne_handle net_handle, netime_t timeout) ;

/* �õ����ͻ���Ŀ��г���*/
NE_NET_API size_t ne_connector_sendlen(ne_handle net_handle);

NE_NET_API void ne_connector_set_crypt(ne_handle net_handle, void *key, NEINT32 size);

NE_NET_API NEINT32 ne_connector_check_crypt(ne_handle net_handle) ;
/*�ȴ�һ��������Ϣ��Ϣ
 *�����������Ϣ�����򷵻���Ϣ�ĳ���(������Ϣ�ĳ���,���������ݳ���)
 *��ʱ,������-1.���类�رշ���0
 *
 */
NE_NET_API NEINT32 ne_connector_waitmsg(ne_handle net_handle, ne_packetbuf_t *msg_hdr, netime_t tmout);

NE_NET_API NEINT32 ne_packet_encrypt(ne_handle net_handle, ne_packetbuf_t *msgbuf);
NE_NET_API NEINT32 ne_packet_decrypt(ne_handle net_handle, ne_packetbuf_t *msgbuf);

//get net connector  or net session start time/connect in time
//static __INLINE__ netime_t ne_connector_starttime(ne_handle net_handle) 
/*����UDT�е�DATAGRAMЭ��*/
NE_NET_API NEINT32 _datagram_entry(ne_udt_node *socket, void *data, size_t len,void *param) ;

/* ��udt��stream���ݱ����Ϣģʽ,�������msg_entry����ִ��
 * return value : 0 nothing to be done
 * on error return -1 , connect need to be closed
 * success return total handle data length
 */
NE_NET_API NEINT32 parse_udt_stream(ne_udt_node *socket_addr, NENET_MSGENTRY msg_entry, void* param);

/* close reason */
enum eCloseReason{
	ECR_NORMAL = 0 
	,ECR_READOVER
	,ECR_SOCKETERROR
	,ECR_TIMEOUT
	,ECR_DATAERROR
	,ECR_MSGERROR
	,ECR_USERCLOSE
	,ECR_SENDBLOCK
	
};

#define  NE_CLOSE_REASON(h)		((struct netui_info*)h)->close_reason
static __INLINE__ void ne_set_close_reason(ne_handle handle, NEINT32 reason)
{
	NE_CLOSE_REASON(handle) = reason ;
}

NE_NET_API NEINT8 *ne_connect_close_reasondesc(ne_netui_handle net_handle) ;
#endif 
