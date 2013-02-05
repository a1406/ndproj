#ifndef _NE_USERTH_H_
#define _NE_USERTH_H_ 

/*
 * 打开一个专门的线程来调用用户函数处理网络消息和定时器事件
 * 这样做的目的可以使整个服务器逻辑看起来和单线程是一样的
 * 但是消息处理函数和发送函数在用法上将有所不同.
 * 使用单独的用户线程处理消息时,每次处理消息将不在锁定每个消息对于的session.
 * 并且在消息处理函数中只能使用sessionid.
 * 两种处理方式在编程习惯用法上将会有一些不相容的地方.

 * 单线程模式,只是用户逻辑上看起来像单线程.
 * 在消息处理和发送函数上将会有一些出入
 * 消息处理函数使用的时sessionID代替session句柄,
 * 发送函数也一样,这样就避免了对session的锁定,简化了逻辑处理.
 *
 */

enum e_msg_kind{
	E_MSG_KINE_DATA = 0 ,		//数据消息
	E_MSG_KINE_ACCEPT ,			//accept消息
	E_MSG_KINE_CLOSE			//
};

//单线程模式的接入和关闭函数
typedef NEINT32(* st_accept_entry) (NEUINT16 sessionid, SOCKADDR_IN *addr, ne_handle listener)  ;
typedef void(* st_deaccept_entry) (NEUINT16 sessionid, ne_handle listener)  ;

//单线程模式消息处理线程数据结构
typedef struct _msg_queuq_context
{
	neatomic_t connect_num ;
	nethread_t thid ;
	//保存用户消息处理函数
	st_accept_entry income;
	st_deaccept_entry outcome;
}ne_mqc_t;


NE_SRV_API NEINT32 addto_queue(ne_netui_handle  nethandle, NEUINT16 msgkind, ne_usermsgbuf_t *msg,ne_handle h_listen) ;
#define ne_singleth_msghandler(nethandle, msg, h_listen) addto_queue((ne_netui_handle )nethandle,E_MSG_KINE_DATA, (ne_usermsgbuf_t*)msg, (ne_handle)h_listen)

//设置为单线程模式
//在设置其他属性之前先设置这个属性
NE_SRV_API NEINT32 ne_listensrv_set_single_thread(ne_handle h_listen ) ;
NE_SRV_API NEINT32 ne_listensrv_set_entry_st(ne_handle h_listen, st_accept_entry income, st_deaccept_entry outcome) ;
/*在listen句柄上安装消息处理函数, 单线程模式*/
static __INLINE__ NEINT32 ne_srv_msgentry_install_st(ne_handle listen_handle, ne_usermsg_func1 fn, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) 
{
	return ne_srv_msgentry_install( listen_handle, (ne_usermsg_func)fn, maxid, minid,  level) ;
}
//单线程模式下的发送函数
NE_SRV_API NEINT32 ne_st_send(NEUINT16 sessionid , ne_usermsghdr_t  *msg, NEINT32 flag, ne_handle h_listen ) ;

//单线程模式下的关闭函数
NE_SRV_API NEINT32 ne_st_close(NEUINT16 sessionid , NEINT32 flag, ne_handle h_listen ) ;

//设置密码
NE_SRV_API void ne_st_set_crypt(NEUINT16 sessionid, void *key, NEINT32 size,ne_handle h_listen);
#endif 
