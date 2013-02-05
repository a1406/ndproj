#if 0
#include "ne_net/ne_netlib.h"
#include "ne_net/ne_udt.h"
#include "ne_net/ne_udtsrv.h"
#include "ne_common/ne_common.h"
#include "ne_common/ne_alloc.h"
#include <stdlib.h>

/* 管理已经连接上的udt接点.
 * 使用一个struct udt_connmgr_node 数组保存所有已经连接的udt_socket.
 * 以udt的session_id作为数组下标存放udt_socket的结构地址.
 * 申请session_id时,通过查找udt_connmgr_node数组中未使用的接点,并返回其下标.
 * 因为连接的接点不会很多,所以可以使用空间换时间
 */

//申请内存
NEINT32 udt_connmgr_create(ne_udtsrv *root, NEINT32 max_conn)
{
	NEINT32 i ;
	struct udt_connmgr_node *conn_node;
	ne_assert(max_conn>0 && max_conn<=UDT_MAX_CONNECT) ;
	root->max_conn_num = max_conn ;
	
	ne_atomic_set(&(root->connect_num),0) ;

	max_conn *= sizeof(struct udt_connmgr_node)  ;
	root->connmgr_addr = malloc(max_conn) ;	
	if(!root->connmgr_addr)
		return -1 ;
	
	conn_node =(struct udt_connmgr_node *) root->connmgr_addr;
	for (i=0; i<root->max_conn_num; i++,conn_node++){
		ne_mutex_init(&(conn_node->lock)) ;
		conn_node->udt_socket = NULL ;
		ne_atomic_set(&(conn_node->__used),0);
		//conn_node->used = 0;
	}
	//memset(root->connmgr_addr,0,max_conn);
	
	return 0 ;
}

//release connect manager 
void udt_connmgr_release(ne_udtsrv *root)
{
	NEINT32 i ;
	size_t s ;
	struct udt_connmgr_node *conn_node;
	ne_assert(root->connmgr_addr && root->max_conn_num) ;
		
	conn_node =(struct udt_connmgr_node *) root->connmgr_addr;
	for (i=0; i<root->max_conn_num; i++,conn_node++){
		ne_mutex_destroy(&(conn_node->lock)) ;
		conn_node->udt_socket = NULL ;
	}

	s = root->max_conn_num * sizeof(struct udt_connmgr_node)  ;
	free(root->connmgr_addr) ;

	root->max_conn_num = 0 ;
	ne_atomic_set(&(root->connect_num),0);
	root->connmgr_addr = NULL ;
}

//add connection to connmgr and alloc a session id 
u_16 connmgr_accept(ne_udtsrv *root , ne_udt_node *udt_socket)
{
	NEINT32 i ;
	struct udt_connmgr_node *node;
	
	if(!root)
		return 0 ;
	node =(struct udt_connmgr_node *) root->connmgr_addr ;
	
	for (i=0; i<root->max_conn_num; i++, node++) {
		if(0==ne_testandset(&node->__used)) {
			node->udt_socket = udt_socket ;
			ne_atomic_inc(&(root->connect_num)) ;
			return i+ UDT_START_SESSION_ID ;
		}
		/*if(0==ne_mutex_trylock(&node->lock) ) {
			if(0==node->used){
				node->used = 1 ;
				node->udt_socket = udt_socket ;
				ne_mutex_unlock(&node->lock) ;
				ne_sourcelog(index, "connmgr_accept", "alloc a udt port") ;
				return i+UDT_START_SESSION_ID ;
			}
			ne_mutex_unlock(&node->lock) ;			
		}
		*/
	}
	return 0 ;

}

//deaccept ne_udt_node 
void connmgr_deaccept(ne_udtsrv *root , ne_udt_node *udt_socket)
{
	NEINT32 index = udt_socket->session_id - UDT_START_SESSION_ID ;
	struct udt_connmgr_node *node ;

	if(index<0 || index >= root->max_conn_num)
		return ;

	node = (struct udt_connmgr_node *)root->connmgr_addr  ;
	node += index ;

	if(0==ne_atomic_read(&(node->__used))){
		ne_assert(0) ;
		return ;
	}
	ne_mutex_lock(&node->lock) ;	
		node->udt_socket = NULL ;
	ne_mutex_unlock(&node->lock) ;
	
	ne_atomic_set(&node->__used,0) ;
//	ne_source_release((void*)index) ;
	ne_atomic_dec(&(root->connect_num)) ;

}

