#ifndef _NEUDT_H_
#define _NEUDT_H_

//#include "ne_net/ne_netlib.h"
#include "ne_common/ne_common.h"
#include "ne_net/ne_udthdr.h"
#include "ne_net/ne_netcrypt.h"
#include "ne_net/ne_netobj.h"

#define FRAGMENT_BUFF_SIZE	16	//�����������С
#define WINDOW_SIZE			C_BUF_SIZE	//���ʹ���
#define MAX_ATTEMPT_SEND	5 	//����ش�����
#define LISTEN_BUF_SIZE		16	//һ����������16�������ڵȴ���������

#define UDT_MAX_PACKET		32	//�����Է���32��δ��ȷ�ϵķ��

#define RETRANSLATE_TIME		10000 //��������ʱ��ms
#define WAIT_CONNECT_TIME		15000 //�ȴ��������ӵ�ʱ��
#define TIME_OUT_BETA			2	//������Ȩ����,����ʱ���ڴ�ֵ * ����ʱ�����Ϊ�ǳ�ʱ��
#define DELAY_ACK_TIME			10 //ms
#define ACTIVE_TIME				1000*5 //20 seconds 
#define CONNECT_TIMEOUT			5000 //S
#define UPDATE_TIMEVAL			100 //MS ��������״̬��ʱ����
#define WAIT_RELEASE_TIME		150000	//�ر�ʱ�ȴ���ʱ

#define MAX_UDP_LEN				65535
/*��������״̬*/
enum _enetstat {
	NETSTAT_CLOSED = 0 ,
	NETSTAT_LISTEN =1,
	NETSTAT_SYNSEND =2,
	NETSTAT_SYNRECV =4,
	NETSTAT_ACCEPT =8,		//�Ѿ����ӳɹ��ȴ��û�accept
	NETSTAT_ESTABLISHED =0x10,	//���ӳɹ�
	
	NETSTAT_FINSEND =0x20,	//����(���Ͷ˱��ر�)
	NETSTAT_TRYTOFIN =0x40, 	//�ȴ��ر�,���������û�з�������ȷ�������

	NETSTAT_SENDCLOSE = 0x80, //д���ݹر�
	NETSTAT_RECVCLOSE =0x100, //�����ݹر�
	
	NETSTAT_TIME_WAIT = 0x200, //���볬ʱ�ȴ��׶�
	NETSTAT_TRYTORESET = 0x400,
	NETSTAT_RESET = 0x800
};

enum UDT_IOCTRL_CMD
{
	UDT_IOCTRL_NONBLOCK = 1 ,		//���÷�����/����
	UDT_IOCTRL_GET_SENDBUF,			//�õ�����
	UDT_IOCTRL_GET_RECVBUF,
	UDT_IOCTRL_SET_STREAM_TPYE,		//0 stream TCP-LIKE ,1 reliable-udp
	UDT_IOCTRL_SET_DATAGRAM_ENTRY,	//set datagram entry function
	UDT_IOCTRL_SET_DATAGRAM_PARAM,	//set datagram entry parameter
	UDT_IOCTRL_DRIVER_MOD			//����io����ģ��ģ��(0Ĭ��,��send��recv�������Զ�����,1��Ҫ�û���ʽ��ʹ��update_socket)
};

//��¼��ʱ�ش�������ƽ��ʱ���ƫ��
struct ne_rtt
{
	NEINT32 average  ;		//������Ȩƽ��ֵ
	NEINT32 deviation ;		//��������
};

/*�����ȷ�ϵķ��*/
struct udt_unack
{
	struct list_head my_list ;
	u_16  data_len ;
	u_16  resend_times ;		//�ش�����(-1 ��ʾ��Ҫ��������
	
	u_32 sequence ;				//��ǰ����ϵ�к�
	netime_t send_time ;		//����ʱ��(����ʱ��)
	netime_t timeout_val ;		//��������ĳ�ʱֵ
	struct neudt_header		_hdr ;	//udt packet header
};

typedef struct _s_udt_socket ne_udt_node ;

