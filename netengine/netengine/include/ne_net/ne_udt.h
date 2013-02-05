#ifndef _NEUDT_H_
#define _NEUDT_H_

//#include "ne_net/ne_netlib.h"
#include "ne_common/ne_common.h"
#include "ne_net/ne_udthdr.h"
#include "ne_net/ne_netcrypt.h"
#include "ne_net/ne_netobj.h"

#define FRAGMENT_BUFF_SIZE	16	//封包缓冲区大小
#define WINDOW_SIZE			C_BUF_SIZE	//发送窗口
#define MAX_ATTEMPT_SEND	5 	//最大重传次数
#define LISTEN_BUF_SIZE		16	//一次最多可以有16个连接在等待建立连接

#define UDT_MAX_PACKET		32	//最多可以发送32个未被确认的封包

#define RETRANSLATE_TIME		10000 //传送往返时间ms
#define WAIT_CONNECT_TIME		15000 //等待建立连接的时间
#define TIME_OUT_BETA			2	//常数加权因子,当超时大于此值 * 往返时间就认为是超时了
#define DELAY_ACK_TIME			10 //ms
#define ACTIVE_TIME				1000*5 //20 seconds 
#define CONNECT_TIMEOUT			5000 //S
#define UPDATE_TIMEVAL			100 //MS 更新网络状态的时间间隔
#define WAIT_RELEASE_TIME		150000	//关闭时等待超时

#define MAX_UDP_LEN				65535
/*网络连接状态*/
enum _enetstat {
	NETSTAT_CLOSED = 0 ,
	NETSTAT_LISTEN =1,
	NETSTAT_SYNSEND =2,
	NETSTAT_SYNRECV =4,
	NETSTAT_ACCEPT =8,		//已经连接成功等待用户accept
	NETSTAT_ESTABLISHED =0x10,	//连接成功
	
	NETSTAT_FINSEND =0x20,	//发送(发送端被关闭)
	NETSTAT_TRYTOFIN =0x40, 	//等待关闭,如果有数据没有发送完就先发送数据

	NETSTAT_SENDCLOSE = 0x80, //写数据关闭
	NETSTAT_RECVCLOSE =0x100, //读数据关闭
	
	NETSTAT_TIME_WAIT = 0x200, //进入超时等待阶段
	NETSTAT_TRYTORESET = 0x400,
	NETSTAT_RESET = 0x800
};

enum UDT_IOCTRL_CMD
{
	UDT_IOCTRL_NONBLOCK = 1 ,		//设置非阻塞/阻塞
	UDT_IOCTRL_GET_SENDBUF,			//得到缓冲
	UDT_IOCTRL_GET_RECVBUF,
	UDT_IOCTRL_SET_STREAM_TPYE,		//0 stream TCP-LIKE ,1 reliable-udp
	UDT_IOCTRL_SET_DATAGRAM_ENTRY,	//set datagram entry function
	UDT_IOCTRL_SET_DATAGRAM_PARAM,	//set datagram entry parameter
	UDT_IOCTRL_DRIVER_MOD			//设置io驱动模型模型(0默认,在send和recv函数中自动驱动,1需要用户显式的使用update_socket)
};

//记录超时重传的样本平均时间何偏差
struct ne_rtt
{
	NEINT32 average  ;		//样本加权平均值
	NEINT32 deviation ;		//样本方差
};

/*保存待确认的封包*/
struct udt_unack
{
	struct list_head my_list ;
	u_16  data_len ;
	u_16  resend_times ;		//重传次数(-1 表示需要立即发送
	
	u_32 sequence ;				//当前包的系列号
	netime_t send_time ;		//发送时间(绝对时间)
	netime_t timeout_val ;		//这个样本的超时值
	struct neudt_header		_hdr ;	//udt packet header
};

typedef struct _s_udt_socket ne_udt_node ;

/*处理报文协议的回调函数,注意data为封包的起始地址,而不是封包中数据的起始地址.
 * 封包有可能是struct neudt_pocket 或者是struct neudp_packet,
 * 因此在处理之前需要先确定封包格式.
 * @len为封包中数据的长度,因为封包头部是变长的.
 */
typedef NEINT32 (*datagram_callback) (ne_udt_node *socket_node, void *data, size_t len,void *param) ;//call back function when datagram come id
/* 定义网络连接节点
 * 用来连接服务器的节点
 * 主要功能是处理发送/接受/缓存/
 * 确认/超时重传等
 */

