#define NE_IMPLETE_MEMPOOL	1		//ʵ���ڴ�ص��ļ�
#define NE_IMPLEMENT_HANDLE 1

#include "ne_common/ne_os.h"
#include "ne_common/list.h"
#include "ne_common/ne_dbg.h"
#include "ne_common/nechar.h"
#include "ne_common/ne_comdef.h"

typedef struct ne_mm_pool *ne_handle ;
#include "ne_common/ne_handle.h"
#include "ne_common/ne_mempool.h"

#define MIN_SIZE				MIN_ALLOC_SIZE

#define SIZE_ALINE(s)			ROUNE_SIZE(s)

#define BIG_SIZE				16			//����������޵��ڴ潫�������ڴ��ڴ������
#define LITTLE_CHUNK_NUM		(BIG_SIZE>>3)	//С�ڴ�����ĸ���
#define POOL_SIZE_BITS			12				//��12λ����
#define CHUNK_INDEX(size)		((size)>>3)
#define SIZE_FROM_INDEX(index)	((index)<<3)

#define MIN_CHUNK_INDEX			2			//��С���ڴ������

#define DEFAULT_PAGE_SIZE	(1024*32)
#define MIN_PAGE_SIZE		8192
#define ROUNE_PAGE_SIZE(s) ((s) + 4095) & (~4095)

typedef NEUINT32 allocheader_t ;

#define UNLIMITED_SIZE		((allocheader_t)-1)
//���������ڴ�������ṹ
struct chunk_list{			//����ÿ���ڴ��Ľṹ
	allocheader_t size ;
	struct list_head list ;
};

//��¼��ϵͳ��������ڴ�ҳ�� ,Ϊ�˱�֤8���ֽڶ���,���в���ʹ��chunk_list
struct page_node{
	allocheader_t size ;
	struct page_node *next ;
};


//��������ʱʹ�õĽ��
struct alloc_node {
	allocheader_t size ;
	NEINT8 data[0];
};


/*��������������ÿ�ֳߴ���ڴ�,�����С������������ߴ�,
 ����ڴ����б�����*/
//�ڴ�ؽṹ
typedef struct ne_mm_pool
{
	NE_OBJ_BASE ;
	/*
	allocheader_t size ;
	NEINT32 type ;			
	NEUINT32	myerrno;
	ne_close_callback close_entry ;						//Ϊ�˺�handle����
*/
	allocheader_t capacity ;							//�ڴ������(ԭʼҳ���С)
	allocheader_t allocated_size ;						//�Ѿ�������ڴ��С(��¼ԭʼҳ���С,���ƹ���ʹ���ڴ�)

	ne_mutex lock ;										//�ڴ������
	NEINT8 *start, *end ;									//��ǰ���Է�����ڴ���ʼ��ַ
	struct list_head self_list;							//���ڴ���е��б�(�����ڴ��ʹ��)
	struct page_node *original_list;					//��ϵͳ�з�������ԭʼ�ڴ��.���ٵ�ʱ�������������پͺ���
//	struct list_head free_big_list ;					//���ڴ��б�
	struct list_head free_littles[LITTLE_CHUNK_NUM] ;	//С�ڴ�����(���2��û�б�ʹ��,���԰�free_littles[0]��Ϊ���泬��BIG_SIZE���б�)
}ne_mmpool_t;

//�����ڴ�������ĸ��ڵ�
//Ҳ������Ϊ�ڴ�ص�һ����������ʹ��
//�����������ڴ��ַ��4K����
struct pool_root_allocator{
	NEINT32 init ;		
	ne_mutex lock ;										//�Ƿ��ʼ��
	struct list_head inuser_list;						//ʹ���е��ڴ��
//	struct list_head free_big_list ;					//���ڴ��б�,�Ѿ��ͷ��ڴ��
	struct list_head free_littles[LITTLE_CHUNK_NUM] ;	//С���ڴ�8k�����(�������ڴ��,�������ڴ��)
} ;

