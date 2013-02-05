/*
 * 这是一个特殊的mempool 
 * 只提供了对特定长度和个数的内存缓冲池.
 * 在需要使用多个同样大小的结构或者内存块的是,可以申请一个内存池,
 * 然后每次使用的时候从这个内存池中申请一个内存块出来,不用了就可以释放给内存池.
 */
#ifndef _NE_MEMPOOL_H_
#define _NE_MEMPOOL_H_

#define ALIGN_SIZE			8
#define ROUNE_SIZE(s) 		((s)+7) & (~7)

#define MIN_ALLOC_SIZE		(ALIGN_SIZE *2)

//内存池类型
enum emem_pool_type{
	EMEMPOOL_TINY = 32*1024,		//微型内存池
	EMEMPOOL_NORMAL = 256*1024,		//普通大小内存池
	EMEMPOOL_HUGE = 1024*1024,		//巨型内存池
	EMEMPOOL_UNLIMIT = -1			//无限制内存池
};

//内存池模块初始化/释放函数
NE_COMMON_API NEINT32 ne_mempool_root_init() ;
NE_COMMON_API void ne_mempool_root_release();

//内存池操作函数
NE_COMMON_API ne_handle ne_pool_create(size_t size ) ;					//创建一个内存池,返回内存池地址
NE_COMMON_API NEINT32 ne_pool_destroy(ne_handle pool, NEINT32 flag);		//销毁一个内存缓冲池
NE_COMMON_API void *ne_pool_alloc(ne_handle pool , size_t size);	//从缓冲池中申请一个内存
NE_COMMON_API void ne_pool_free(ne_handle pool ,void *addr) ;		//释放一个内存
NE_COMMON_API void ne_pool_reset(ne_handle pool) ;					//reset a memory pool
//分配指定长度内存块,需要用指定长度的释放函数
//当使用 *_alloc_l函数申请内存时,必须使用*_free_l函数释放,否则发生不可预期的错误
//增加*_l系列函数的目的是提高内存的使用效率,比不带_l的系列节省4个字节
NE_COMMON_API void *ne_pool_alloc_l(ne_handle pool , size_t size) ;
NE_COMMON_API void ne_pool_free_l(ne_handle pool ,void *addr, size_t size) ;

NE_COMMON_API size_t ne_pool_freespace(ne_handle pool) ;	//get free space 

/*得到全局默认的内存池*/
NE_COMMON_API ne_handle ne_global_mmpool() ;

#ifdef NE_DEBUG
typedef void *(*ne_alloc_func)(size_t __s) ;		//定义内存申请函数指针
typedef void (*ne_free_func)(void *__p) ;			//定义内存释放函数指针

//检测allocfn所分配的内存是否越界
NE_COMMON_API void *ne_alloc_check(size_t __n,ne_alloc_func allocfn)  ;
NE_COMMON_API void ne_free_check(void *__p, ne_free_func freefn)  ;

#endif 

//全局分配函数
NE_COMMON_API void *ne_global_alloc(size_t size) ;
NE_COMMON_API void ne_global_free(void *p) ;

//申请指定长度的内存,在内存块的起始处不保存内存块长度
//NE_COMMON_API void *ne_global_alloc_l(size_t size) ;
//NE_COMMON_API void ne_global_free_l(void *p, size_t size)  ;


#endif
