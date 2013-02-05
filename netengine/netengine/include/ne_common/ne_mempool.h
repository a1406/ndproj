/*
 * ����һ�������mempool 
 * ֻ�ṩ�˶��ض����Ⱥ͸������ڴ滺���.
 * ����Ҫʹ�ö��ͬ����С�Ľṹ�����ڴ�����,��������һ���ڴ��,
 * Ȼ��ÿ��ʹ�õ�ʱ�������ڴ��������һ���ڴ�����,�����˾Ϳ����ͷŸ��ڴ��.
 */
#ifndef _NE_MEMPOOL_H_
#define _NE_MEMPOOL_H_

#define ALIGN_SIZE			8
#define ROUNE_SIZE(s) 		((s)+7) & (~7)

#define MIN_ALLOC_SIZE		(ALIGN_SIZE *2)

//�ڴ������
enum emem_pool_type{
	EMEMPOOL_TINY = 32*1024,		//΢���ڴ��
	EMEMPOOL_NORMAL = 256*1024,		//��ͨ��С�ڴ��
	EMEMPOOL_HUGE = 1024*1024,		//�����ڴ��
	EMEMPOOL_UNLIMIT = -1			//�������ڴ��
};

//�ڴ��ģ���ʼ��/�ͷź���
NE_COMMON_API NEINT32 ne_mempool_root_init() ;
NE_COMMON_API void ne_mempool_root_release();

//�ڴ�ز�������
NE_COMMON_API ne_handle ne_pool_create(size_t size ) ;					//����һ���ڴ��,�����ڴ�ص�ַ
NE_COMMON_API NEINT32 ne_pool_destroy(ne_handle pool, NEINT32 flag);		//����һ���ڴ滺���
NE_COMMON_API void *ne_pool_alloc(ne_handle pool , size_t size);	//�ӻ����������һ���ڴ�
NE_COMMON_API void ne_pool_free(ne_handle pool ,void *addr) ;		//�ͷ�һ���ڴ�
NE_COMMON_API void ne_pool_reset(ne_handle pool) ;					//reset a memory pool
//����ָ�������ڴ��,��Ҫ��ָ�����ȵ��ͷź���
//��ʹ�� *_alloc_l���������ڴ�ʱ,����ʹ��*_free_l�����ͷ�,����������Ԥ�ڵĴ���
//����*_lϵ�к�����Ŀ��������ڴ��ʹ��Ч��,�Ȳ���_l��ϵ�н�ʡ4���ֽ�
NE_COMMON_API void *ne_pool_alloc_l(ne_handle pool , size_t size) ;
NE_COMMON_API void ne_pool_free_l(ne_handle pool ,void *addr, size_t size) ;

NE_COMMON_API size_t ne_pool_freespace(ne_handle pool) ;	//get free space 

/*�õ�ȫ��Ĭ�ϵ��ڴ��*/
NE_COMMON_API ne_handle ne_global_mmpool() ;

#ifdef NE_DEBUG
typedef void *(*ne_alloc_func)(size_t __s) ;		//�����ڴ����뺯��ָ��
typedef void (*ne_free_func)(void *__p) ;			//�����ڴ��ͷź���ָ��

//���allocfn��������ڴ��Ƿ�Խ��
NE_COMMON_API void *ne_alloc_check(size_t __n,ne_alloc_func allocfn)  ;
NE_COMMON_API void ne_free_check(void *__p, ne_free_func freefn)  ;

#endif 

//ȫ�ַ��亯��
NE_COMMON_API void *ne_global_alloc(size_t size) ;
NE_COMMON_API void ne_global_free(void *p) ;

//����ָ�����ȵ��ڴ�,���ڴ�����ʼ���������ڴ�鳤��
//NE_COMMON_API void *ne_global_alloc_l(size_t size) ;
//NE_COMMON_API void ne_global_free_l(void *p, size_t size)  ;


#endif