static struct pool_root_allocator  __mem_root ;		//�ڴ������,ϵͳ�����ڴ�ĺ���
ne_mmpool_t *s_common_mmpool ;						//�����ڴ�� 

ne_mmpool_t *ne_global_mmpool()
{
	if(!__mem_root.init )
		return 0 ;
	return s_common_mmpool ;
}

//���Ҳ�ɾ��һ���ڴ�ڵ�
static struct chunk_list *search_del_chunk(struct list_head *header, allocheader_t size, NEINT32 equal_or_big) ;

#ifdef WIN32
static HANDLE  s_heap ;

//�������ϵͳ��ص��ڴ����뺯��
static void * __sys_alloc(size_t size)
{
	void *addr =0;
	if(!s_heap) {
#if 0
		s_heap = HeapCreate(0, 1024*1024, 1024*1024*512 ) ;
#else 
		s_heap = HeapCreate(0, 0, 0 ) ;
#endif 
		if(!s_heap) 
			return 0 ;
	}
#ifdef NE_DEBUG
	return  HeapAlloc(s_heap, HEAP_ZERO_MEMORY,(DWORD)size); 
#else 
	return HeapAlloc(s_heap, 0, (DWORD)size); 
#endif
	/*
	__try {
#ifdef NE_DEBUG
		addr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,(DWORD)size); 
#else 
		addr = HeapAlloc(GetProcessHeap(), 0, (DWORD)size); 
#endif
	}
	__except(EXCEPTION_EXECUTE_HANDLER ){
#ifdef NE_DEBUG
		DWORD dwExpCde = GetExceptionCode() ;
		switch(dwExpCde)
		{
		case STATUS_NO_MEMORY:
			ne_msgbox("alloc error EXPCODE is STATUS_NO_MEMORY", "error", MB_OK) ;
			break ;
		case STATUS_ACCESS_VIOLATION:
			ne_msgbox("alloc error EXPCODE is STATUS_ACCESS_VIOLATION", "error", MB_OK) ;
			break ;
		default :
			ne_msgbox("alloc error EXPCODE is unknow exception", "error", MB_OK) ;
			break ;
		}
		ne_assert(0) ;
#endif
		addr = 0 ;
	}
	*/
	return addr ;
}

static void __sys_free(void *addr)
{
#ifdef NE_DEBUG 
	if(!HeapFree(s_heap,0,addr)) {
		ne_showerror() ;
	}
#else 
	HeapFree(s_heap,0,addr) ;
#endif 
}
#else 
#include <unistd.h>

#define USE_LIBC_MALLOC
//��ϵͳ�����ڴ�
static void *__sys_alloc(size_t size ) 
{
#if !defined(USE_LIBC_MALLOC) 
	void *p ;
	static void *init_addr ;
	static ne_mutex slock = PTHREAD_MUTEX_INITIALIZER;
	
	ne_mutex_lock(&slock) ;
	if(size ==0 ) {
		init_addr = sbrk(0) ;
		p = init_addr ;
		ne_mutex_unlock(&slock) ; 
		return init_addr ;
	}

	p = sbrk(size) ;
	if((size_t)-1== (size_t)p ) {
		perror("sbrk") ;
		ne_mutex_unlock(&slock) ;
		return 0 ;
	}
	
	init_addr = p ;
	ne_mutex_unlock(&slock) ;
	return p ;
#else
	return malloc(size) ;
#endif
}

//��ϵͳ�ͷ��ڴ棬ʲôҲû��
static void __sys_free(void *p) 
{
#ifndef USE_LIBC_MALLOC
	return ;
#else 
	free(p) ;
#endif 
}

#endif

