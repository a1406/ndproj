#ifndef _NE_HANDLE_H_
#define _NE_HANDLE_H_

/*
 * �����Ҫʵ���Լ���handle����,���ڰ�����ǰ�ļ�ǰ ���� NE_IMPLEMENT_HANDLE
 * Ϊ�˱����Զ������ͺ͵�ǰ�ļ��е����ͳ�ͻ
 */

typedef NEINT32(*ne_close_callback)(void* handle, NEINT32 flag) ;			//����رպ���
typedef void (*ne_init_func)(void*)  ;								//�����ʼ������

//����رշ�ʽ
enum eObjectClose{
	COMMON_CLOSE,
	FORCE_CLOSE
};

#define NE_OBJ_BASE \
	NEINT32 size ;	\
	NEINT32 type  ;	\
	NEUINT32	myerrno;		\
	ne_close_callback close_entry 

struct tag_ne_handle
{
	NE_OBJ_BASE ;
	/*
	NEINT32 size ;						//����Ĵ�С
	NEINT32 type  ;					//�������	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			//����ͷź���
	*/
} ;

#ifndef NE_IMPLEMENT_HANDLE
	/* ����һ��ͨ�õľ������*/
	typedef struct tag_ne_handle *ne_handle ;
#else 
#endif

/* ����һ������ʵ��,��ne_create_object() ����֮ǰע��һ�����ֽ�name�Ķ�������,�Ϳ�����create��������
 * ���ض���ľ��
 * ne_object_create() ʹ��malloc����������һ���ڴ�,���ǲ�û����
 */
NE_COMMON_API ne_handle _object_create(NEINT8 *name) ;

/* 
 * ����һ�����, �������ne_object_create ���������Ķ���,����Ҫ�Լ��ͷ��ڴ�
 * ������Ҫ�ֶ��ͷ��ڴ�
 * force = 0 �����ͷ�,�ȵ���ռ�õ���Դ�ͷź󷵻�
 * force = 1ǿ���ͷ�,���ȴ� ref eObjectClose
 */
NE_COMMON_API NEINT32 _object_destroy(ne_handle handle, NEINT32 force) ;

#define OBJECT_NAME_SIZE		40
/*���ע����Ϣ*/
struct ne_handle_reginfo
{
	NEINT32 object_size ;		//�������Ĵ�С
	ne_close_callback close_entry ;	//�رպ���
	ne_init_func	init_entry ;	//��ʼ������
	NEINT8 name[OBJECT_NAME_SIZE] ;	//��������
};

/* ע��һ����������,������windows�������͵�ע��
 * ע�����֮��Ϳ���ʹ�ô�����������һ�����ڵ�ʵ��
 */
NE_COMMON_API NEINT32 ne_object_register(struct ne_handle_reginfo *reginfo) ;

NEINT32 destroy_object_register_manager(void) ;

#ifdef NE_DEBUG
static __INLINE__ ne_handle  object_create(NEINT8 *name,NEINT8 *file, NEINT32 line) 
{	
	ne_handle p = _object_create(name )  ;
	if(p) {
		_source_log(p,(NEINT8*)"ne_object_create",(NEINT8*)"object not release!", file,line) ;
	}
	return p ;
}
static __INLINE__ NEINT32 object_destroy(ne_handle handle, NEINT32 force) 
{
	if(handle){
		NEINT32 ret =_source_release(handle) ;
		//ne_assert(0==ret) ;
		return _object_destroy(handle, force) ;
	}
	return -1 ;
}
#define ne_object_create(name)		object_create(name,(NEINT8*)__FILE__, __LINE__) 
#define ne_object_destroy(h,flag)	object_destroy(h, flag)

#else 
#define ne_object_create(name)		_object_create(name)
#define ne_object_destroy(h,flag)	_object_destroy(h, flag)
#endif

static __INLINE__ NEUINT32 ne_object_lasterror(ne_handle h)
{
	return ((struct tag_ne_handle*)h)->myerrno ;
}

static __INLINE__ void ne_object_seterror(ne_handle h, NEUINT32 errcode)
{
	((struct tag_ne_handle*)h)->myerrno = errcode;
}

NE_COMMON_API NEINT8 *ne_object_errordesc(ne_handle h) ;
#endif
