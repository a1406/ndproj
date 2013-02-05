#define NE_IMPLEMENT_HANDLE
typedef struct clientmap_allocator *ne_handle ;

#include "ne_srvcore/ne_srvlib.h"
#include "ne_common/ne_alloc.h"
//#include "ne_net/ne_tcp.h"
//#include "ne_net/ne_udt.h"
//#include "ne_srvcore/ne_listensrv.h"
//#include "ne_srvcore/ne_udtsrv.h"
//#include "ne_net/ne_netui.h"
#ifdef WIN32
#include "../win_iocp/ne_iocp.h"
#endif 


/*
 * client map 分配器
 */
typedef struct clientmap_allocator
{
	//client map 分配器,前面3个成员和handle兼容
	NEUINT32 size ;						/*句柄的大小*/
	NEUINT32 type ;					/*句柄类型*/	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			/*句柄释放函数*/

	NEINT32 client_num ;		//numbers of client map
	
	NEINT32 free_num ;			// nubers of free client map

	size_t client_size ;	//affix_datalen + sizeof ne_client_map
	
	ne_mutex  list_lock;
	struct list_head __free_list;

	NEINT8 __client_addr[0] ;

}ne_cm_allocator_t;

NEINT32 ne_cm_allocator_freenum(ne_cm_allocator_t *a)
{
	return a->free_num ;
}

NEINT32 ne_cm_allocator_capacity(ne_cm_allocator_t *a) 
{
	return a->client_num ;
}

ne_cm_allocator_t *ne_cm_allocator_create(NEINT32 client_num, size_t client_size) 
{
	NEINT32 i ;
	size_t raw_len;
	NEINT8 *addr ;
	ne_cm_allocator_t *allocator ;
	if(client_num<=0 || client_num>= MAX_CLIENTS || client_size>=MAX_AFFIX_DATALEN) 
		return 0;
	
	raw_len = client_num * client_size + sizeof(ne_cm_allocator_t);

	allocator = (ne_cm_allocator_t *)malloc(raw_len ) ;

	if(!allocator) {
		return NULL;
	}

	allocator->size = raw_len;						/*句柄的大小*/
	allocator->type = 'a'<< 8 | 'l';					/*句柄类型*/
	allocator->close_entry =(ne_close_callback) ne_cm_allocator_destroy ;			/*句柄释放函数*/
	allocator->myerrno = NEERR_SUCCESS ;
	allocator->client_num = client_num;		//numbers of client map
	allocator->free_num = client_num;	
	allocator->client_size = client_size ;	//affix_datalen + sizeof ne_client_map
	
	ne_mutex_init(&allocator->list_lock);
	INIT_LIST_HEAD(&allocator-> __free_list);

	addr = allocator->__client_addr ;

	for (i = 0; i<client_num; i++){
		//ne_climap_handle clihandle = (ne_climap_handle)addr ;
		struct list_head *list = (struct list_head *)addr ;
		
		INIT_LIST_HEAD(list);
		list_add(list,&(allocator->__free_list)) ;
		addr += client_size ;
	}
	return allocator ;
}

NEINT32 ne_cm_allocator_destroy(ne_cm_allocator_t *allocator, NEINT32 flag)
{
	if(!allocator) {
		return -1;
	}

	ne_assert(allocator->size ==(allocator->client_num * allocator->client_size + sizeof(ne_cm_allocator_t))) ;
	ne_mutex_destroy(&allocator->list_lock);
	free(allocator) ;
	
	return 0 ;
}

void* ne_cm_node_alloc(ne_cm_allocator_t *allocator) 
{
	struct list_head *pos;
	allocator->myerrno = NEERR_SUCCESS;
	ne_mutex_lock(&allocator->list_lock);
	pos = allocator->__free_list.next ;

	if(pos== &allocator->__free_list){
		allocator->myerrno = NEERR_LIMITED ;
		ne_mutex_unlock(&allocator->list_lock);
		return NULL ;
	}

	list_del(pos) ;
	--(allocator->free_num );	
	ne_mutex_unlock(&allocator->list_lock);

	return (ne_climap_handle)pos;
}

void ne_cm_node_free(void* cli_handle,ne_cm_allocator_t *allocator ) 
{
	struct list_head *head ;
	if(!cli_handle || !allocator) {
		return ;
	}

	head = (struct list_head *) cli_handle ;

	INIT_LIST_HEAD(head) ;

	ne_mutex_lock(&allocator->list_lock);
		list_add_tail(head,&(allocator->__free_list)) ;
		++(allocator->free_num );
	ne_mutex_unlock(&allocator->list_lock);
}
#undef  NE_IMPLEMENT_HANDLE