//�ڴ�س�ʼ��
NEINT32 ne_mempool_root_init()
{
	NEINT32 i ;
	if(	__mem_root.init )
		return 0 ;
	INIT_LIST_HEAD(&__mem_root.inuser_list) ;
//	INIT_LIST_HEAD(&__mem_root.free_big_list);
	for (i=0; i<LITTLE_CHUNK_NUM; i++){
		INIT_LIST_HEAD(&(__mem_root.free_littles[i])) ;
	}
	ne_mutex_init(&__mem_root.lock) ;
	
	__mem_root.init = 1 ;

	s_common_mmpool = ne_pool_create(EMEMPOOL_UNLIMIT) ;
	ne_assert(s_common_mmpool) ;

	if(!s_common_mmpool) {
		ne_mutex_destroy(&__mem_root.lock) ;
		__mem_root.init = 0 ;	
		return -1 ;	
	}
	return 0 ;
}

//�Ӹ������������ڴ�
//��������ڴ��Ѿ�����
static struct alloc_node * mm_root_allocator(size_t size )
{
	NEINT32 index ;
	struct chunk_list * chunk = NULL; 
	struct alloc_node * alloc_addr = NULL;
	struct list_head * free_header=NULL;

	ne_assert(__mem_root.init) ;
	if(size==0) {
		return NULL ;
	}
	
	//�����Ƿ��к��ʵ��ڴ��	
	index = size >> POOL_SIZE_BITS ;

	ne_assert(index>=1) ;

	if(index < LITTLE_CHUNK_NUM) {
		ne_mutex_lock(&__mem_root.lock) ;
		free_header = __mem_root.free_littles[index].next ;
		if(free_header!=&(__mem_root.free_littles[index])) {
			list_del(free_header) ;
			chunk = list_entry(free_header, struct chunk_list, list) ;
		}
	
		ne_mutex_unlock(&__mem_root.lock) ;
	}
	else {
		ne_mutex_lock(&__mem_root.lock) ;
		//free_header = __mem_root.free_big_list.next ;
		free_header = __mem_root.free_littles[0].next;
		if(free_header !=&(__mem_root.free_littles[0]) )
			chunk = search_del_chunk(&(__mem_root.free_littles[0]), size, 0) ;
		ne_mutex_unlock(&__mem_root.lock) ;
	}
	
	if(chunk) {
		alloc_addr = (struct alloc_node* )chunk ;
	}
	else {
		alloc_addr = __sys_alloc(size) ;
		if(alloc_addr)
			alloc_addr->size = size ;
	}
	
	return alloc_addr ;
}

//��root���ͷ�һ���ڴ��
static void mm_root_free(struct alloc_node*addr)
{
	NEUINT32 index ;
	struct chunk_list *chunk = (struct chunk_list *)addr ;
	if(!chunk)
		return ;
	index = chunk->size >> POOL_SIZE_BITS  ;
	INIT_LIST_HEAD(&chunk->list) ;

	if(index>=LITTLE_CHUNK_NUM) {
		ne_mutex_lock(&__mem_root.lock) ;
		list_add(&chunk->list, &__mem_root.free_littles[0]) ;
		ne_mutex_unlock(&__mem_root.lock) ;
	}
	else {
		ne_mutex_lock(&__mem_root.lock) ;
		list_add(&chunk->list, &__mem_root.free_littles[index]) ;
		ne_mutex_unlock(&__mem_root.lock) ;
	}
}

//�ڴ������
void ne_mempool_root_release()
{
	if(s_common_mmpool) {
		ne_pool_destroy(s_common_mmpool,0 ) ;
		s_common_mmpool = 0 ;
	}
	//nothing to be done !
#ifdef WIN32
	if(s_heap) {
		HeapDestroy(s_heap) ;
		s_heap = 0 ;
	}
	__mem_root.init = 0 ;
#endif
}

