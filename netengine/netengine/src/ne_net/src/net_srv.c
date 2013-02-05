#include "ne_net/ne_netlib.h"
#include "ne_net/ne_tcp.h"
#include "ne_net/ne_srv.h"
#include "ne_common/ne_alloc.h"
#include "ne_common/ne_atomic.h"

/*
 * 本节实现了对server端连接的管理,每个连接在server上称为session(绘话)
 * 每个绘话对于一个数据结构.
 * 每个绘话是全局变量,需要从绘话或者是连接管理池中分配一个给已经进入的连接.
 * 多个线程可以对每个绘话进行访问,为了避免线程对绘话数据造成同步错误,
 * 对每个绘话分配一个连接管理器: struct cm_node  .
 * 如果需要对绘话进行访问,需要首先找到连接管理节器对于的struct cm_node结构,然后用cm_node中的锁锁住绘话句柄便可以访问.
 * 同时连接管理节点自身也需要进行多线程互斥,主要避免的是多线程同时竞争一个节点,或者正在使用中的节点被释放.
 * 但是在对节点和session进行线程互斥的时候要避免圈套锁和死锁,
 * 因此在对节点进行互斥的时候没有使用lock而使用了引用计数(原子操作).
 */
//
static void *ne_cm_alloc(ne_handle alloctor) ;
static void ne_cm_dealloc(void *socket_node,ne_handle alloctor) ;
static void ne_cm_init(void *socket_node, ne_handle h) ;
static NEUINT16 ne_cm_accept(struct cm_manager *root, void *socket_node);
static NEINT32 ne_cm_deaccept(struct cm_manager *root, NEUINT16 session_id);
static void *ne_cm_lock(struct cm_manager *root, NEUINT16 session_id);
static void *ne_cm_trylock(struct cm_manager *root, NEUINT16 session_id);
static void ne_cm_unlock(struct cm_manager *root, NEUINT16 session_id);
static void ne_cm_walk_node(struct cm_manager *root,cm_walk_callback cb_entry, void *param);
static void *ne_cm_search(struct cm_manager *root, NEUINT16 session_id);
static void* ne_cm_lock_first(struct cm_manager *root,cmlist_iterator_t *it);
static void* ne_cm_lock_next(struct cm_manager *root,cmlist_iterator_t *it) ;
static void ne_cm_unlock_iterator(struct cm_manager *root, cmlist_iterator_t *it) ;
//static NEINT32 ne_cm_inc_ref(struct cm_manager *root, NEUINT16 session_id);
//static void ne_cm_dec_ref(struct cm_manager *root, NEUINT16 session_id);

NEINT32 ne_tcpsrv_open(NEINT32 port,NEINT32 listen_nums, struct ne_srv_node *node)
{
	nesocket_t fd;
	ne_assert(node) ;
	node->myerrno = NEERR_SUCCESS ;
	fd = ne_socket_openport(port,SOCK_STREAM,NULL,listen_nums) ;
	if(-1==fd){
		ne_logfatal("open port %s" AND ne_last_error()) ;
		node->myerrno = NEERR_OPENFILE ;
		return -1 ;
	}
//	_set_socket_addribute(fd) ;
	
	node->fd = fd ;
	node->port = port ;
	node->status = 1 ;
	
	return 0 ;
}

void ne_tcpsrv_close(struct ne_srv_node *node)
{
	ne_assert(node) ;
	ne_socket_close(node->fd) ;
	node->status = 0 ;
	node->fd = 0 ;
}

void ne_tcpsrv_node_init(struct ne_srv_node *node)
{
	bzero(node,sizeof(*node)) ;
	node->size = sizeof(*node);
	node->type = TCP_SERVER_TYPE ;
	//INIT_LIST_HEAD(nod)
}

NEINT32 cm_listen(struct cm_manager *root, NEINT32 max_num) 
{
	NEINT32 i;
	struct cm_node  *conn_node;
	
	root->max_conn_num = max_num ;
	ne_atomic_set(&(root->connect_num),0) ;
	
	root->connmgr_addr = (struct cm_node *)malloc(max_num*sizeof(struct cm_node)) ;
	if(!root->connmgr_addr) {
		ne_logerror("malloc error!\n") ;
		return -1 ;
	}

	conn_node = root->connmgr_addr;
	for (i=0; i<root->max_conn_num; i++,conn_node++){
		ne_mutex_init(&(conn_node->lock)) ;
		conn_node->client_map = NULL ;
		conn_node->is_mask = 0 ;
		ne_atomic_set(&(conn_node->__used),0);
		//conn_node->used = 0;
	}
	
#define SET_FUNC(a,name) if((a)-> name==0)(a)-> name = ne_cm_##name
	SET_FUNC(root,alloc) ;
	SET_FUNC(root,init) ;
	SET_FUNC(root,dealloc) ;
	SET_FUNC(root,accept) ;
	SET_FUNC(root,deaccept) ;
	SET_FUNC(root,lock) ;
	SET_FUNC(root,trylock) ;
	SET_FUNC(root,unlock) ;
	SET_FUNC(root,walk_node) ;
	SET_FUNC(root,search) ;
	SET_FUNC(root,lock_first) ;
	SET_FUNC(root,lock_next) ;
	SET_FUNC(root,unlock_iterator) ;
//	SET_FUNC(root,inc_ref) ;
//	SET_FUNC(root,dec_ref) ;

#undef SET_FUNC
	return 0 ;
}