/*������Э��Ļص�����,ע��dataΪ�������ʼ��ַ,�����Ƿ�������ݵ���ʼ��ַ.
 * ����п�����struct neudt_pocket ������struct neudp_packet,
 * ����ڴ���֮ǰ��Ҫ��ȷ�������ʽ.
 * @lenΪ��������ݵĳ���,��Ϊ���ͷ���Ǳ䳤��.
 */
typedef NEINT32 (*datagram_callback) (ne_udt_node *socket_node, void *data, size_t len,void *param) ;//call back function when datagram come id
/* �����������ӽڵ�
 * �������ӷ������Ľڵ�
 * ��Ҫ�����Ǵ�����/����/����/
 * ȷ��/��ʱ�ش���
 */

struct _s_udt_socket
{
	NE_NETOBJ_BASE ;

#if 0
	NEUINT32	length ;				//self lenght sizeof struct ne_tcp_node
	NEUINT32	nodetype;				//node type must be 't'	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;		//�رպ���,Ϊ�˺�handle����
	u_32	level ;						//Ȩ�޵Ǽ�
	size_t	send_len ;					/* already send valid data length */
	size_t	recv_len ;					/* received valid data length */
	netime_t start_time;				//��ʼ����ʱ��
	netime_t last_recvpacket ;			//��һ�ν��յ������ʱ��
	u_16	session_id ;				//�ڵ� id��һ����������Ψһ�� ;
	u_16	close_reason;		/* reason be closed */
	write_packet_entry	write_entry;	/* send packet entry */	
	void *user_msg_entry ;				//�û���Ϣ���
	void *srv_root;
	void *user_data ;			//user data 
	ne_cryptkey crypt_key ;				//���ܽ�����Կ(�Գ���Կ)
	SOCKADDR_IN dest_addr ;				//Ŀ�ĵ������ַ
#endif //0
	u_16 	conn_state ;				//reference enum _enetstat
	u_16	is_accept:4;				//0 connect , 1 accept
	u_16	is_reset:1;
	u_16	nonblock:1;					//0 block 1 nonblock
	u_16	need_ack:1 ;				////����һ�˷������ݵ�ʱ���Ƿ���Ҫ����ȷ��
	u_16	iodrive_mod:1 ;				//����ģ��0Ĭ��,��send��recv�������Զ�����,1��Ҫ�û���ʾ��ʹ��update_socket)
	u_16	is_datagram:1;				//0 data stream(like tcp) ,1 datagram(udp protocol)

	nesocket_t listen_fd ;				//if 0 use fd
	NEINT32		resend_times ;				//�ط��Ĵ���,��Ҫ��fin��connect
	netime_t retrans_timeout ;			//��ʱ�ش�ʱ��͵ȴ��ر�ʱ��(��¼ʱ�������Ǿ���ʱ��)
	netime_t last_active ;				//��һ�η��ͷ��ʱ��(����ʱ�����̫���ͳ�ʱ,���߷���alive��)
	netime_t last_send ;				//��һ�η�������ʱ��,�����ʱ��û�з������ͳ�alive���ݱ�
	
	u_32 send_sequence ;				//��ǰ���͵ĵ�ϵ�к�
	u_32 acknowledged_seq ;				//�Ѿ����Է�ȷ�ϵ�ϵ�к� (send_sequence - acknowledged_seq)����û�з��ʹ�����δ��ȷ�ϵ�
	u_32 received_sequence ;			//���ܵ��ĶԷ���ϵ�к�
	size_t window_len ;					//�Է����մ��ڵĳ���
	
	datagram_callback datagram_entry ;	//����datagram�Ļص�����(����udt��udp��)
	void *callback_param ;				//����datagram�Ļص������Ĳ���
	
	ne_mutex			*send_lock ;		//send metux
	ne_mutex			__lock ;			//lock this node 

	struct ne_rtt		_rtt ;				//��¼��������ʱ��

	struct list_head	unack_queue ;		//δ��֪ͨ�Ķ���(�Ƚ��ȳ�
	struct list_head	unack_free ;		//����ne_unack�Ŀ��нڵ�
	struct udt_unack	__unack_buf[UDT_MAX_PACKET];

	//struct ne_linebuf	_recv_buf ;		//�������ݻ�����
	//struct ne_linebuf	_send_buf ;		//�������ݻ�����
	ne_netbuf_t _recv_buf, _send_buf;
} ;