struct _s_udt_socket
{
	NE_NETOBJ_BASE ;

#if 0
	NEUINT32	length ;				//self lenght sizeof struct ne_tcp_node
	NEUINT32	nodetype;				//node type must be 't'	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;		//关闭函数,为了和handle兼容
	u_32	level ;						//权限登记
	size_t	send_len ;					/* already send valid data length */
	size_t	recv_len ;					/* received valid data length */
	netime_t start_time;				//开始连接时间
	netime_t last_recvpacket ;			//上一次接收到封包的时间
	u_16	session_id ;				//节点 id在一个进程中是唯一的 ;
	u_16	close_reason;		/* reason be closed */
	write_packet_entry	write_entry;	/* send packet entry */	
	void *user_msg_entry ;				//用户消息入口
	void *srv_root;
	void *user_data ;			//user data 
	ne_cryptkey crypt_key ;				//加密解密密钥(对称密钥)
	SOCKADDR_IN dest_addr ;				//目的地网络地址
#endif //0
	u_16 	conn_state ;				//reference enum _enetstat
	u_16	is_accept:4;				//0 connect , 1 accept
	u_16	is_reset:1;
	u_16	nonblock:1;					//0 block 1 nonblock
	u_16	need_ack:1 ;				////在下一此发送数据的时候是否需要带上确认
	u_16	iodrive_mod:1 ;				//驱动模型0默认,在send和recv函数中自动驱动,1需要用户显示的使用update_socket)
	u_16	is_datagram:1;				//0 data stream(like tcp) ,1 datagram(udp protocol)

	nesocket_t listen_fd ;				//if 0 use fd
	NEINT32		resend_times ;				//重发的次数,主要是fin和connect
	netime_t retrans_timeout ;			//超时重传时间和等待关闭时间(记录时间间隔不是绝对时间)
	netime_t last_active ;				//上一次发送封包时间(绝对时间如果太长就超时,或者发送alive包)
	netime_t last_send ;				//上一次发送数据时间,如果长时间没有发送则送出alive数据报
	
	u_32 send_sequence ;				//当前发送的的系列号
	u_32 acknowledged_seq ;				//已经被对方确认的系列号 (send_sequence - acknowledged_seq)就是没有发送窗口中未被确认的
	u_32 received_sequence ;			//接受到的对方的系列号
	size_t window_len ;					//对方接收窗口的长度
	
	datagram_callback datagram_entry ;	//接收datagram的回调函数(处理udt和udp包)
	void *callback_param ;				//接收datagram的回调函数的参数
	
	ne_mutex			*send_lock ;		//send metux
	ne_mutex			__lock ;			//lock this node 

	struct ne_rtt		_rtt ;				//记录样本往返时间

	struct list_head	unack_queue ;		//未被通知的队列(先进先出
	struct list_head	unack_free ;		//保存ne_unack的空闲节点
	struct udt_unack	__unack_buf[UDT_MAX_PACKET];

	//struct ne_linebuf	_recv_buf ;		//接受数据缓冲区
	//struct ne_linebuf	_send_buf ;		//发送数据缓冲区
	ne_netbuf_t _recv_buf, _send_buf;
} ;

#define UDTSO_SET_RESET(udt_socket)  (udt_socket)->is_reset = 1
#define UDTSO_IS_RESET(udt_socket)  (udt_socket)->is_reset

//得到超时重传的时间,绝对时间
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

//锁住整个socket
#define udt_lock(sock) ne_mutex_lock(&(sock)->__lock)
#define udt_unlock(sock) ne_mutex_unlock(&(sock)->__lock)
#define udt_trytolock(sock) ne_mutex_trylock(&(sock)->__lock)

#include "ne_net/ne_udtsrv.h"


NEINT32 _handle_ack(ne_udt_node *socket_node, u_32 ack_seq);

NEINT32 _handle_fin(ne_udt_node* socket_node, struct neudt_pocket *pocket);

NEINT32 _udt_syn(ne_udt_node *socket_node) ;

NEINT32 _handle_income_data(ne_udt_node* socket_node, struct neudt_pocket *pocket, size_t len);

NEINT32 _udt_fin(ne_udt_node *socket_node);

/*从系统的socket fd 中读取一个udt包*/
NEINT32 _recvfrom_udt_packet(nesocket_t fd , udt_pocketbuf* buf, SOCKADDR_IN* from_addr,netime_t outval);

//Wait and read data from ne_udt_node
//return 0 no data in coming, -1 error check error code ,else return lenght of in come data
NEINT32 _wait_data(ne_udt_node *socket_node,udt_pocketbuf* buf,netime_t outval) ;

NEINT32 write_pocket_to_socket(ne_udt_node *socket_node,struct neudt_pocket *pocket, size_t len) ;

NEINT32 _handle_syn(ne_udt_node *socket_node,struct neudt_pocket *pocket);
/*处理udt协议包*/
NEINT32 _udt_packet_handler(ne_udt_node *socket_node,struct neudt_pocket *pocket,size_t len);