void cm_destroy(struct cm_manager *root)
{
	NEINT32 i;
	struct cm_node  *conn_node;
	if(!root->connmgr_addr) {
		ne_logerror("input error in cm_destroy!\n") ;
		ne_assert(0) ;
		return ;
	}

	conn_node = root->connmgr_addr;
	for (i=0; i<root->max_conn_num; i++,conn_node++){
		ne_mutex_destroy(&(conn_node->lock)) ;
	}

	free(root->connmgr_addr) ;
	root->connmgr_addr = 0 ;
}

#define GET_SESSION_ID(session_id) (((session_id) & (~SESSION_ID_MASK)) - START_SESSION_ID)

//
NEUINT16 ne_cm_accept(struct cm_manager *root, void *socket_node)
{	
	NEINT32 i ;
	struct cm_node *node;
	
	if(!root)
		return 0 ;
	node = root->connmgr_addr;
	
	for (i=0; i<root->max_conn_num; i++, node++) {
		ne_mutex_lock(&node->lock) ;
		if (ne_atomic_read(&node->__used) == 0) {
			ne_atomic_set(&node->__used, 2);
			
			node->client_map = socket_node ;
			ne_atomic_inc(&(root->connect_num)) ;
			if(node->is_mask)
				return (i+ START_SESSION_ID) | SESSION_ID_MASK ;
			else
				return i+ START_SESSION_ID;
		}
		ne_mutex_unlock(&node->lock);
	}
	return 0 ;
}

NEINT32 ne_cm_deaccept(struct cm_manager *root, NEUINT16 session_id)
{
	NEINT32 index = GET_SESSION_ID(session_id) ;
	struct cm_node *node ;
	void *p ;

	if(index<0 || index >= root->max_conn_num)
		return -1;

	node = root->connmgr_addr  ;
	node += index ;
	ne_assert(node->__used>1) ;

	p = node->client_map ;
	if(ne_compare_swap(&node->__used,2,0)) {		//这个时候used是2,因为在deaccept的时候是锁住的
//		ne_mutex_lock(&node->lock) ;				//这里不能锁，因为已经锁住了
			if(p==node->client_map) 
				node->client_map = NULL ;
//		ne_mutex_unlock(&node->lock) ;
		node->is_mask = !(node->is_mask) ;
		ne_atomic_dec(&(root->connect_num)) ;
		return 0 ;
	}
	return -1 ;
}

void *ne_cm_lock(struct cm_manager *root, NEUINT16 session_id)
{
	neatomic_t tmp ;
	NEINT32 index = GET_SESSION_ID(session_id) ;
	struct cm_node *connmgr_node ;
	
	if(index<0 || index >= root->max_conn_num)
		return NULL;

	connmgr_node = root->connmgr_addr  ;
	connmgr_node += index ;

	ne_mutex_lock(&connmgr_node->lock);	
	if (ne_atomic_read(&connmgr_node->__used) <= 0)
		goto fail;

	if(!connmgr_node->client_map)
		goto fail;

	return connmgr_node->client_map ;
fail:	
	ne_mutex_unlock(&connmgr_node->lock);	
	return NULL;
}

void *ne_cm_trylock(struct cm_manager *root, NEUINT16 session_id)
{
	neatomic_t tmp ;
	NEINT32 index = GET_SESSION_ID(session_id) ;
	struct cm_node *connmgr_node ;

	if(index<0 || index >= root->max_conn_num)
		return NULL;

	connmgr_node = root->connmgr_addr  ;
	connmgr_node += index ;
	if(0 != ne_mutex_trylock(&connmgr_node->lock) )
		return NULL;

	if (ne_atomic_read(&(connmgr_node->__used)) <= 0)
		goto fail;

	if(!connmgr_node->client_map)
		goto fail;

	return connmgr_node->client_map ;
fail:	
	ne_mutex_unlock(&connmgr_node->lock);	
	return NULL;
}