#define UDTSO_SET_RESET(udt_socket)  (udt_socket)->is_reset = 1
#define UDTSO_IS_RESET(udt_socket)  (udt_socket)->is_reset

//�õ���ʱ�ش���ʱ��,����ʱ��
static __INLINE__ netime_t retrans_time(ne_udt_node *nct)
{
	return ne_time() + nct->retrans_timeout * TIME_OUT_BETA ;
}

static __INLINE__ size_t send_window(ne_udt_node *socket_node)
{
	return nelbuf_free_capacity(&socket_node->_recv_buf) ;
}

static __INLINE__ NEINT32 udt_get_error(ne_udt_node *socket_node)
{
	return (NEINT32)( socket_node->myerrno );
}

static __INLINE__ void udt_set_error(ne_udt_node *socket_node, NEINT32 err)
{
	 socket_node->myerrno = (u_32) err;
}

void set_socket_ack(ne_udt_node *socket_node, NEINT32 flag) ;
#define get_socket_ack(socket_node ) (socket_node)->need_ack 

/* send lock udt connect-node*/
static __INLINE__ void udt_send_lock(ne_udt_node *node)
{
	if(node->send_lock)
		ne_mutex_lock(node->send_lock) ;
}
static __INLINE__ NEINT32 udt_send_trytolock(ne_udt_node *node)
{
	if(node->send_lock)
		return ne_mutex_trylock(node->send_lock) ;
	else 
		return 0 ;
}
static __INLINE__ void udt_send_unlock(ne_udt_node *node)
{
	if(node->send_lock)
		ne_mutex_unlock(node->send_lock) ;
}

//��ס����socket
#define udt_lock(sock) ne_mutex_lock(&(sock)->__lock)
#define udt_unlock(sock) ne_mutex_unlock(&(sock)->__lock)
#define udt_trytolock(sock) ne_mutex_trylock(&(sock)->__lock)

#include "ne_net/ne_udtsrv.h"


NEINT32 _handle_ack(ne_udt_node *socket_node, u_32 ack_seq);

NEINT32 _handle_fin(ne_udt_node* socket_node, struct neudt_pocket *pocket);

NEINT32 _udt_syn(ne_udt_node *socket_node) ;

NEINT32 _handle_income_data(ne_udt_node* socket_node, struct neudt_pocket *pocket, size_t len);

NEINT32 _udt_fin(ne_udt_node *socket_node);

/*��ϵͳ��socket fd �ж�ȡһ��udt��*/
NEINT32 _recvfrom_udt_packet(nesocket_t fd , udt_pocketbuf* buf, SOCKADDR_IN* from_addr,netime_t outval);

//Wait and read data from ne_udt_node
//return 0 no data in coming, -1 error check error code ,else return lenght of in come data
NEINT32 _wait_data(ne_udt_node *socket_node,udt_pocketbuf* buf,netime_t outval) ;

NEINT32 write_pocket_to_socket(ne_udt_node *socket_node,struct neudt_pocket *pocket, size_t len) ;

NEINT32 _handle_syn(ne_udt_node *socket_node,struct neudt_pocket *pocket);
/*����udtЭ���*/
NEINT32 _udt_packet_handler(ne_udt_node *socket_node,struct neudt_pocket *pocket,size_t len);

NEINT32 udt_send_ack(ne_udt_node *socket_node) ;

//return value :  return value >0 data is handle success , 
// on error return -1 check error code
//return 0 read data over , closed by remote peer
//�Ѵ������������ݷֽ��udtЭ������
NEINT32 udt_parse_rawpacket(ne_udt_node *socket_node,void *data,size_t len ) ;

NEINT8 *send_window_start(ne_udt_node* socket_node, NEINT32 *sendlen) ;

void udt_conn_timeout(ne_udt_node* socket_node) ;

netime_t calc_timeouval(ne_udt_node *socket_node, NEINT32 measuerment) ;

void send_reset_packet(ne_udt_node* socket_node) ;

NE_NET_API void ne_udtnode_init(ne_udt_node *socket_node);

