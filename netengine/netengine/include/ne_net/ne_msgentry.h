#ifndef _NE_MSGENTRY_H_
#define _NE_MSGENTRY_H_

#include "ne_net/ne_netpack.h"
#include "ne_net/ne_netui.h"

enum  privalige_level{
	EPL_NONE = 0 ,			//没有任何权限
	EPL_CONNECT,			//普通权限
	EPL_LOGIN ,				//登陆权限
	EPL_HIGHT				//特区
};

#define SUB_MSG_NUM		64		//每一类消息可以有多少个子消息号
#define MAX_MAIN_NUM	256		//最多可以拥有的消息类型

typedef NEUINT16	nemsgid_t ;		//消息编号
typedef NEUINT32	nemsgparam_t ;	//消息参数


#pragma pack(push)
#pragma pack(1)
//用户消息类型 头
typedef struct ne_usermsghdr_t
{
	ne_packhdr_t	packet_hdr ;		//消息包头
	nemsgid_t		maxid ;		//主消息号 16bits
	nemsgid_t		minid ;		//次消息号 16bits
	nemsgparam_t	param;		//消息参数
}ne_usermsghdr_t ;

#define NE_USERMSG_HDRLEN sizeof(ne_usermsghdr_t)
//用户数据长度
#define NE_USERMSG_DATA_CAPACITY  (NE_PACKET_SIZE-sizeof(ne_usermsghdr_t) )		
//定义用户缓冲
typedef struct ne_usermsgbuf_t
{
	ne_usermsghdr_t msg_hdr ;
	NEINT8			data[NE_USERMSG_DATA_CAPACITY] ;
}ne_usermsgbuf_t;

#pragma pack(pop)

static __INLINE__ void ne_usermsghdr_init(ne_usermsghdr_t *hdr)
{
	memset(hdr, 0, sizeof(*hdr)) ;
	hdr->packet_hdr.length = NE_USERMSG_HDRLEN ;
	hdr->packet_hdr.version = 1 ;
}
//#define NE_USERMSG_INITILIZER {{NE_USERMSG_HDRLEN,1,0,0,0},0,0,0} 
//#define NE_PACKHDR_INIT = (ne_packhdr_t){NE_USERMSG_HDRLEN,1,0,0,0};
#define ne_netmsg_hton(m)		//把网络消息转变成主机格式
#define ne_netmsg_ntoh(m)		//把主机格式转变成网络消息

#define NE_USERMSG_LEN(m)	((ne_packhdr_t*)m)->length
#define NE_USERMSG_VERSION(m)	((ne_usermsghdr_t*)m)->version
#define NE_USERMSG_MAXID(m)	((ne_usermsghdr_t*)m)->maxid 
#define NE_USERMSG_MINID(m)	((ne_usermsghdr_t*)m)->minid 
#define NE_USERMSG_PARAM(m)	((ne_usermsghdr_t*)m)->param
#define NE_USERMSG_DATA(m)	(((ne_usermsgbuf_t*)m)->data)
#define NE_USERMSG_DATALEN(m)	(((ne_packhdr_t*)m)->length - NE_USERMSG_HDRLEN)

#define NE_USERMSG_ENCRYPT(m)	((ne_usermsghdr_t*)m)->packet_hdr.encrypt

/* 用户自定义消息处理函数
 * 在网络数据到达时会根据消息号执行相应的处理函数.
 * 函数返回值: 出错时返回-1, 系统会自动关闭对应的连接.
 * 需要关闭连接时,只需要在处理函数中返回-1即可
 */
typedef NEINT32 (*ne_usermsg_func)(ne_handle  handle, ne_usermsgbuf_t *msg , ne_handle listener);

//使用session id的消息处理函数
typedef NEINT32 (*ne_usermsg_func1)(NEUINT16 session_id, ne_usermsgbuf_t *msg , ne_handle listener); 

/* 为连接句柄创建消息入口表
 * @mainmsg_num 主消息的个数(有多数类消息
 * @base_msgid 主消息开始号
 * return value : 0 success on error return -1
 */
NE_NET_API NEINT32 ne_msgtable_create(ne_handle  handle, NEINT32 mainmsg_num, NEINT32 base_msgid) ;

NE_NET_API void ne_msgtable_destroy(ne_handle  handle) ;

/*在handle连接句柄上安装消息处理函数*/
NE_NET_API NEINT32 ne_msgentry_install(ne_handle  handle, ne_usermsg_func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) ;

/* 为listen句柄创建消息入口表
 * @mainmsg_num 主消息的个数(有多数类消息
 * @base_msgid 主消息开始号
 * return value : 0 success on error return -1
 */
NE_NET_API NEINT32 ne_srv_msgtable_create(ne_handle listen_handle, NEINT32 mainmsg_num, NEINT32 base_msgid) ;
NE_NET_API void ne_srv_msgtable_destroy(ne_handle listen_handle) ;

/*在listen句柄上安装消息处理函数*/
NE_NET_API NEINT32 ne_srv_msgentry_install(ne_handle listen_handle, ne_usermsg_func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) ;

/* 正常发送消息 */
/*
static __INLINE__ NEINT32 ne_connectmsg_send(ne_handle  connector_handle, ne_usermsgbuf_t *msg ) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg, ESF_NORMAL) ;
}
*/
/*投递消息,不可靠的发送,消息可能会丢失*/
/*
static __INLINE__ NEINT32 ne_connectmsg_post(ne_handle  connector_handle, ne_usermsgbuf_t *msg ) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg, ESF_POST) ;
}
 */
/* 发送紧急数据 */
  /* 
static __INLINE__ NEINT32 ne_connectmsg_send_urgen(ne_handle  connector_handle, ne_usermsgbuf_t *msg ) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg,  ESF_URGENCY) ;
}
  */
/* 发送扩展函数*/
   /*
  static __INLINE__ NEINT32 ne_connectmsg_sendex(ne_handle  connector_handle, ne_usermsgbuf_t *msg , NEINT32 flag) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg,  flag) ;
}
   */
/*
 * 把网络接口过来的消息传递给用户定义的消息,
 * 一般是网络解析层调用
 * @connect_handle 进入的消息句柄
 * @msg 消息内容
 * @callback_param 用户熟人参数
 */
NE_NET_API NEINT32 ne_translate_message(ne_handle  connector_handle, ne_packhdr_t *msg ) ;


/* 
 * 服务器端消息传递函数
 * 把网络接口过来的消息传递给用户定义的消息,
 * 一般是网络解析层调用
 */
NE_NET_API NEINT32 ne_srv_translate_message(ne_handle listen_handle,ne_handle  connector_handle, ne_packhdr_t *msg ) ;

//使用sessionid 模式
NE_NET_API NEINT32 ne_srv_translate_message1(ne_handle listen_handle,NEUINT16 session_id, ne_packhdr_t *msg ) ;

//权限等级
NE_NET_API NEUINT32 ne_connect_level_get(ne_handle  connector_handle) ;

//权限等级
NE_NET_API void ne_connect_level_set(ne_handle  connector_handle,NEUINT32 level);

#endif

