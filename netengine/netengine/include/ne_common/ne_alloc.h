#ifndef _NE_MEMORY_H_
#define _NE_MEMORY_H_

#include "ne_common/ne_common.h"
#include "ne_common/ne_mempool.h"

#define USER_NE_MALLOC		1

#ifdef USER_NE_MALLOC

#ifdef NE_IMPLETE_MEMPOOL
#error can not include this file in current c-file
#endif 
#undef free 
#undef malloc

#ifdef NE_DEBUG
#include "ne_common/ne_dbg.h"

static __INLINE__ void *ne_malloc(size_t size, NEINT8 *file, NEINT32 line)
{
	void *p = ne_alloc_check(size,ne_global_alloc )  ;
	if(p) {
		_source_log(p,(NEINT8 *)"malloc",(NEINT8 *)"memory leak!", file,line) ;
	}
	return p ;
}

static __INLINE__ void ne_free(void *p)
{
	if(p) {
		NEINT32 ret =_source_release(p) ;
		ne_assert(0==ret) ;
		ne_free_check(p,ne_global_free ) ;
	}
}

#define malloc(__n)		ne_malloc(__n,__FILE__, __LINE__) 
#define free(__p)		ne_free(__p)
//#define malloc_l(__n)	ne_global_alloc_l(__n) 
//#define free_l(__p,__n) ne_global_free_l(__p,__n)

#else 
#define malloc(__n)		ne_global_alloc(__n) 
#define free(__p)		ne_global_free(__p)
//#define malloc_l(__n)	ne_global_alloc_l(__n) 
//#define free_l(__p,__n) ne_global_free_l(__p,__n)
#endif			//debug

#else 
#endif	//	USER_NE_MALLOC

#endif