NE_NET_API void _deinit_udt_socket(ne_udt_node *socket_node) ;
/*����UDT����:�رյ�ǰ���Ӻ�,��ջ���͸���״̬;
 * ���Ǳ����û����������,��Ϣ�������ͼ�����Կ
 */
NE_NET_API void ne_udtnode_reset(ne_udt_node *socket_node) ;
/*��ʼ�����ͷŷ��ͻ���,�����Ҫʹ�÷��ͻ���ͳ�ʼ������,�����ڷ���ʱ��ʽ����*/
NE_NET_API NEINT32 udt_init_sendlock(ne_udt_node *socket_node);
NE_NET_API void udt_release_sendlock(ne_udt_node *socket_node);

/*����UDT,�����ӳٷ���,����ȷ��,��ʱ�ش�,�������Ӵ���
 * return value : -1 error check error code
 * return 0 , received data over, closed by remote peer
 * else return > 0
 */
NE_NET_API NEINT32 update_socket(ne_udt_node* socket_node) ;

//����һ������
NE_NET_API void udt_reset(ne_udt_node* socket_node,NEINT32 issend_reset) ;

//�ر�һ������
NE_NET_API NEINT32 udt_close(ne_udt_node* socket_node,NEINT32 force);

//���ӵ�������
NE_NET_API ne_udt_node* udt_connect(NEINT8 *host, NEINT16 port, ne_udt_node *socket_node) ;

//���Ϳɿ�����ʽЭ��
NE_NET_API NEINT32 udt_recv(ne_udt_node* socket_node,void *buf, NEINT32 len ) ;
NE_NET_API NEINT32 udt_send(ne_udt_node* socket_node,void *data, NEINT32 len ) ;
/* ����һ����װ�õ����ݰ�
 * Ϊ�˱���������ٴ�copy��neudt_pocket��
 * @data_len ����neudt_pocket::data�ĳ���
 */
NE_NET_API NEINT32 udt_sendex(ne_udt_node* socket_node,struct neudt_pocket *packet, NEINT32 data_len ) ;

//����udt�˿ڷ���udpЭ��(���ɿ��ı���Э��)
//NE_NET_API NEINT32 ndudp_recvfrom(ne_udt_node* socket_node,void *buf, NEINT32 len ) ;
NE_NET_API NEINT32 ndudp_sendto(ne_udt_node* socket_node,void *data, NEINT32 len ) ;

/*�õ����Է������ݵĳ���(���ͻ���ĳ���*/
static __INLINE__ size_t udt_send_len(ne_udt_node* socket_node) 
{
	return nelbuf_freespace(&socket_node->_send_buf);
}

//����һ���Ѿ���ʽ���õ�udpЭ��
NE_NET_API NEINT32 ndudp_sendtoex(ne_udt_node* socket_node,struct neudp_packet *packet, NEINT32 len ) ;

//like ioctl system call cmd reference enum UDT_IOCTRL_CMD
NE_NET_API NEINT32 udt_ioctl(ne_udt_node* socket_node, NEINT32 cmd, u_32 *val) ;
NE_NET_API NEINT32 udt_set_nonblock(ne_udt_node* socket_node , NEINT32 flag) ;	//flag = 0 set socket block ,else set nonblock

//����UDT ne_packhdr_t ��
NE_NET_API NEINT32 udt_connector_send(ne_udt_node* socket_addr, ne_packhdr_t *msg_buf, NEINT32 flag) ;									
/*
 * udt_cli_xxx() ϵ�к����ṩ�˺ͱ�׼socket���ƵĲ�������
 * ��Ҫ����Ʒp2p����Ҫ���ʹ򶴳��򲢽�����Ϣ
 */
/*NE_NET_API ne_udt_node* udt_cli_socket(NEINT32 af,NEINT32 type,NEINT32 protocol) ;
NE_NET_API NEINT32 udt_cli_connect(ne_udt_node *socket_node, NEINT16 port, NEINT8 *host) ;
NE_NET_API NEINT32 udt_cli_bind(ne_udt_node* socket_node ,const struct sockaddr *addr, NEINT32 namelen) ;
NE_NET_API NEINT32 udt_cli_close(ne_udt_node* socket_node, NEINT32 force );
*/
#endif