/*����һ���ڴ��,�����ڴ�ص�ַ*/
ne_handle ne_pool_create(size_t maxsize ) 
{
	NEINT32 i ;
	allocheader_t size ;
	ne_handle pool ;

	if(0 == maxsize) {
		return NULL ;
	}
	else if(-1 == maxsize) {
		size = DEFAULT_PAGE_SIZE ;
	}
	else if(maxsize <= DEFAULT_PAGE_SIZE) {
		size = DEFAULT_PAGE_SIZE ;
		maxsize = DEFAULT_PAGE_SIZE ;
	}
	else {
		size = DEFAULT_PAGE_SIZE ;
		maxsize = ROUNE_PAGE_SIZE(maxsize) ;
	}

	pool = (ne_handle)mm_root_allocator(size) ;
	if(!pool) {
		return NULL ;
	}

	pool->type = 'm'<<8 | 'p';	
	pool->capacity = (allocheader_t)maxsize ;
	pool->allocated_size = size ;
	pool->myerrno = NEERR_SUCCESS ;
	
	ne_mutex_init(&pool->lock) ;				//�ڴ������
	INIT_LIST_HEAD(&pool->self_list);			//���ڴ���е��б�(�����ڴ��ʹ��)
	
	for (i=0; i<LITTLE_CHUNK_NUM; i++){
		INIT_LIST_HEAD(&(pool->free_littles[i])) ;
	}
	pool->original_list = NULL;					//��ϵͳ�з�������ԭʼ�ڴ��.����ʱ��Ҫ��������ж��ͷ�
	pool->end = (NEINT8*)pool + pool->size ;
	pool->start =(NEINT8*)(pool + 1) ;

	pool->close_entry = (ne_close_callback )ne_pool_destroy ;
	//add memory allocator root 
	ne_mutex_lock(&__mem_root.lock) ;
		list_add(&pool->self_list, &__mem_root.inuser_list) ;
	ne_mutex_unlock(&__mem_root.lock) ;
	return pool ;
}

//����һ���ڴ滺���
NEINT32 ne_pool_destroy(ne_mmpool_t *pool, NEINT32 flag)
{
	struct page_node *chunk,*next ;
	if(!pool)
		return -1;

	chunk = pool->original_list ;
	while (chunk){
		next = chunk->next ;
		mm_root_free((struct alloc_node*)chunk) ;
		chunk = next ;
	}	

	ne_mutex_lock(&__mem_root.lock) ;
		list_del(&pool->self_list) ; 
	ne_mutex_unlock(&__mem_root.lock) ;
	
	ne_mutex_destroy(&pool->lock);
	mm_root_free((struct alloc_node*)pool) ;
	return 0 ;
}

//���³�ʼ��һ���ڴ��
void ne_pool_reset(ne_mmpool_t *pool)
{
	NEINT32 i;

	struct page_node *chunk,*next ;
	if(!pool)
		return ;
	ne_mutex_lock(&pool->lock); 

	chunk = pool->original_list ;
	while (chunk){
		next = chunk->next ;
		mm_root_free((struct alloc_node*)chunk) ;		
		chunk = next ;
	}
	pool->original_list = NULL;
	
	pool->allocated_size = pool->size ;

	for (i=0; i<LITTLE_CHUNK_NUM; i++){
		INIT_LIST_HEAD(&(pool->free_littles[i])) ;
	}
	pool->end = (NEINT8*)pool + pool->size ;
	pool->start =(NEINT8*)(pool + 1) ;
	ne_mutex_unlock(&pool->lock); 
}

//���ڴ��������һ��ԭʼ�ڴ�(��ϵͳ����)
static struct page_node * alloc_page(ne_mmpool_t *pool,NEINT32 size) 
{
	struct page_node *addr ;
	
	size += sizeof(*addr) ;
	size = ROUNE_PAGE_SIZE(size) ;
	
	addr = (struct page_node *) mm_root_allocator( size ) ;
	if(!addr) 
		return NULL ;
	
	addr->next = pool->original_list ;
	pool->original_list = addr ;
	
