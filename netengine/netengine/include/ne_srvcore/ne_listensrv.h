#ifndef _NE_LISTENSRV_H_
#define _NE_LISTENSRV_H_

//#include "ne_srvcore/ne_srvlib.h"
#include "ne_srvcore/client_map.h"
#include "ne_srvcore/ne_session.h"
#include "ne_net/ne_udt.h"
#include "ne_srvcore/ne_udtsrv.h"
#include "ne_srvcore/ne_userth.h"

//#define _MAX_CLIENTS	 2		//fot test
#define CONNECTION_TIMEOUT		60 //time out between twice read/write (second)
//#define _AFFIX_DATA_LENT 12		//for test
#define LISTEN_INTERVAL  100    //10hz

enum ENE_LISTEN_MOD{
	NE_LISTEN_COMMON = 0 ,
	NE_LISTEN_OS_EXT,
	NE_LISTEN_UDT_STREAM			//使用UDT服务STREAM
	//NE_LISTEN_UDT_DATAGRAM			//使用 reliable udp
};

#define _IS_UDT_MOD(iomod) ((iomod)==NE_LISTEN_UDT_STREAM )

typedef struct swap_list{
	NEINT32 inited ;
	struct list_head list ;
	ne_mutex		 lock ;
}swap_list_t;

static __INLINE__ void ne_swap_list_init(swap_list_t *sw)
{
	sw->inited = 1 ;
	INIT_LIST_HEAD(&sw->list) ;
	ne_mutex_init(&sw->lock) ;
}
static __INLINE__ void ne_swap_list_destroy(swap_list_t *sw)
{
	if(sw->inited) {
		ne_mutex_destroy(&sw->lock) ;
		sw->inited = 0 ;
	}
}


/* listen service contex*/
struct listen_contex
{
	union {
		struct ne_srv_node	tcp;		//server port
		ne_udtsrv			udt;		//udt socket port
	};
	NEINT32 io_mod ;						//listen module
//	NEINT32 single_thread_mod ;				//是否使用单线程模式
	netime_t	operate_timeout ;				//timeout between twice net read/write (if timeout session would be close)
	volatile ne_thsrvid_t  listen_id ;			//listen thread server id
	volatile ne_thsrvid_t  sub_id ;				//sub thread server id 
	void	 *user_data ;						//user data of listener
	ne_mqc_t	 mqc;							//单线程模式数据结构
	struct list_head conn_list ;				//连接队列
	swap_list_t		wait_free ;					//等待被释放的队列
#ifdef USER_UPDATE_LIST
	swap_list_t		wait_add ;					//等待被添加的队列
#endif	
//	ne_close_callback	session_release_entry ;	//session 的底层关闭函数
//	ne_mutex			wf_lock ;				//lock of wait_free
//	struct list_head	wait_free;				//保存等待释放的队列
} ;

#ifdef IMPLEMENT_LISTEN_HANDLE
	typedef struct listen_contex *ne_listen_handle ;
#else 
	typedef ne_handle	ne_listen_handle ;
#endif

/*打开网络,并且启动监听线程*/
NE_SRV_API NEINT32 ne_listensrv_open(NEINT32 port, ne_listen_handle handle) ;
/*关闭网络关闭监听线程*/
NE_SRV_API NEINT32 ne_listensrv_close(ne_listen_handle handle, NEINT32 force) ;
/*设置对应连接的相关属性并分配内存*/
NE_SRV_API NEINT32 ne_listensrv_session_info(ne_listen_handle handle, NEINT32 max_client,size_t session_size) ;
/*设置连接进入和退出的回调函数*/
NE_SRV_API void ne_listensrv_set_entry(ne_listen_handle handle, accept_callback income,pre_deaccept_callback pre_close, deaccept_callback outcome) ;
/*
 * 设置session的alloc 和dealloc 还要initilizer 函数
 * 一般情况不要单独设置,除非你不想使用默认的分配和初始化函数.
 */
NE_SRV_API void ne_listensrv_alloc_entry(ne_listen_handle handle, cm_alloc al, cm_dealloc dealloc, cm_init init) ;

NE_SRV_API ne_handle ne_listensrv_get_cmallocator(ne_listen_handle handle) ;

//得到连接管理器
NE_SRV_API struct cm_manager *ne_listensrv_get_cmmamager(ne_listen_handle handle) ;

//在监听器上建立定时器
//只有在单线程模式的时候才有必要在监听器上创建定时
//这样可以保证定时器和消息处理都在一个线程上执行
//如果是多线程模式可以建立单独的线程来执行定时器函数这样时间会更精确一些
NE_SRV_API netimer_t ne_listensrv_timer(ne_listen_handle h_listen,ne_timer_entry func,void *param,netime_t interval, NEINT32 run_type ) ;
NE_SRV_API void ne_listensrv_del_timer(ne_listen_handle h_listen, netimer_t timer_id ) ;

/* 释放已经被关闭,但是还在使用的绘话*/
//void release_dead_cm(struct listen_contex * lc) ;
//得到listen file description
static __INLINE__ nesocket_t get_listen_fd(struct listen_contex * handle)
{
	return handle->tcp.fd;
}

struct ne_client_map * accetp_client_connect(struct listen_contex *listen_info) ;

/*listen_msg_entry 进行消息处理回调,是在connect socket的基础上*/
//NEINT32 listen_msg_entry(struct ne_tcp_node*node,struct ndnet_msg *msg,void *param)  ;

NEINT32 srv_stream_data_entry(void *node,ne_packhdr_t *msg,void *param) ;
/*deal with received net message*/
/*NE_SRV_API*/ NEINT32 ne_do_netmsg(struct ne_client_map *cli_map,struct ne_srv_node *srv_node) ;

NE_SRV_API void udt_clientmap_init(struct ne_udtcli_map *node, ne_handle h_listen ) ; 

static __INLINE__ void ne_set_connection_timeout(ne_listen_handle h, NEINT32 second)
{
	((struct listen_contex*)h)->operate_timeout = second * 1000 ;
}

static __INLINE__ netime_t ne_get_connection_timeout(ne_listen_handle h)
{
	return ((struct listen_contex*)h)->operate_timeout;
}

#endif