void ne_cm_unlock(struct cm_manager *root, NEUINT16 session_id)
{
	NEINT32 index = GET_SESSION_ID(session_id) ;
	struct cm_node *connmgr_node ;

	if(index<0 || index >= root->max_conn_num)
		return ;

	connmgr_node = root->connmgr_addr  ;
	connmgr_node += index ;

	ne_mutex_unlock(&connmgr_node->lock) ;
}

void ne_cm_walk_node(struct cm_manager *root,cm_walk_callback cb_entry, void *param)
{
	neatomic_t v = 0 ;
	NEINT32 i;
	struct cm_node *node = root->connmgr_addr ;
	NEINT32 num = ne_atomic_read(&root->connect_num);

	for (i=0; i<root->max_conn_num && num>0; i++,node++){
		ne_mutex_lock(&node->lock);
		if (ne_atomic_read(&node->__used) > 0) {
			if(node->client_map ) 
				cb_entry(node->client_map, param);
			--num;
		}
		ne_mutex_unlock(&node->lock);
	}
}

void *ne_cm_search(struct cm_manager *root, NEUINT16 session_id)
{

	NEINT32 index = GET_SESSION_ID(session_id) ;
	struct cm_node *connmgr_node ;

	if(index<0 || index >= root->max_conn_num)
		return NULL;

	connmgr_node = root->connmgr_addr  ;
	connmgr_node += index ;

	if(0==ne_atomic_read(&connmgr_node->__used))
		return NULL;
	return connmgr_node->client_map ;
}

NEINT32 ne_srv_capacity(struct ne_srv_node *srvnode) 
{
	return srvnode->conn_manager.max_conn_num ;
}

void *ne_cm_alloc(ne_handle alloctor) 
{
	ne_msgbox("please set alloc function of client manager", "error" ) ;
	ne_logerror("please set alloc function of client manager") ;
	return NULL ;
}

void ne_cm_dealloc(void *socket_node,ne_handle alloctor) 
{
	ne_msgbox("please set free function of client manager", "error" ) ;
	ne_logerror("please set free function of client manager") ;
	return ;
}

void ne_cm_init(void *socket_node, ne_handle h)
{
	ne_msgbox("please set initialization function of client manager", "error" ) ;
	ne_logerror("please set initialization function of client manager") ;
	return ;
}

void* ne_cm_lock_first(struct cm_manager *root,cmlist_iterator_t *it)
{
	neatomic_t v = 0 ;
	NEINT32 i ;
	struct cm_node *node;
	
	node = root->connmgr_addr  ;
	it->session_id = 0 ;it->numbers = 0 ;
	for (i=0; i<root->max_conn_num; i++, node++) {
		ne_mutex_lock(&node->lock);
		if (ne_atomic_read(&node->__used) <= 0) {
			ne_mutex_unlock(&node->lock);
			continue;
		}
		if(!node->client_map ) {
			ne_mutex_unlock(&node->lock);
			continue;
		}
		it->session_id = i+START_SESSION_ID;
		++(it->numbers) ; 
		return node->client_map;
	}
	it->session_id = 0 ;
	return NULL ;
}

void ne_cm_unlock_iterator(struct cm_manager *root, cmlist_iterator_t *it) 
{
	NEINT32 i = GET_SESSION_ID(it->session_id)  ;
	struct cm_node *node;
	
	ne_assert(i>=0 && i<root->max_conn_num) ;

	node = root->connmgr_addr + i  ;

	ne_cm_unlock(root, it->session_id) ;

	it->session_id = 0 ;
}
void* ne_cm_lock_next(struct cm_manager *root,cmlist_iterator_t *it)
{
	neatomic_t v = 0 ;
	NEINT32 i = GET_SESSION_ID(it->session_id);
	struct cm_node *node;
	
	ne_assert(i>=0 && i<root->max_conn_num) ;

	node = root->connmgr_addr + i  ;
	
	ne_cm_unlock(root, it->session_id) ;

	for ( ++i, ++node; i<root->max_conn_num && it->numbers < ne_atomic_read(&root->connect_num); i++, node++) {
		ne_mutex_lock(&node->lock);
		if (ne_atomic_read(&node->__used) <= 0) {
			ne_mutex_unlock(&node->lock);
			continue;
		}
		if(!node->client_map ) {
			ne_mutex_unlock(&node->lock);
			continue;
		}
		it->session_id = i+START_SESSION_ID;
		++(it->numbers) ;
		return node->client_map;
	}
	it->session_id = 0 ;
	return NULL ;
}

#undef  GET_SESSION_ID