	pool->allocated_size += addr->size ;
	ne_assert(size ==addr->size);
	return addr ;
}


//��һ�ڴ����ӵ����ж���
static void pool_add_free(struct ne_mm_pool *pool , struct chunk_list *insert_node) 
{
	NEUINT32 index ;

	INIT_LIST_HEAD(&(insert_node->list)) ;
	index = CHUNK_INDEX(insert_node->size) ;
	insert_node->size |= 1 ;		// set lowest bit 
	ne_assert(index>= MIN_CHUNK_INDEX) ;
	if(index >=LITTLE_CHUNK_NUM) {
		//add big free list 
		list_add(&insert_node->list, &pool->free_littles[0]) ;
	}
	else {
		list_add(&insert_node->list, &pool->free_littles[index]) ;
	}
}

//�ѿ����ڴ����ӵ����ж�����
static void add_freeto_list(struct ne_mm_pool *pool)
{
	size_t free_size = pool->end - pool->start ;
	if(free_size>=MIN_SIZE) {
		struct chunk_list *tmp_chunk = (struct chunk_list *)pool->start ;
		tmp_chunk->size = free_size ;
		pool_add_free(pool, tmp_chunk) ;
		pool->start = pool->end;
	}
}

//�ͷ�һ���ڴ��
static void pool_free_chunk(struct ne_mm_pool *pool , struct chunk_list *chunk) 
{
	allocheader_t free_size ;
	struct chunk_list *insert_node ;

	ne_assert(chunk->size > 0) ;
	free_size = pool->end - pool->start ;
	if(free_size==0) {
		pool->start = (NEINT8*)chunk ;
		pool->end = (NEINT8*)chunk + (chunk->size & ~1) ;
		return ;
	}
	else if(chunk->size > (free_size +1) ) {
		insert_node = (struct chunk_list *) pool->start ;
		insert_node->size = free_size ;
		pool->start = (NEINT8*)chunk ;
		pool->end = (NEINT8*)chunk + (chunk->size & ~1) ;
	}
	else {
		insert_node = chunk ;
	}
	pool_add_free(pool, insert_node) ;
	
	ne_assert((insert_node->size& 1)) ;
}

//�Ӷ���header���ҵ�һ����size����ڴ��,���ҰѸÿ�Ӷ�����ɾ��
//equal_or_big = 0 ��sizeһ�����,equal_or_big=1�� >=szie���ڴ�
struct chunk_list *search_del_chunk(struct list_head *header, allocheader_t size, NEINT32 equal_or_big)
{
	struct list_head *pos ;
	struct chunk_list *chunk ;
	pos = header->next ;
	while(pos != header) {
		chunk = list_entry(pos, struct chunk_list, list) ;
		if(equal_or_big){
			if(chunk->size >= size) {
				list_del(pos) ;
				return chunk ;
			}
		}
		else {
			if( chunk->size == size) {				
				list_del(pos) ;
				return chunk ;
				//break ;
			}
		}
		pos = pos->next ;
	}
	return NULL;
	
}

#ifdef NE_DEBUG
NEINT32 check_chunk(struct ne_mm_pool *pool,struct chunk_list *chunk  )
{
	NEINT32 i ;
	for (i =0; i<LITTLE_CHUNK_NUM; i++)	{
		struct list_head *pos;
		struct list_head *header = &pool->free_littles[i] ;
		struct chunk_list *chunk_tmp ;

		list_for_each(pos,header){
			chunk_tmp = list_entry(pos, struct chunk_list, list) ;
			if(chunk == chunk_tmp) {
				return 1;
			}
		}
	}
	return 0 ;
}
#endif 
//��[start,end)֮��������ڴ�ϲ���һ������ڴ�
//return value; address of  max free chunk
static struct chunk_list * tryto_merge_border(NEINT8 *start, NEINT8 *end,struct ne_mm_pool *pool)
{
	NEINT32 num = 0 ;
	struct chunk_list *chunk  ;
	struct chunk_list *free_start = NULL ;
	struct chunk_list *max_chumk = NULL;
	
