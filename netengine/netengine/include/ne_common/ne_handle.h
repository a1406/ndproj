#ifndef _NE_HANDLE_H_
#define _NE_HANDLE_H_

/*
 * 如果需要实现自己的handle类型,请在包含当前文件前 定义 NE_IMPLEMENT_HANDLE
 * 为了避免自定义类型和当前文件中的类型冲突
 */

typedef NEINT32(*ne_close_callback)(void* handle, NEINT32 flag) ;			//对象关闭函数
typedef void (*ne_init_func)(void*)  ;								//对象初始化函数

//对象关闭方式
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
	NEINT32 size ;						//句柄的大小
	NEINT32 type  ;					//句柄类型	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			//句柄释放函数
	*/
} ;

#ifndef NE_IMPLEMENT_HANDLE
	/* 定义一个通用的句柄类型*/
	typedef struct tag_ne_handle *ne_handle ;
#else 
#endif

/* 创建一个对象实例,在ne_create_object() 函数之前注册一个名字叫name的对象类型,就可以用create函数创建
 * 返回对象的句柄
 * ne_object_create() 使用malloc函数申请了一块内存,但是并没有在
 */
NE_COMMON_API ne_handle _object_create(NEINT8 *name) ;

/* 
 * 销毁一个句柄, 如果是用ne_object_create 函数创建的对象,则不需要自己释放内存
 * 否则需要手动释放内存
 * force = 0 正常释放,等等所占用的资源释放后返回
 * force = 1强制释放,不等待 ref eObjectClose
 */
NE_COMMON_API NEINT32 _object_destroy(ne_handle handle, NEINT32 force) ;

#define OBJECT_NAME_SIZE		40
/*句柄注册信息*/
struct ne_handle_reginfo
{
	NEINT32 object_size ;		//句柄对象的大小
	ne_close_callback close_entry ;	//关闭函数
	ne_init_func	init_entry ;	//初始化函数
	NEINT8 name[OBJECT_NAME_SIZE] ;	//对象名字
};

/* 注册一个对象类型,类似于windows窗口类型的注册
 * 注册完成之后就可以使用创建函数创建一个对于的实例
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