//search udt_socnet_node from connmgr
ne_udt_node *connmgr_lock(ne_udtsrv *root, u_16 session_id)
{
	NEINT32 index = session_id - UDT_START_SESSION_ID ;
	struct udt_connmgr_node *connmgr_node ;
	
	if(index<0 || index >= root->max_conn_num)
		return NULL;

	connmgr_node = (struct udt_connmgr_node *)root->connmgr_addr  ;
	connmgr_node += index ;

	ne_mutex_lock(&connmgr_node->lock);

	if(0 == ne_atomic_read(&(connmgr_node->__used)) )
		goto fail;

	if(connmgr_node->udt_socket)
		return connmgr_node->udt_socket ;
fail:	
	ne_mutex_unlock(&connmgr_node->lock);
	return NULL;
}


//search udt_socnet_node from connmgr
ne_udt_node *connmgr_trylock(ne_udtsrv *root, u_16 session_id)
{
	NEINT32 index = session_id - UDT_START_SESSION_ID ;
	struct udt_connmgr_node *connmgr_node ;

	if(index<0 || index >= root->max_conn_num)
		return NULL;

	connmgr_node = (struct udt_connmgr_node *)root->connmgr_addr  ;
	connmgr_node += index ;

	if(0==ne_mutex_trylock(&connmgr_node->lock))
		return NULL;

	if(0==ne_atomic_read(&(connmgr_node->__used)))  {
		goto fail;
	}

	if(connmgr_node->udt_socket)
		return connmgr_node->udt_socket ;
fail:
	ne_mutex_unlock(&connmgr_node->lock);	
	return NULL;
}

void connmgr_unlock(ne_udtsrv *root, u_16 session_id)
{
	NEINT32 index = session_id - UDT_START_SESSION_ID ;
	struct udt_connmgr_node *connmgr_node ;

	if(index<0 || index >= root->max_conn_num)
		return ;

	connmgr_node = (struct udt_connmgr_node *)root->connmgr_addr  ;
	connmgr_node += index ;

	ne_mutex_unlock(&connmgr_node->lock) ;
}


ne_udt_node *connmgr_search(ne_udtsrv *root, u_16 session_id)
{
	NEINT32 index = session_id - UDT_START_SESSION_ID ;
	struct udt_connmgr_node *connmgr_node ;

	if(index<0 || index >= root->max_conn_num)
		return NULL;

	connmgr_node = (struct udt_connmgr_node *)root->connmgr_addr  ;
	connmgr_node += index ;

	if(0==ne_atomic_read(&(connmgr_node->__used))) 
		return NULL;
	return connmgr_node->udt_socket ;

}

void connmgr_walk_node(ne_udtsrv *root,walk_callback cb_entry)
{
	NEINT32 i;
	struct udt_connmgr_node *node =(struct udt_connmgr_node *) root->connmgr_addr ;
	NEINT32 num = ne_atomic_read(&root->connect_num);

	for (i=0; i<root->max_conn_num && num>0; i++,node++){
		ne_mutex_lock(&node->lock) ;		
		if(ne_atomic_read(&(node->__used))){
			if(node->udt_socket )
				cb_entry(node->udt_socket);
			--num;
		}
		ne_mutex_unlock(&node->lock);
	}
}

//遍历所以已经建立的有效的udt连接
void connmgr_walk_valid_udt(ne_udtsrv *root,walk_callback cb_entry)
{
	NEINT32 i;
	struct udt_connmgr_node *node =(struct udt_connmgr_node *) root->connmgr_addr ;
	NEINT32 num = ne_atomic_read(&root->connect_num);

	for (i=0; i<root->max_conn_num && num>0; i++,node++){
		ne_mutex_lock(&node->lock) ;
		if(ne_atomic_read(&(node->__used))){
			if(node->udt_socket && NETSTAT_ESTABLISHED==node->udt_socket->conn_state)
				cb_entry(node->udt_socket);
			--num ;
		}
		ne_mutex_unlock(&node->lock);
	}
}

#endif