NEINT32 udt_send_ack(ne_udt_node *socket_node) ;

//return value :  return value >0 data is handle success , 
// on error return -1 check error code
//return 0 read data over , closed by remote peer
//把从网络进入的数据分解成udt协议数据
NEINT32 udt_parse_rawpacket(ne_udt_node *socket_node,void *data,size_t len ) ;

NEINT8 *send_window_start(ne_udt_node* socket_node, NEINT32 *sendlen) ;

void udt_conn_timeout(ne_udt_node* socket_node) ;

netime_t calc_timeouval(ne_udt_node *socket_node, NEINT32 measuerment) ;

void send_reset_packet(ne_udt_node* socket_node) ;

NE_NET_API void ne_udtnode_init(ne_udt_node *socket_node);

NE_NET_API void _deinit_udt_socket(ne_udt_node *socket_node) ;
/*重置UDT连接:关闭当前连接和,清空缓冲和各种状态;
 * 但是保存用户的相关设置,消息处理函数和加密密钥
 */
NE_NET_API void ne_udtnode_reset(ne_udt_node *socket_node) ;
/*初始化和释放发送互斥,如果需要使用发送互斥就初始化即可,无需在发送时显式调用*/
NE_NET_API NEINT32 udt_init_sendlock(ne_udt_node *socket_node);
NE_NET_API void udt_release_sendlock(ne_udt_node *socket_node);

/*驱动UDT,进行延迟发送,数据确认,超时重传,保持连接存活等
 * return value : -1 error check error code
 * return 0 , received data over, closed by remote peer
 * else return > 0
 */
NE_NET_API NEINT32 update_socket(ne_udt_node* socket_node) ;

//重置一个连接
NE_NET_API void udt_reset(ne_udt_node* socket_node,NEINT32 issend_reset) ;

//关闭一个连接
NE_NET_API NEINT32 udt_close(ne_udt_node* socket_node,NEINT32 force);

//连接到服务器
NE_NET_API ne_udt_node* udt_connect(NEINT8 *host, NEINT16 port, ne_udt_node *socket_node) ;

//发送可靠的流式协议
NE_NET_API NEINT32 udt_recv(ne_udt_node* socket_node,void *buf, NEINT32 len ) ;
NE_NET_API NEINT32 udt_send(ne_udt_node* socket_node,void *data, NEINT32 len ) ;
/* 发送一个封装好的数据包
 * 为了避免把数据再次copy到neudt_pocket中
 * @data_len 输入neudt_pocket::data的长度
 */
NE_NET_API NEINT32 udt_sendex(ne_udt_node* socket_node,struct neudt_pocket *packet, NEINT32 data_len ) ;

//利用udt端口发送udp协议(不可靠的报文协议)
//NE_NET_API NEINT32 ndudp_recvfrom(ne_udt_node* socket_node,void *buf, NEINT32 len ) ;
NE_NET_API NEINT32 ndudp_sendto(ne_udt_node* socket_node,void *data, NEINT32 len ) ;

/*得到可以发送数据的长度(发送缓冲的长度*/
static __INLINE__ size_t udt_send_len(ne_udt_node* socket_node) 
{
	return nelbuf_freespace(&socket_node->_send_buf);
}

//发送一个已经格式化好的udp协议
NE_NET_API NEINT32 ndudp_sendtoex(ne_udt_node* socket_node,struct neudp_packet *packet, NEINT32 len ) ;

//like ioctl system call cmd reference enum UDT_IOCTRL_CMD
NE_NET_API NEINT32 udt_ioctl(ne_udt_node* socket_node, NEINT32 cmd, u_32 *val) ;
NE_NET_API NEINT32 udt_set_nonblock(ne_udt_node* socket_node , NEINT32 flag) ;	//flag = 0 set socket block ,else set nonblock

//发送UDT ne_packhdr_t 包
NE_NET_API NEINT32 udt_connector_send(ne_udt_node* socket_addr, ne_packhdr_t *msg_buf, NEINT32 flag) ;									
/*
 * udt_cli_xxx() 系列函数提供了和标准socket类似的操作函数
 * 主要用在品p2p中需要发送打洞程序并接收消息
 */
/*NE_NET_API ne_udt_node* udt_cli_socket(NEINT32 af,NEINT32 type,NEINT32 protocol) ;
NE_NET_API NEINT32 udt_cli_connect(ne_udt_node *socket_node, NEINT16 port, NEINT8 *host) ;
NE_NET_API NEINT32 udt_cli_bind(ne_udt_node* socket_node ,const struct sockaddr *addr, NEINT32 namelen) ;
NE_NET_API NEINT32 udt_cli_close(ne_udt_node* socket_node, NEINT32 force );
*/
#endif