	while(start < end ) {
		chunk = (struct chunk_list *) start ;
		ne_assert(chunk->size>0) ;
		start += chunk->size & ~1  ;

		if(chunk->size & 1) {		//current node is free
			if(free_start==NULL) {
				num = 0 ;
				free_start = chunk ;
			}
			else {
				free_start->size += chunk->size & ~1 ;
				list_del(&chunk->list) ;
				++num ;
			}
				
			if(max_chumk==NULL || (max_chumk->size) < free_start->size) {
				max_chumk = free_start ;
			}
			
		}
		else {
			if(free_start){
				if(num ) {
					list_del(&free_start->list) ;
					pool_add_free(pool, free_start) ; 
				}
				free_start = NULL;
				num = 0 ;				
			}
		}
	}
	return max_chumk ;
	
}

/*��С����ڴ��ϲ��ɴ����ڴ��*/
static void pool_merge_free(ne_mmpool_t *pool)
{
	NEINT8 *start, *end ;
	struct page_node *page, *nextpage, *pre ;
	struct chunk_list  *max_chunk  ;

	//��[start,end)֮����ڴ���ӵ����ж�����
	add_freeto_list(pool) ;
	/*if(pool->start < pool->end) {
		struct chunk_list *tmp_chunk = (struct chunk_list *)pool->start ;
		tmp_chunk->size = pool->end - pool->start ;
		ne_assert(tmp_chunk->size >= MIN_SIZE) ;
		pool_add_free(pool, tmp_chunk) ;
		pool->start = pool->end;
	}
	*/
	start = (NEINT8*) (pool+1) ;
	end = ((NEINT8*)pool ) + pool->size ;
	
	max_chunk = tryto_merge_border(start, end, pool) ;
	
	page = pool->original_list ;
	pre = pool->original_list ;
	while(page){
		struct chunk_list*  ret_chunk ;

		nextpage = page->next ;		
		start =(NEINT8*) (page + 1) ;
		end = (NEINT8*)page + page->size ;
		ret_chunk = tryto_merge_border(start, end, pool) ;
		
		//try to free page
		if((NEINT8*)ret_chunk==start && (ret_chunk->size & ~1)==(page->size - sizeof(struct page_node )) ) {
			if(page == pool->original_list ){
				pool->original_list = nextpage ;
				pre = pool->original_list ;
			}
			else {
				pre->next = nextpage ;
			}
			page->next = 0 ;
			pool->allocated_size -= page->size ;
			list_del(&ret_chunk->list) ;
			mm_root_free((struct alloc_node*)page) ;
			
			page = nextpage ;
			ret_chunk = NULL;
			continue ;
		}
		
		if(ret_chunk){
			if(max_chunk==NULL || ret_chunk->size > max_chunk->size) {
				max_chunk = ret_chunk ;
			}
		}
		pre = page ;
		page = nextpage ;	
		
	}
	
	if(max_chunk) {
		ne_assert(pool->start==pool->end) ;
		list_del(&max_chunk->list) ;
		pool->start =(NEINT8*) max_chunk ;
		pool->end = (NEINT8*) max_chunk + (max_chunk->size & ~1) ;
	}
		
}

