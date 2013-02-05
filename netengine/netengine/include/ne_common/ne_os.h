#ifndef _NE_OS_H_
#define _NE_OS_H_

//#define NEINT8  signed char
//#define NEUINT8 unsigned char

//#define NEINT16 signed short
//#define NEUINT16 unsigned short

//#define NEINT32  signed int
//#define NEUINT32 unsigned int
typedef signed char 	NEINT8  ;
typedef unsigned char 	NEUINT8 ;

typedef signed short 	NEINT16  ;
typedef unsigned short 	NEUINT16 ;

typedef signed int		NEINT32  ;
typedef unsigned int	NEUINT32 ;

typedef unsigned int	NEBOOL ;

typedef double          NEFLOAT;

#if defined(NE_UNICODE)
typedef unsigned short  NEBYTE;
#else 
typedef unsigned char	NEBYTE;
#endif
#define NETRUE			1 
#define NEFALSE			0

#define NE_ESC			0x1b
#ifdef __cplusplus
#define CPPAPI extern "C" 
#else 
#define CPPAPI 
#endif

#define  NE_MULTI_THREADED 1		//使用多线程


#ifdef WIN32
#include "ne_common/ne_win.h"
	typedef signed __int64 NEINT64  ;
	typedef unsigned __int64 NEUINT64 ; 

	typedef HANDLE			neth_handle ;	//线程句柄
	typedef DWORD 			nethread_t;	//线程id
	__INLINE__ int ne_thread_equal(nethread_t t1, nethread_t t2){return (t1==t2) ;}
	
	//typedef volatile long atomic_t ;
	static __INLINE__ int set_maxopen_fd(int max_fd) {return 0;}
	#define ne_close_handle(h)	CloseHandle(h)
	
#else //if __LINUX__		//UNIX OR linux platform
#include "ne_common/ne_unix.h"
	typedef signed long long int INT64  ;
	typedef unsigned long long int NEUINT64 ;
	typedef unsigned long		DWORD ;
	typedef unsigned char 		BYTE ;
	typedef unsigned short		WORD ;
	
	typedef pthread_t		neth_handle ;	//线程句柄
	typedef pthread_t 		nethread_t;		//线程ID
	#define  ne_thread_equal	pthread_equal
	NE_COMMON_API int set_maxopen_fd(int max_fd) ;
	#define ne_close_handle(h)	(void)0
#endif


#include "ne_common/ne_atomic.h"

typedef void* (*NETH_FUNC)(void* param) ;		//线程函数
enum {
	NET_PRIORITY_NORMAL,
	NET_PRIORITY_HIGHT,
	NET_PRIORITY_LOW
};
//创建线程函数
NE_COMMON_API neth_handle ne_createthread(NETH_FUNC func, void* param,nethread_t *thid,int priority);
//强迫线程让出执行时间
NE_COMMON_API int ne_threadsched(void) ;
//强迫线程退出
NE_COMMON_API void ne_threadexit(int exitcode);
//等待一个线程的结束
NE_COMMON_API int ne_waitthread(neth_handle handle) ;

NE_COMMON_API int ne_terminal_thread(neth_handle handle,int exit_code);


typedef struct nefast_lock
{
	neatomic_t  locked;
} nefastlock_t;

#define ne_flock(l) (0==ne_testandset(&((l)->locked)) ) 
#define ne_funlock(l) ne_atomic_swap(&((l)->locked),0)
#define ne_flock_init(l) ne_atomic_set(&((l)->locked),0) ;

static  __INLINE__ int ne_key_esc() 
{
	if(kbhit()) {
		if(NE_ESC==getch())
			return 1 ;
	}
	return 0 ;
}
#endif
