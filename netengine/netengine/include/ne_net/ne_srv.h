#ifndef _NE_SRV_H_
#define _NE_SRV_H_

#include "ne_net/ne_sock.h"
#include "ne_common/ne_common.h"

#define START_SESSION_ID 1024  
#define SESSION_ID_MASK   0x8000

#define TCP_SERVER_TYPE ('t'<<8 | 's' )		//定义tcp句柄类型
/*活动连接的管理节点*/
struct cm_manager;

typedef void *(*cm_alloc)(ne_handle alloctor) ;							//从特定分配器上分配一个连接块内存
typedef void (*cm_init)(void *socket_node, ne_handle h_listen) ;		//初始化一个socket
typedef void (*cm_dealloc)(void *socket_node,ne_handle alloctor) ;		//从分配器上释放连接块内存
typedef void(*cm_walk_callback)(void *socket_node, void *param) ;
typedef NEUINT16 (*cm_accept)(struct cm_manager *root, void *socket_node);
typedef NEINT32 (*cm_deaccept)(struct cm_manager *root, NEUINT16 session_id);	//返回0成功，－1失败如果没有返回成功，很可能是引用计数不为0
//typedef NEINT32 (*cm_inc_ref)(struct cm_manager *root, NEUINT16 session_id);	//增加引用次数 返回0成功，－1失败
//typedef void (*cm_dec_ref)(struct cm_manager *root, NEUINT16 session_id);	//减少引用次数
typedef void *(*cm_lock)(struct cm_manager *root, NEUINT16 session_id);
typedef void *(*cm_trylock)(struct cm_manager *root, NEUINT16 session_id);
typedef void (*cm_unlock)(struct cm_manager *root, NEUINT16 session_id);
typedef void (*cm_walk_node)(struct cm_manager *root,cm_walk_callback cb_entry, void *param);
typedef void *(*cm_search)(struct cm_manager *root, NEUINT16 session_id);

typedef struct cmlist_iterator
{
	NEUINT16 session_id ;
	NEUINT16 numbers ;
}cmlist_iterator_t ;

typedef void* (*cm_lock_first)(struct cm_manager *root,cmlist_iterator_t *it);	//所住队列中第一个,输入session_id 0
typedef void* (*cm_lock_next)(struct cm_manager *root, cmlist_iterator_t *it);	//所住下一个,同时释放但前输入session_id但前被锁住的ID,输出session_id已经被所住的下一个ID,并且释放当前被锁住的对象
typedef void (*cm_unlock_iterator)(struct cm_manager *root, cmlist_iterator_t *it) ;
/*记录已经ACCEPT的节点*/
struct cm_node 
{
	neatomic_t __used;			//used status指示此节点是否使用
	NEINT32		 is_mask ;			//是否设置掩码位(为了防止两次申请同一个slot产生同样sessionid,需要在下次产生session时设置mask)
//	NEINT32	wait_realease;			//等待被释放(如果使用计数没有被清0,将会设置此标志,下次继续释放,知道释放成功为止
	ne_mutex	lock ;			//对udt_socket的互斥
	void *client_map ;
};

struct cm_manager
{
	NEINT32				max_conn_num;	//最大连接个数
	neatomic_t		connect_num;	//当前连接数量
	struct cm_node	*connmgr_addr;	//管理udt连接的起始地址

	cm_alloc		alloc;
	cm_init			init ;		//初始化函数
	cm_dealloc		dealloc ;

	//define connect manager function 
	cm_accept		accept ;
	cm_deaccept		deaccept ;
//	cm_inc_ref		inc_ref ;
//	cm_dec_ref		dec_ref ;
	cm_lock			lock;
	cm_trylock		trylock ;
	cm_unlock		unlock ;
	cm_walk_node	walk_node ;
	cm_search		search;

	cm_lock_first	lock_first ;
	cm_lock_next	lock_next ;
	cm_unlock_iterator	unlock_iterator;
};

typedef NEINT32 (*accept_callback) (void* income_handle, SOCKADDR_IN *addr, ne_handle listener) ;	//当有连接接入是执行的回调函数
typedef void (*deaccept_callback) (void* exit_handle, ne_handle listener) ;					//当有连接被释放时执行的回调函数
typedef NEINT32 (*pre_deaccept_callback) (void* handle, ne_handle listener) ;					//释放连接前执行的回调函数

struct ne_srv_node
{
	/*为了保持和 ne_udtsrv 的兼容,前面8个成员位置顺序不能改变*/
	NEUINT32 size ;						/*句柄的大小*/
	NEUINT32 type ;						/*句柄类型*/	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			/*句柄释放函数*/
	
	NEINT32					status;				/*socket state in game 0 not read 1 ready*/
	nesocket_t			fd ;				/* socket file description */
	NEINT32					port ;				/* listen port */
	
	void				*user_msg_entry ;	//用户消息入口
	ne_handle			cm_alloctor;		//连接分配器(提供连接块内存的分配

	accept_callback		connect_in_callback ;		//连接进入通知函数
	pre_deaccept_callback	pre_out_callback ;		//连接需要退出,在退出之前执行的函数,如果返回-1将不会释放连接,下一次继续
	deaccept_callback	connect_out_callback ;		//连接退出通知函数
	struct cm_manager	conn_manager ;				/* 连接管理器*/
};

static __INLINE__ ne_handle ne_srv_get_allocator(struct ne_srv_node *node)
{
	return node->cm_alloctor;
}

static __INLINE__ void ne_srv_set_allocator(struct ne_srv_node *node,ne_handle a)
{
	node->cm_alloctor = a;
}

NE_NET_API void ne_tcpsrv_node_init(struct ne_srv_node *node);
/*加密tcp封包*/
//NE_NET_API void _tcp_encrypt(struct ne_tcp_node *socket_addr,	struct ndnet_msg*msg_buf) ;

//NE_NET_API NEINT32 _tcp_decrypt(struct ne_tcp_node *socket_addr,	struct ndnet_msg*msg_buf) ;
NE_NET_API NEINT32 ne_tcpsrv_open(NEINT32 port,NEINT32 listen_nums,struct ne_srv_node *node) ;	/*open net server*/

NE_NET_API void ne_tcpsrv_close(struct ne_srv_node *node) ; /* close net server*/
NE_NET_API NEINT32 tcpsrv_get_accept(nesocket_t listen_fd,struct ne_tcp_node *node) ;


//设定最大连接数
NE_NET_API NEINT32 cm_listen(struct cm_manager *root, NEINT32 max_num) ;
NE_NET_API void cm_destroy(struct cm_manager *root) ;

//得到最大连接数
NE_NET_API NEINT32 ne_srv_capacity(struct ne_srv_node *srvnode) ;
#endif
