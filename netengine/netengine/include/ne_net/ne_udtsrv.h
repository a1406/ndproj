#ifndef _NEUDTSRV_H_
#define _NEUDTSRV_H_

#include "ne_common/ne_common.h"
//#include "ne_net/ne_srv.h"

#define UDT_MAX_CONNECT  10000
#define UDP_SERVER_TYPE  ('u'<<8 | 's')
//数据连接过程管
/************************************************************************/

typedef NEINT32 (*data_recv_callback) (ne_udt_node *new_socket, void *data, size_t len) ;//call back function when data come id
typedef NEINT32 (*data_notify_callback)(ne_udt_node *close_socket, size_t data_len) ;		//通知有数据到来 data_len = 0 closed

typedef struct _ne_udtsrv
{
	/*为了保持和 struct ne_srv_node 的兼容,前面11个成员位置顺序不能改变*/
	NEUINT32 size ;						/*句柄的大小*/
	NEUINT32 type;						/*句柄类型*/	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			/*句柄释放函数*/
	NEINT32			listen_status ;				//
	nesocket_t  listen_fd	 ;				//udp socket所有accept的连接公用这个socket
	NEINT32			port ;						/* listen port */	
	void		*user_msg_entry ;			//用户消息入口
	ne_handle	cm_alloctor;				//连接块分配器
	
	accept_callback		connect_in_callback ;		//连接进入通知函数
	pre_deaccept_callback	pre_out_callback ;		//连接需要退出,在退出之前执行的函数,如果返回-1将不会释放连接,下一次继续
	deaccept_callback	connect_out_callback ;		//连接退出通知函数
	struct cm_manager	conn_manager ;				/* client map mamager */

	//udt server 所特有的结构
	data_recv_callback income_entry ;				//数据处理函数,如果使用此函数则函数返回以后数据自动被删除
	data_notify_callback data_notify_entry ;		//如果安装了该函数,将不会执行income_entry 函数(函数返回以后数据不会被删除)

	ne_mutex	send_lock ;		//发送metux 
	SOCKADDR_IN self_addr ; 	//自己的地址

} ne_udtsrv;

static __INLINE__ void root_send_lock(ne_udtsrv *root)
{
	ne_assert(root) ;
	ne_mutex_lock(&root->send_lock);
}

static __INLINE__ void root_send_unlock(ne_udtsrv *root)
{
	ne_assert(root) ;
	ne_mutex_unlock(&root->send_lock);
}

ne_udt_node *search_socket_node(struct list_head *head,NEINT32 session_id) ;

//释放一个已经关闭的连接
NE_NET_API void release_dead_node(ne_udt_node *socket_node,NEINT32 needcallback) ;

//发送fin并请求关闭连接
void _close_listend_socket(ne_udt_node* socket_node) ;

NE_NET_API ne_udtsrv* udt_socket(NEINT32 af,NEINT32 type,NEINT32 protocol );

NE_NET_API NEINT32 udt_socketex(ne_udtsrv* udt_root );

NE_NET_API NEINT32 udt_bind(ne_udtsrv* listen_node ,const struct sockaddr *addr, NEINT32 namelen);

NE_NET_API NEINT32 udt_listen(ne_udtsrv* listen_node,NEINT32 listen_number);

//创建并绑定一个udp服务端口
//如果使用次函数 udt_socket, udt_bind, udt_listen 都可以不显示调用
NE_NET_API ne_udtsrv* udt_open_srv(NEINT32 port);

NE_NET_API NEINT32 udt_open_srvex(NEINT32 port, ne_udtsrv *listen_root) ;

//关闭udt服务
NE_NET_API void udt_close_srv(ne_udtsrv *root , NEINT32 force);
NE_NET_API void udt_close_srvex(ne_udtsrv *root , NEINT32 force) ;

//把socket_node添加到root的等待队列中
NE_NET_API NEINT32 udt_pre_acceptex(ne_udtsrv *root, ne_udt_node *socket_node);

//read income data and accept new connect
NE_NET_API NEINT32 udt_bindio_handler(ne_udtsrv *root, accept_callback accept_entry, data_recv_callback recv_entry,deaccept_callback release_entry);
NE_NET_API void udt_set_notify_entry(ne_udtsrv *root,data_notify_callback entry);

/* 安装连接节点管理程序 
 * 申请,释放和管理
 */
NE_NET_API void udt_install_connect_manager(ne_udtsrv *root,cm_alloc alloc_entry ,cm_dealloc dealloc_entry) ;

/*驱动整个UDT服务器,进行相应的运算*/
NE_NET_API NEINT32 doneIO(ne_udtsrv *root, netime_t timeout) ;

//更新每个udt_socket的状态
//定时驱动每个连接
NE_NET_API void update_all_socket(ne_udtsrv *root) ;

//读取到达的网络数据
NE_NET_API NEINT32 read_udt_handler(ne_udtsrv *root, netime_t timeout) ;

NE_NET_API void init_udt_srv_node(ne_udtsrv *root) ;
#endif 