/*�ӻ����������һ���ڴ�*/
static void *__pool_alloc(ne_mmpool_t *pool , size_t size)
{
	NEINT32 index  ;
	size_t  free_size;
	struct alloc_node *alloc_addr;
	struct chunk_list *chunk ;
	struct list_head *pos ;
	
	size += sizeof(struct alloc_node) ;
	size = SIZE_ALINE(size) ;

	index = CHUNK_INDEX(size) ;
	
	ne_assert(index>=MIN_CHUNK_INDEX) ;
	//�����ж����Ƿ����ʺϵĽڵ�
	if(index<LITTLE_CHUNK_NUM) {
		pos = pool->free_littles[index].next ;
		if(pos != &pool->free_littles[index]) {
			chunk = list_entry(pos, struct chunk_list, list) ;
			list_del(pos) ;
			alloc_addr = (struct alloc_node *)chunk ;
			alloc_addr->size &= ~1;
			ne_assert(size==alloc_addr->size) ;
			return (void*)(alloc_addr->data) ;
		}
		
	}
	
	free_size = pool->end - pool->start ;	
	if(free_size < size){			//ʣ��δ�����ڴ�鲻����		
		//�Ӵ�������ڴ���в���
		chunk = search_del_chunk(&pool->free_littles[0], size+1,1) ;		//size + 1 free-list�е�szie���λָʾ�ڴ���Ƿ�����
		if(!chunk){
			//�Ӹ����С���ڴ��б��в���
			while( (++index) < LITTLE_CHUNK_NUM ) {
				pos = pool->free_littles[index].next ;
				if(pos != &pool->free_littles[index]) {
					chunk = list_entry(pos, struct chunk_list, list) ;
					list_del(pos) ;
					break ;
				}
			}
		}
		if(chunk){					//�ҵ���һ�������ڴ��
			ne_assert(chunk->size>=size+1) ;
			add_freeto_list(pool) ;
			//�ѿ����ڴ��ŵ��ڴ�ط������ռ�
			chunk->size = (chunk->size & ~1) ;
			pool->start = (NEINT8*)chunk ;
			pool->end = chunk->size + pool->start ;
		}		
		else {		
			if(pool->allocated_size>=pool->capacity){
				pool_merge_free(pool) ;	 
				free_size = pool->end - pool->start ;
				if(free_size < size){
					//����ڴ��Ѿ��ﵽʹ�ý���
					pool->myerrno = NEERR_LIMITED ;
					return NULL ;
				}
			}
			else {
				struct page_node *page ;
				allocheader_t new_block = DEFAULT_PAGE_SIZE - sizeof(*page) ;
				new_block = max(new_block, (size+sizeof(*page)) );					
				page = alloc_page(pool, new_block) ;	//�������ԭʼ�ڴ��,�Ѿ���ӵ�pool��ԭʼ������
				if(!page) {
					pool_merge_free(pool) ;	 
					free_size = pool->end - pool->start ;
					if(free_size < size){
						//�޿����ڴ�
						pool->myerrno = NEERR_NOSOURCE ;
						return NULL ;
					}
				}
				else {
					add_freeto_list(pool) ;
					pool->start = (NEINT8*)(page+1) ;
					pool->end = (NEINT8*)page + page->size ;
				}
			}
		}
	}

	alloc_addr = (struct alloc_node *)pool->start ;
	alloc_addr->size = size ;
	pool->start += size ;
	free_size = pool->end - pool->start ;
	if(free_size<MIN_SIZE) {
		alloc_addr->size += free_size ;
		pool->start = pool->end; 
	}
	return (void*)(alloc_addr->data) ;
}
void *ne_pool_alloc(ne_mmpool_t *pool , size_t size)
{
	void *addr ; 
	ne_assert(pool) ;
	ne_assert(size>0) ;

	pool->myerrno = NEERR_SUCCESS ;
	if (size==0){
		pool->myerrno = NEERR_INVALID_INPUT ;
		return 0;
	}
	
	ne_mutex_lock(&pool->lock);
	addr = __pool_alloc(pool,size) ;
	ne_mutex_unlock(&pool->lock);

#ifdef NE_DEBUG
	if(addr){
		struct alloc_node *alloc_tmp = (struct alloc_node *)addr ;
		--alloc_tmp ;
		ne_assert(!(alloc_tmp->size& 1)) ;
		ne_assert(alloc_tmp->size >= size) ;
	}
#endif
	return addr ;
}

