#ifndef _NE_TCP_H_
#define _NE_TCP_H_

#include "ne_net/byte_order.h"
#include "ne_net/ne_sock.h"
#include "ne_common/ne_common.h"
#include "ne_net/ne_netcrypt.h"

#include "ne_net/ne_netpack.h"

#include "ne_net/ne_netobj.h"

#define SENDBUF_PUSH	512		/*���ͻ�������,������ݴﵽ��������ɷ���*/
#define SENDBUF_TMVAL	100		/*���λ������ʱ����,���������������*/
#define ALONE_SENE_SIZE 256		/*�������ݳ��ȴ���������ֽ�������*/

/* 
 * WAIT_WRITABLITY_TIME �ȴ�һ�����ӱ�Ϊ��д��ʱ��.
 * �������̫�����ܻ�ʹϵͳ����,Ӱ��������������������.
 * ����ʹ��ne_set_wait_writablity_time �������������õȴ�ʱ��
 */
#ifdef NE_DEBUG
#define WAIT_WRITABLITY_TIME	-1		/*���޵ȴ�*/
#else 
#define WAIT_WRITABLITY_TIME    1000	/*�ȴ�socke��д��ʱ��*/
#endif

//TCP����״̬
enum ETCP_CONNECT_STATUS{
	ETS_DEAD = 0 ,			//������(���ߵȴ����ͷ�)
	ETS_ACCEPT,				//�ȴ����ӽ���(IOCP)
	ETS_CONNECTED ,			//���ӳɹ�
	ETS_TRYTO_CLOSE	,		//�ȴ��ر�����need to send data can be receive data
	ETS_RESET				//need to be reset (force close). socket would be set reset status on error when write data
};

/*�������ӽڵ������,ʹ��tcp������udp CONNECT NODE*/
enum ENET_CONNECT_TYPE{
	NE_TCP = 't',
	NE_UDT = 'u'
};
struct ne_tcp_node;
typedef NEINT32 (*write_data_entry)(struct ne_tcp_node *node,void *data , size_t len) ; //�������ݺ���

typedef NEINT32 (*write_packet_entry)(ne_handle net_handle, ne_packhdr_t *msg_buf, NEINT32 flag) ;	//define extend send function
/* socket connect info struct */
struct ne_tcp_node{	
	NE_NETOBJ_BASE ;
#if 0
	NEUINT32	length ;					//self lenght sizeof struct ne_tcp_node
	NEUINT32	nodetype;					//node type must be 't'	unsigned	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			//�رպ���,Ϊ�˺�handle����
	NEUINT32	level ;						//Ȩ�޵Ǽ�
	size_t		send_len ;			/* already send data length */
	size_t		recv_len ;			/* received data length */
	netime_t	start_time ;		/* net connect or start time*/
	netime_t	last_recv ;			/* last recv packet time */
	NEUINT16	session_id;				
	NEUINT16	close_reason;		/* reason be closed */
	write_packet_entry	write_entry;	/* send packet entry */
	void		*user_msg_entry ;		//�û���Ϣ���
	void		*srv_root;				//root of server listen node
	void		*user_data ;			//user data 
	ne_cryptkey	crypt_key ;				/* crypt key*/
	struct sockaddr_in 	remote_addr ;
#endif //0
	NEUINT16	read_again;			/* need to reread data*/
	NEUINT32	status;				/*socket state */
	nesocket_t	fd ;				/* socket file description */
	netime_t	last_push ;			/* time last push send buffer*/
		
	ne_mutex			*send_lock;			/* sender lock*/
	//struct ne_linebuf	recv_buffer ;		/* buffer store data recv from net */
	//struct ne_linebuf	send_buffer ;		/* buffer store data send from net */
	ne_netbuf_t recv_buffer, send_buffer;
};

#define check_connect_valid(node) (((struct ne_tcp_node*)(node))->fd > 0 && ((struct ne_tcp_node*)(node))->status==ETS_CONNECTED)

//connect to host
NE_NET_API NEINT32 ne_tcpnode_connect(NEINT8 *host, NEINT32 port, struct ne_tcp_node *node);	//���ӵ�����
NE_NET_API NEINT32 ne_tcpnode_close(struct ne_tcp_node *node, NEINT32 force);				//�ر�����
NE_NET_API NEINT32 ne_tcpnode_send(struct ne_tcp_node *node, ne_packhdr_t *msg_buf, NEINT32 flag) ;	//����������Ϣ flag ref send_flag
NE_NET_API NEINT32 ne_tcpnode_read(struct ne_tcp_node *node) ;		//��ȡ����
NE_NET_API NEINT32 _tcpnode_push_sendbuf(struct ne_tcp_node *conn_node, NEINT32 force) ;	//���ͻ����е�����
NE_NET_API NEINT32 ne_tcpnode_tryto_flush_sendbuf(struct ne_tcp_node *conn_node) ;	//���ͻ����е�����

NE_NET_API NEINT32 tcpnode_parse_recv_msgex(struct ne_tcp_node *node,NENET_MSGENTRY msg_entry , void *param) ;
NE_NET_API void ne_tcpnode_init(struct ne_tcp_node *conn_node) ;	//��ʼ�����ӽڵ�

NE_NET_API void ne_tcpnode_deinit(struct ne_tcp_node *conn_node)  ;
NE_NET_API NEINT32 ne_tcpnode_sendlock_init(struct ne_tcp_node *conn_node) ;
NE_NET_API void ne_tcpnode_sendlock_deinit(struct ne_tcp_node *conn_node) ;

/*����TCP����:�رյ�ǰ���Ӻ�,��ջ���͸���״̬;
 * ���Ǳ����û����������,��Ϣ�������ͼ�����Կ
 */
NE_NET_API void ne_tcpnode_reset(struct ne_tcp_node *conn_node)  ;
NE_NET_API NEINT32 ne_socket_wait_writablity(nesocket_t fd,NEINT32 timeval) ;
NE_NET_API NEINT32 _set_socket_addribute(nesocket_t sock_fd) ;

/*���º�����ס����
 * ���û������в���Ҫ��ʽ�ĵ���
 */

static __INLINE__ void ne_tcpnode_lock(struct ne_tcp_node *node)
{
	if(node->send_lock)
		ne_mutex_lock(node->send_lock) ;
}
static __INLINE__ NEINT32 ne_tcpnode_trytolock(struct ne_tcp_node *node)
{
	if(node->send_lock)
		return ne_mutex_trylock(node->send_lock) ;
	else 
		return 0;
}
static __INLINE__ void ne_tcpnode_unlock(struct ne_tcp_node *node)
{
	if(node->send_lock)
		ne_mutex_unlock(node->send_lock) ;
}

#define TCPNODE_FD(conn_node) (((struct ne_tcp_node*)(conn_node))->fd )
#define TCPNODE_READ_AGAIN(conn_node) (conn_node)->read_again
//TCP״̬������
#define TCPNODE_STATUS(conn_node) (((struct ne_tcp_node*)(conn_node))->status )
#define TCPNODE_SET_OK(conn_node) ((struct ne_tcp_node*)(conn_node))->status = ETS_CONNECTED
#define TCPNODE_CHECK_OK(conn_node) (((struct ne_tcp_node*)(conn_node))->status == ETS_CONNECTED)
#define TCPNODE_CHECK_CLOSED(conn_node) (((struct ne_tcp_node*)(conn_node))->status == ETS_TRYTO_CLOSE)
#define TCPNODE_SET_CLOSED(conn_node) (((struct ne_tcp_node*)(conn_node))->status = ETS_TRYTO_CLOSE)
#define TCPNODE_SET_RESET(conn_node) (((struct ne_tcp_node*)(conn_node))->status = ETS_RESET)
#define TCPNODE_CHECK_RESET(conn_node) (((struct ne_tcp_node*)(conn_node))->status == ETS_RESET)

#define ne_tcpnode_flush_sendbuf(node)	_tcpnode_push_sendbuf(node,0)
#define ne_tcpnode_flush_sendbuf_force(node)	_tcpnode_push_sendbuf(node,1)

NE_NET_API NEINT32 ne_set_wait_writablity_time(NEINT32 newtimeval) ;
NE_NET_API NEINT32 ne_get_wait_writablity_time() ;


/*�ȴ�һ��������Ϣ��Ϣ
 *�����������Ϣ�����򷵻���Ϣ�ĳ���(������Ϣ�ĳ���,���������ݳ���)
 *��ʱ,������-1,��ʱ������Ҫ���ر�
 *����0��ʾ�����ݵ���
 */
NE_NET_API NEINT32 tcpnode_wait_msg(struct ne_tcp_node *node, netime_t tmout);
/* tcpnode_wait_msg ����ɹ��ȴ�һ��������Ϣ,
 * ��ô���ڿ���ʹ��get_net_msg��������Ϣ��������ȡһ����Ϣ
 */
NE_NET_API ne_packhdr_t* tcpnode_get_msg(struct ne_tcp_node *node); 

/*ɾ���Ѿ����������Ϣ*/
NE_NET_API void tcpnode_del_msg(struct ne_tcp_node *node, ne_packhdr_t *msgaddr) ;


#endif

