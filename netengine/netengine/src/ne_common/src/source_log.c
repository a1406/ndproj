#include "ne_common/ne_os.h"
#include "ne_common/list.h"
#include "ne_common/ne_common.h"

#include <stdio.h>

#ifdef _NE_MEMORY_H_
//这里需要使用libc的malloc函数
#error do not include ne_alloc.h
#endif

#ifdef NE_DEBUG

#ifdef NE_MULTI_THREADED
//使用多线程
static ne_mutex __S_source_lock ;
static NEINT32 __s_source_init = 0 ;

#define __LOCK() do{ \
	ne_assert(__s_source_init) ;\
	ne_mutex_lock(&__S_source_lock) ; \
}while(0)
#define __UNLOCK() ne_mutex_unlock(&__S_source_lock) 
#else 
#define __LOCK() (void)0
#define __UNLOCK() (void)0
#endif

static LIST_HEAD(__s_source_head) ;
static NEINT32 _source_numbers = 0 ;
#define SOURCE_DUMP(msg) ne_logfatal(msg)
//fprintf(stderr,msg) 
//定义资源记录器的数据结果
struct _Source_loginfo{
	void *__source ;
	NEINT32 __line ;			//line munber in file
	NEINT8 __file[256] ;
	NEINT8 __operate[32] ;
	NEINT8 __msg[128] ;
	struct list_head __list ;
};

NEINT32 _source_log(void *p , NEINT8 *operate, NEINT8 *msg, NEINT8 *file, NEINT32 line)
{
	if(!p) {
		return -1 ;
	}
	else {
		struct _Source_loginfo *node ;
		node = (struct _Source_loginfo *)
			malloc(sizeof(struct _Source_loginfo ) ) ;
		if(!node) {
			return -1 ;
		}
		node->__source = p ;
		node->__line = line ;
		//node->__file = file ;
		//node->__operate = operate;
		//node->__msg = msg ;
		if(operate)
			strncpy(node->__operate,operate,32) ;
		if(msg) 
			strncpy(node->__msg,msg,128) ;
		if(file) 
			strncpy(node->__file,file,256) ;
		
		__LOCK() ;
			INIT_LIST_HEAD(&(node->__list)) ;
			list_add(&(node->__list),&__s_source_head) ;
			++_source_numbers ;
		__UNLOCK() ;
		return 0 ;
	}
}
static void _destroy_source_node(struct _Source_loginfo *node)
{
	ne_assert(node) ;
	list_del_init(&(node->__list) );
	--_source_numbers ;
	free(node) ;	
}

static void _dump_source(struct _Source_loginfo *node)
{
	NEINT8 buf[1024] = {0};
	ne_assert(node) ;
	snprintf(buf,1024, "ERROR %s operate [%s] in file %s line %d\n",
		node->__msg,node->__operate,node->__file,node->__line) ;
	
	SOURCE_DUMP(buf) ;
	list_del(&(node->__list) );
	free(node) ;
}
NEINT32 _source_release(void *source)
{
	NEINT32 ret = -1 ;
	if(!source) {
		ne_assert(source) ;
		return  -1;
	}
	else {
		struct list_head *pos;
		struct _Source_loginfo *node = NULL ;
		__LOCK() ;
		pos = __s_source_head.next ;
#if 1
		list_for_each(pos, &__s_source_head) {
			node = list_entry(pos, struct _Source_loginfo, __list) ;
			if(source == node->__source){
				_destroy_source_node(node) ;
				ret = 0 ;
				break ;
			}
		}
#else
		while(pos!= &__s_source_head) {
			node = list_entry(pos, struct _Source_loginfo, __list) ;
			pos = pos->next ;
			if(source == node->__source){
				_destroy_source_node(node) ;
				ret = 0 ;
				break ;
			}
		}
#endif
		__UNLOCK() ;
	}
	return ret ;
}
NEINT32 ne_sourcelog_init()
{

#ifdef NE_MULTI_THREADED
	if(__s_source_init==0){
		if(-1==ne_mutex_init(&__S_source_lock)  ) {
			ne_logfatal("source log initilized failed!\n") ;
			return -1 ;
		}
	}
	__s_source_init = 1 ;
#endif
	return 0;
}
void ne_sourcelog_dump()
{
	struct list_head *pos =__s_source_head.next ;
	struct _Source_loginfo *node ;
	while(pos!=&__s_source_head) {
		node = list_entry(pos, struct _Source_loginfo, __list) ;
		pos = pos->next ;
		_dump_source(node) ;
	}

#ifdef NE_MULTI_THREADED
	ne_mutex_destroy(&__S_source_lock)   ;
	__s_source_init = 0 ;
#endif
}

#undef  __LOCK
#undef __UNLOCK
#endif		//NE_DEBUG
