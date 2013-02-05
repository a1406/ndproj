#ifndef _NE_APPDEF_H_
#define _NE_APPDEF_H_

/* 单线程模式使用的是sessionid ,普通模式使用的是连接句柄
 * 并且发送和处理函数格式上有少许不同,为了统一编程接口,
 * 在这里加了条件编译,主要就是为了解决这个问题
 */

#ifdef SINGLE_THREAD_MOD
	//定义连接标识符
	typedef NEUINT16 NE_SESSION_T ;
	//定义消息处理函数
	#define MSG_ENTRY_INSTANCE(name) \
	NEINT32 name (NEUINT16 nethandle,ne_usermsgbuf_t *msg, ne_handle h_listen) 
	#define MSG_ENTRY_DECLARE(name) \
	CPPAPI NEINT32 name (NEUINT16 nethandle,ne_usermsgbuf_t *msg, ne_handle h_listen)

	//定义消息发送函数
	#define NE_SENDEX(nethandle, msg, flag, h_listen)	ne_st_send(nethandle, (ne_usermsghdr_t *) (msg), flag, h_listen )
    #define NE_MSG_SEND(nethandle, msg,  h_listen)		ne_st_send(nethandle, (ne_usermsghdr_t *) (msg), ESF_NORMAL, h_listen )
	#define NE_MSG_WRITEBUF(nethandle, msg,  h_listen)	ne_st_send(nethandle, (ne_usermsghdr_t *) (msg), ESF_WRITEBUF, h_listen )
	#define NE_MSG_URGEN(nethandle, msg,  h_listen)		ne_st_send(nethandle, (ne_usermsghdr_t *) (msg), ESF_URGENCY, h_listen )
	#define NE_MSG_POST(nethandle, msg,  h_listen)		ne_st_send(nethandle, (ne_usermsghdr_t *) (msg), ESF_POST, h_listen )

	#define NE_CLOSE(nethandle, flag, h_listen)		ne_st_close(nethandle, flag, h_listen ) 
	#define NE_INSTALL_HANDLER						ne_srv_msgentry_install_st
	#define SET_CRYPT( sid, k,  size, h)			ne_st_set_crypt( sid, k,  size, h)

	#define NE_BROAD_CAST(h_listen, msg, sid)		ne_session_broadcast(h_listen, (ne_usermsghdr_t *)(msg), sid) 
	#define NE_SET_ONCONNECT_ENTRY(h, in, pre_out, out) ne_listensrv_set_entry_st(h, in, out)
	#define USER_NETMSG_FUNC	ne_usermsg_func1
#else 
	typedef ne_handle NE_SESSION_T ;
	
	#define MSG_ENTRY_INSTANCE(name) \
	NEINT32 name (ne_handle nethandle,ne_usermsgbuf_t *msg, ne_handle h_listen) 
	#define MSG_ENTRY_DECLARE(name) \
	CPPAPI NEINT32 name (ne_handle nethandle,ne_usermsgbuf_t *msg, ne_handle h_listen)

	//定义消息发送函数
	#define NE_SENDEX(nethandle, msg, flag, h_listen)	ne_sessionmsg_sendex(nethandle, msg, flag )
/*
    #define NE_MSG_SEND(nethandle, msg,  h_listen)		ne_sessionmsg_send(nethandle, msg )
	#define NE_MSG_WRITEBUF(nethandle, msg,  h_listen)	ne_sessionmsg_writebuf(nethandle, msg )
	#define NE_MSG_URGEN(nethandle, msg,  h_listen)		ne_sessionmsg_send_urgen(nethandle, msg )
	#define NE_MSG_POST(nethandle, msg,  h_listen)		ne_sessionmsg_post(nethandle, msg )
*/
	#define NE_CLOSE(nethandle, flag, h_listen)		ne_session_close(nethandle, flag ) 
	#define NE_INSTALL_HANDLER						ne_srv_msgentry_install
	#define SET_CRYPT( sid, k,  size, h)			ne_connector_set_crypt( sid, k,  size)
	#define NE_BROAD_CAST(h_listen, msg, hsession)	ne_session_broadcast(h_listen, (ne_usermsghdr_t *)(msg), ((ne_netui_handle)hsession)->session_id) 
	#define NE_SET_ONCONNECT_ENTRY(h, in, pre_out, out) ne_listensrv_set_entry(h, (accept_callback)in, (pre_deaccept_callback)pre_out, (deaccept_callback)out)
	
	#define USER_NETMSG_FUNC	ne_usermsg_func		
#endif

#endif