/*�ͷ�һ���ڴ�*/
void ne_pool_free(ne_mmpool_t *pool ,void *addr)
{
	struct chunk_list*free_addr ;
	ne_assert(addr) ;
	free_addr = (struct chunk_list*)(((struct alloc_node *)addr ) -1 );
	ne_assert((NEINT32)free_addr->size>0) ;
	ne_mutex_lock(&pool->lock);
		pool_free_chunk( pool , free_addr) ; 
	ne_mutex_unlock(&pool->lock);
}

//ȫ�ַ��亯��
void *ne_global_alloc(size_t size) 
{
	ne_assert(__mem_root.init) ;
	ne_assert(s_common_mmpool) ;
	return ne_pool_alloc(s_common_mmpool, size) ;
}

void ne_global_free(void *p) 
{
	if(p) {
		ne_pool_free( s_common_mmpool ,p) ;
	}
}

//////////////////////////////////////////////////////////////////////////
//�����ڴ��ͷ�Խ��
#ifdef NE_DEBUG
struct __alloc_header {
	NEUINT32 __magic: 16;
	NEUINT32 __type_size:16;
	NEUINT32 _M_size;
}; // that is 8 bytes for sure
// Sunpro CC has bug on enums, so extra_before/after set explicitly
enum { __pad = 8, __magic = 0xdeba, __deleted_magic = 0xdebd,
	__shred_byte = 0xb8
};
enum { __extra_before = 16, __extra_after = 8 };

// ne_alloc_check ne_free_check  ��Ҫ�Ǽ��allocfn����������ڴ��ڴ���û��Խ��
void *ne_alloc_check(size_t __n,ne_alloc_func allocfn) 
{
	size_t __real_n = __n + __extra_before + __extra_after;
	struct __alloc_header *__result = 
		(struct __alloc_header *)allocfn(__real_n);
	if(__result) {
		memset((NEINT8*)__result, __shred_byte, __real_n*sizeof(NEINT8));
		__result->__magic = __magic;
		__result->__type_size = sizeof(NEINT8);
		__result->_M_size = (NEUINT32)__n;
		return ((NEINT8*)__result) + (NEINT32)__extra_before;
	}
	else {
		return NULL ;
	}
}

//debug �汾�� _destroy_chunkpool 
void ne_free_check(void *__p, ne_free_func freefn) 
{
	NEUINT8* __tmp;
	struct __alloc_header * __real_p ;
	size_t __real_n ;
	if(__p==NULL) 
		return ;
	
	__real_p = (struct __alloc_header*)((NEINT8 *)__p -(NEINT32)__extra_before);
	
	__real_n = __real_p->_M_size + __extra_before + __extra_after;
	// check integrity
	ne_assert(__real_p->__magic != __deleted_magic) ;
	ne_assert(__real_p->__magic == __magic);
	ne_assert(__real_p->__type_size == 1);
	//ne_assert(__real_p->_M_size == __n);
	// check pads on both sides
	for (__tmp = (NEUINT8*)(__real_p+1); __tmp < (NEUINT8*)__p; __tmp++) {  
		ne_assert(*__tmp==__shred_byte) ;
	}


	for (__tmp = ((NEUINT8*)__p)+__real_p->_M_size*sizeof(NEINT8*);
	__tmp < ((NEUINT8*)__real_p)+__real_n ; __tmp++) {
		ne_assert(*__tmp==__shred_byte) ;
	}

	// that may be unfortunate, just in case
	__real_p->__magic = __deleted_magic;
	memset((NEINT8*)__p, __shred_byte, __real_p->_M_size*sizeof(NEINT8));
	freefn(__real_p);
}

#endif

size_t ne_pool_freespace(ne_mmpool_t *pool)
{
	return pool->end - pool->start ;
}

#undef NE_IMPLEMENT_HANDLE

