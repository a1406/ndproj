#include "ne_common/ne_common.h"

#define DELBUF_SIZE 16
struct ne_timer_root
{
	NEUINT32 size ;						/*句柄的大小*/
	NEUINT32 type  ;					/*句柄类型*/	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			/*句柄释放函数*/
	neatomic_t	num ;
	neatomic_t start_id ;
	ne_handle pallocator ;					//memory allocator 
	ne_mutex	list_lock ;
	struct list_head list ;					//node list
	netimer_t	del_buf[DELBUF_SIZE] ;
} ;


struct timer_node 
{	
	netimer_t timer_id ;
	NEINT32 run_type ;
	void *param ;
	ne_timer_entry entry;
	netime_t last_time ;
	netime_t interval ;
	
	struct list_head list ;
};

static void tryto_del_timer(struct ne_timer_root *root) ;
/* 增加一个计时执行函数*/
netimer_t ne_timer_add(ne_handle handle,ne_timer_entry func,void *param,netime_t interval, NEINT32 run_type )
{
	struct ne_timer_root * root ;
	struct timer_node *node ;
	ne_assert(handle) ;
	root = (struct ne_timer_root*)handle ;
	ne_assert(root->pallocator) ;
	if(!func)
		return 0 ;

	node = ne_pool_alloc(root->pallocator, sizeof(*node)) ;
	if(!node) {
		return 0 ;
	}
	node->timer_id = ne_atomic_inc(&root->start_id) ;
	node->run_type = run_type ;
	node->param = param ;
	node->entry = func ;
	node->interval = interval ;
	node->last_time = ne_time() ;
	
	INIT_LIST_HEAD(&node->list) ;
	

	ne_mutex_lock(&root->list_lock) ;
		list_add_tail(&node->list, &root->list) ;
	ne_mutex_unlock(&root->list_lock) ;
	ne_atomic_inc(&root->num) ;
	return node->timer_id;
}

/* 删除定时器,外部使用函数
 * 把计时器放倒被删除队列中,在update时删除
 * 这样做可以在定时器函数中增加和删除定时器函数,
 * 因为避免了圈套luck
 */
NEINT32 ne_timer_del(ne_handle handle, netimer_t id) 
{
	NEINT32 i ,ret=-1;
	struct ne_timer_root *root ;

	if(!handle)
		return -1 ;
	if(0==id)
		return -1 ;
	root = (struct ne_timer_root *)handle ;
	
	for (i=0; i<DELBUF_SIZE; i++) {
		ret = ne_compare_swap(root->del_buf+i,0, id) ;
		if(0==ret)
			break ;
	}
	return (ret==0)? 0 : -1 ;

}

/* 销毁一个定时器对象*/
NEINT32  ne_timer_destroy(ne_handle timer_handle, NEINT32 force) 
{
	struct timer_node *node ;
	struct ne_timer_root *root ;
	struct list_head *pos , *next;
	if(!timer_handle) {
		return -1 ;
	}
	root =(struct ne_timer_root *) timer_handle ;
	if(!root->pallocator) {
		return -1 ;
	}
	//release all node 
	ne_mutex_lock(&root->list_lock) ;
		pos = root->list.next ;
		while(pos != &root->list) {
			node = list_entry(pos, struct timer_node, list) ;
			next = pos->next ;
			list_del(pos) ;
			ne_pool_free(root->pallocator,node) ;
			pos = next ;
		}
	ne_mutex_unlock(&root->list_lock) ;
	//release root node 
	ne_pool_free(root->pallocator,root) ;
	return 0 ;
}

/* create timer root */
ne_handle ne_timer_create(ne_handle pallocator) 
{
	static neatomic_t _s_id = 0 ;
	struct ne_timer_root *root ;
	if(!pallocator) {
		pallocator = ne_global_mmpool() ;

	}
	ne_assert(pallocator) ;

	root = ne_pool_alloc(pallocator, sizeof(*root)) ;
	if(!root) {
		return NULL ;
	}

	root->size = sizeof(*root) ;					/*句柄的大小*/
	root->type = 'T' << 8 | 'm' ;					/*句柄类型*/	
	root->myerrno = 0 ;
	root->close_entry =(ne_close_callback )ne_timer_destroy;			/*句柄释放函数*/
	root->pallocator = pallocator;					//memory allocator 
	root->start_id = 0 ;
	root->num = 0 ;
	ne_mutex_init(&root->list_lock) ;
	INIT_LIST_HEAD(&root->list) ;					//node list
	bzero(root->del_buf, sizeof(root->del_buf)) ;
	return (ne_handle) root ;
}

/* 执行定时器函数*/
NEINT32  ne_timer_update(ne_handle handle) 
{
	NEINT32 ret = 0 ,run_ret =0 ;
	netime_t now_tm ;
	struct list_head *pos,*next, header ;
	struct timer_node *node ;
	struct ne_timer_root *root = (struct ne_timer_root *)handle ;
	
	if(ne_atomic_read(&root->num) < 1 ) {
		return 0 ;
	}
	tryto_del_timer(root) ;

	INIT_LIST_HEAD(&header) ;

	ne_mutex_lock(&root->list_lock) ;
    list_add(&header, &(root->list)) ;
    list_del_init(&root->list) ;
	ne_mutex_unlock(&root->list_lock) ;

	pos = header.next;	
	while(pos != &header) {
		next = pos->next ;
		node = list_entry(pos, struct timer_node, list) ;
		now_tm = ne_time() ;
		if((now_tm - node->last_time) >= node->interval) {
			run_ret = node->entry(node->timer_id,node->param) ;
			if(node->run_type == ETT_ONCE || -1 == run_ret) {
				list_del(pos) ;
				ne_pool_free(root->pallocator,node) ;
			}
			else
				node->last_time = now_tm ;
		}
		pos = next ;
	}

	ne_mutex_lock(&root->list_lock) ;
		list_join(&header, &root->list) ;
	ne_mutex_unlock(&root->list_lock) ;
	
	ne_assert(header.next==header.prev) ;
	return ret ;
}

static void _del_from_list(struct ne_timer_root *root, netimer_t id) 
{
	struct list_head *pos ;
	struct timer_node *node ;
	ne_assert(root) ;

	ne_mutex_lock(&root->list_lock) ;
		list_for_each(pos, &root->list){
			node = list_entry(pos, struct timer_node, list) ;
			if(node->timer_id == id) {
				list_del(pos) ;
				break ;
			}
			else 
				node = 0 ;
		}
	ne_mutex_unlock(&root->list_lock) ;

	if(node) {
		ne_atomic_dec(&root->num) ;
		ne_pool_free(root->pallocator,node) ;
	}

}

void tryto_del_timer(struct ne_timer_root *root)
{
	NEINT32 i ;
	netimer_t id ;

	for (i=0; i<DELBUF_SIZE; i++) {
		id = ne_atomic_swap(root->del_buf+i,0) ;
		if(id) {
			_del_from_list(root, id) ;
		}
	}
}
