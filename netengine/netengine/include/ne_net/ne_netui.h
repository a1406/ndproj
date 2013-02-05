/* 定义统一的网络用户接口,主要是针对每个连接的
 * 服务器端的启动结束接口不在此范围.
 * 目的是为了让udt和tcp使用统一的接口函数
 * 使用统一的消息结构.
 * 
 * 这里的定义都是面向网络层的,用户在使用消息发送函数时,
 * 请勿直接使用这里定的 ne_connector_send() 函数,这里发送的是封包,
 * 请使用 ne_msgentry.h 定义的 ne_connectmsg_send** 系列函数
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
	NE_TCP_STREAM = 0 ,		//使用tcp协议连接
	NE_UDT_STREAM 	//,		//使用udt的stream协议
	//NE_UDT_DATAGRAM			//使用udt的datagram协议
};

/*网络连接或绘话句柄,成员内容只读*/
typedef struct netui_info
{
	NE_NETOBJ_BASE ;
#if 0
	NEUINT32	length ;					//self lenght sizeof struct ne_tcp_node
	NEUINT32	nodetype;					//node type must be 't'	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			//关闭函数,为了和handle兼容
	NEUINT32	level ;						//权限登记
	size_t		send_len ;					/* already send valid data length */
	size_t		recv_len ;					/* received valid data length */
	netime_t	start_time ;				/* net connect or start time*/
	netime_t	last_recv ;					/* last recv packet time */
	NEUINT16	session_id;		
	NEUINT16	close_reason;				/* reason be closed */
	write_packet_entry	write_entry;		/* send packet entry */	
	void *user_msg_entry ;					//用户消息入口
	void *srv_root;
	void *user_data ;						//user data 
	ne_cryptkey	crypt_key ;					/* crypt key*/
	struct sockaddr_in 	remote_addr ;	
#endif //0
}*ne_netui_handle;

/*从连接句柄得到网络接口信息*/
static __INLINE__ struct netui_info *ne_get_netui(ne_handle handle)
{
	ne_assert(((struct tag_ne_handle*)handle)->type==NE_TCP || ((struct tag_ne_handle*)handle)->type==NE_UDT) ;	
	return (struct netui_info *)handle ;
};

//把udt的stream变成nd的网络消息
//NE_NET_API 

/* 发送网络消息 
 * @net_handle 网络连接的句柄,指向struct ne_tcp_node(TCP连接)
 *		或者ndudt_socket(UDT)节点
 * @ne_msgui_buf 发送消息内容
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
 * 把已经创建的句柄连接到主机host的端口 port上
 * 用法: 
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
 * 关闭网络连接并重新初始化连接状态,但保留用户设置信息(消息处理函数,加密密钥)
 */
NE_NET_API NEINT32 ne_connector_reset(ne_handle net_handle) ;

/*销毁连接器*/
NEINT32 _connector_destroy(ne_handle net_handle, NEINT32 force) ;
/*更新,驱动网络模块, 处理连接消息
 * 主要是用在处理connect端,server端不在次定义
 * 出错放回-1,网络需要被关闭
 * 返回0等待超时
 * on error return -1,check errorcode , 
 * return nothing to be done
 * else success
 * if return -1 connect need to be closed
 */
NE_NET_API NEINT32 ne_connector_update(ne_handle net_handle, netime_t timeout) ;

/* 得到发送缓冲的空闲长度*/
NE_NET_API size_t ne_connector_sendlen(ne_handle net_handle);

NE_NET_API void ne_connector_set_crypt(ne_handle net_handle, void *key, NEINT32 size);

NE_NET_API NEINT32 ne_connector_check_crypt(ne_handle net_handle) ;
/*等待一个网络消息消息
 *如果有网络消息到了则返回消息的长度(整条消息的长度,到来的数据长度)
 *超时,出错返回-1.网络被关闭返回0
 *
 */
NE_NET_API NEINT32 ne_connector_waitmsg(ne_handle net_handle, ne_packetbuf_t *msg_hdr, netime_t tmout);

NE_NET_API NEINT32 ne_packet_encrypt(ne_handle net_handle, ne_packetbuf_t *msgbuf);
NE_NET_API NEINT32 ne_packet_decrypt(ne_handle net_handle, ne_packetbuf_t *msgbuf);

//get net connector  or net session start time/connect in time
//static __INLINE__ netime_t ne_connector_starttime(ne_handle net_handle) 
/*处理UDT中的DATAGRAM协议*/
NE_NET_API NEINT32 _datagram_entry(ne_udt_node *socket, void *data, size_t len,void *param) ;

/* 把udt的stream数据编程消息模式,并输入给msg_entry函数执行
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
