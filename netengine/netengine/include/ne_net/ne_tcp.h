#ifndef _NE_TCP_H_
#define _NE_TCP_H_

#include "ne_net/byte_order.h"
#include "ne_net/ne_sock.h"
#include "ne_common/ne_common.h"
#include "ne_net/ne_netcrypt.h"

#include "ne_net/ne_netpack.h"

#include "ne_net/ne_netobj.h"

#define SENDBUF_PUSH	512		/*发送缓冲上限,如果数据达到这个数即可发送*/
#define SENDBUF_TMVAL	100		/*两次缓冲清空时间间隔,超过这个间隔将清空*/
#define ALONE_SENE_SIZE 256		/*发送数据长度大于这个数字将不缓冲*/

/* 
 * WAIT_WRITABLITY_TIME 等待一个连接变为可写的时间.
 * 如果设置太长可能会使系统挂起,影响整个服务器的吞吐率.
 * 可以使用ne_set_wait_writablity_time 函数来重新设置等待时间
 */
#ifdef NE_DEBUG
#define WAIT_WRITABLITY_TIME	-1		/*无限等待*/
#else 
#define WAIT_WRITABLITY_TIME    1000	/*等待socke可写的时间*/
#endif

//TCP连接状态
enum ETCP_CONNECT_STATUS{
	ETS_DEAD = 0 ,			//无连接(或者等待被释放)
	ETS_ACCEPT,				//等待连接进入(IOCP)
	ETS_CONNECTED ,			//连接成功
	ETS_TRYTO_CLOSE	,		//等待关闭连接need to send data can be receive data
	ETS_RESET				//need to be reset (force close). socket would be set reset status on error when write data
};

/*定义连接节点的属性,使用tcp或者是udp CONNECT NODE*/
enum ENET_CONNECT_TYPE{
	NE_TCP = 't',
	NE_UDT = 'u'
};
struct ne_tcp_node;
typedef NEINT32 (*write_data_entry)(struct ne_tcp_node *node,void *data , size_t len) ; //发送数据函数

typedef NEINT32 (*write_packet_entry)(ne_handle net_handle, ne_packhdr_t *msg_buf, NEINT32 flag) ;	//define extend send function
/* socket connect info struct */
struct ne_tcp_node{	
	NE_NETOBJ_BASE ;
#if 0
	NEUINT32	length ;					//self lenght sizeof struct ne_tcp_node
	NEUINT32	nodetype;					//node type must be 't'	unsigned	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			//关闭函数,为了和handle兼容
	NEUINT32	level ;						//权限登记
	size_t		send_len ;			/* already send data length */
	size_t		recv_len ;			/* received data length */
	netime_t	start_time ;		/* net connect or start time*/
	netime_t	last_recv ;			/* last recv packet time */
	NEUINT16	session_id;				
	NEUINT16	close_reason;		/* reason be closed */
	write_packet_entry	write_entry;	/* send packet entry */
	void		*user_msg_entry ;		//用户消息入口
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
NE_NET_API NEINT32 ne_tcpnode_connect(NEINT8 *host, NEINT32 port, struct ne_tcp_node *node);	//连接到主机
NE_NET_API NEINT32 ne_tcpnode_close(struct ne_tcp_node *node, NEINT32 force);				//关闭连接
NE_NET_API NEINT32 ne_tcpnode_send(struct ne_tcp_node *node, ne_packhdr_t *msg_buf, NEINT32 flag) ;	//发送网络消息 flag ref send_flag
NE_NET_API NEINT32 ne_tcpnode_read(struct ne_tcp_node *node) ;		//读取数据
NE_NET_API NEINT32 _tcpnode_push_sendbuf(struct ne_tcp_node *conn_node, NEINT32 force) ;	//发送缓冲中的数据
NE_NET_API NEINT32 ne_tcpnode_tryto_flush_sendbuf(struct ne_tcp_node *conn_node) ;	//发送缓冲中的数据

NE_NET_API NEINT32 tcpnode_parse_recv_msgex(struct ne_tcp_node *node,NENET_MSGENTRY msg_entry , void *param) ;
NE_NET_API void ne_tcpnode_init(struct ne_tcp_node *conn_node) ;	//初始化连接节点

NE_NET_API void ne_tcpnode_deinit(struct ne_tcp_node *conn_node)  ;
NE_NET_API NEINT32 ne_tcpnode_sendlock_init(struct ne_tcp_node *conn_node) ;
NE_NET_API void ne_tcpnode_sendlock_deinit(struct ne_tcp_node *conn_node) ;

/*重置TCP连接:关闭当前连接和,清空缓冲和各种状态;
 * 但是保存用户的相关设置,消息处理函数和加密密钥
 */
NE_NET_API void ne_tcpnode_reset(struct ne_tcp_node *conn_node)  ;
NE_NET_API NEINT32 ne_socket_wait_writablity(nesocket_t fd,NEINT32 timeval) ;
NE_NET_API NEINT32 _set_socket_addribute(nesocket_t sock_fd) ;

/*以下函数锁住发送
 * 在用户程序中不需要显式的调用
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
//TCP状态操作宏
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


/*等待一个网络消息消息
 *如果有网络消息到了则返回消息的长度(整条消息的长度,到来的数据长度)
 *超时,出错返回-1,此时网络需要被关闭
 *返回0表示无数据到了
 */
NE_NET_API NEINT32 tcpnode_wait_msg(struct ne_tcp_node *node, netime_t tmout);
/* tcpnode_wait_msg 如果成功等待一个网络消息,
 * 那么现在可以使用get_net_msg函数从消息缓冲中提取一个消息
 */
NE_NET_API ne_packhdr_t* tcpnode_get_msg(struct ne_tcp_node *node); 

/*删除已经处理过的消息*/
NE_NET_API void tcpnode_del_msg(struct ne_tcp_node *node, ne_packhdr_t *msgaddr) ;


#endif

