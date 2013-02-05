#ifndef _NE_WIN_H_
#define _NE_WIN_H_

/*
#ifndef _WIN32_WINNT
#define  _WIN32_WINNT 0x0500		//default winnt 5
#endif
*/

#ifndef _AFXDLL
#include <winsock2.h>
#include <windows.h>
#else
#include <winsock2.h>
#endif 

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

//#include "nechar.h"

#pragma warning (disable:  4018)	
#pragma warning (disable:  4251)	
/* nonstandard extension 'single line comment' was used */
#pragma warning(disable:4001)	
// unreferenced formal parameter
#pragma warning(disable:4100)	
// Note: Creating precompiled header 
#pragma warning(disable:4699)	
// function not inlined
#pragma warning(disable:4710)	
// unreferenced inline function has been removed
#pragma warning(disable:4514)	
// assignment operator could not be generated
#pragma warning(disable:4512)

#pragma  warning(disable: 4996)

#if  defined(NE_COMMON_EXPORTS) 
	#define NE_COMMON_API 				CPPAPI  __declspec(dllexport)
#else
	#define NE_COMMON_API 				CPPAPI __declspec(dllimport)
#endif

#define __INLINE__			__inline	

NE_COMMON_API NEINT8 *ne_process_name() ;
NE_COMMON_API NEINT32 ne_arg(NEINT32 argc, NEINT8 *argv[]);

// compatible for unix
#define snprintf _snprintf
#define bzero(pstr,size) 	memset((pstr),0,(size))		//定义bzero 兼容gcc bzero

//define assert
//only for x86
#ifdef _DEBUG
#define DebugBreak()    				_asm { int 3 }
#else  
#define DebugBreak()    				(void)0
#endif
__INLINE__ void NE_MsgBox(PCSTR s) {
	char buf[1024] ;
	snprintf(buf, 1024, "%s ASSERT FAILED" , ne_process_name()) ;
	MessageBoxA(GetActiveWindow(), s, buf, MB_OK);
}
__INLINE__ void NE_FAILED(PSTR szMsg) {
   NE_MsgBox(szMsg);
   DebugBreak();
}
// Put up an assertion failure message box.
__INLINE__ void NE_ASSERTFAIL(LPCSTR file, NEINT32 line, PCSTR expr) {
   char sz[1024];
   snprintf(sz, 1024, "ASSERT failed in\nFile %s, line %d \n"
	   "ne_assert(%s)", file, line, expr);
   NE_FAILED(sz);
}
// Put up a message box if an assertion fails in a debug build.
//定义NDASSERT()代替assert()

#ifdef _DEBUG
#define ne_assert(x) if (!(x)) NE_ASSERTFAIL(__FILE__, __LINE__, #x)
#else
#define ne_assert(x)
#endif


//define sem wait return value
#define NDSEM_SUCCESS		WAIT_OBJECT_0
#define NDSEM_ERROR			WAIT_ABANDONED
#define NDSEM_TIMEOUT		WAIT_TIMEOUT

//single operate
typedef HANDLE 				ndsem_t ;		//信号变量
static __INLINE__ NEINT32 _init_event(HANDLE *h)
{
	*h = CreateEvent(NULL,FALSE,FALSE,NULL) ;
	return (*h) ? 0 : -1 ;
}
#define ne_sem_wait(s,t)	WaitForSingleObject(s,t)			//等待信号
#define ne_sem_post(s)		SetEvent(s)							//发送信号
#define ne_sem_init(s)		_init_event(&(s))  //(s)=CreateEvent(NULL,FALSE,FALSE,NULL)	//initilize semahpore resource
#define ne_sem_destroy(s)   CloseHandle(s) 						//destroy semahpore resource

#define ne_sleep(ms)	Sleep(ms) 			//睡眠1/1000 second

//thread operate
#define ne_thread_self()	GetCurrentThreadId()				//得到现成自己的id
#define ne_processid()		GetCurrentProcessId()				//得到进程ID

NE_COMMON_API  DWORD _ErrBox(NEINT8 *file, NEINT32 line) ;
//last error 
#define ne_last_errno() GetLastError() 
NE_COMMON_API const NEINT8 *ne_last_error() ;		//得到系统的最后一个错误描述(不是ne_common模块的)
#define ne_showerror()	_ErrBox(__FILE__,__LINE__)		//弹出错误描述的对话框

#define __FUNC__ 	"unknow_function"
// 定义messagebox
#define ne_msgbox(msg,title) MessageBoxA(GetActiveWindow(), msg, title, MB_OK)

//定义TRACK
#ifdef _DEBUG
#include <crtdbg.h>
NE_COMMON_API NEINT32 MyDbgReport(NEINT8 *file, NEINT32 line, NEINT8 *stm, ...);
#define NETRACF(msg)  MyDbgReport(__FILE__, __LINE__,msg) 
#define NETRAC(msg)  MyDbgReport(NULL, 0 ,msg) 
#define _CRTTRAC(msg) do { if ((1 == _CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "%s", msg))) _CrtDbgBreak(); } while (0)
#define ne_msgbox_dg(msg,title,flag) \
do{	NEINT8 buf[1024] ;				\
	NEINT8 header[128] ;				\
	snprintf(buf,1024, "%s\n%s:%d line \n %s",(title),__FILE__,__LINE__, (msg)) ;	\
	ne_msgbox(buf,header,flag);		\
}while(0)
#else 
#define MyDbgReport 
#define NETRACF(msg) (void) 0
#define NETRAC(msg) (void) 0
#define _CRTTRAC(msg) (void) 0
#define ne_msgbox_dg(text, cap,flag) (void) 0

#endif	//_DEBUG

#define ne_exit(code)  PostQuitMessage(code)

//define condition value
#include <process.h>
typedef struct _sNE_mutex
{
	NEINT32 _spinCount ;	//spin
	NEINT32 lockCount ;		//lock
	NEINT32 used ;			//used times 
	DWORD ownerID ;		//owner thread id 
	HANDLE hSig ;
}NDMutex ;
typedef struct _sNE_condvar
{
	NEINT32 lockCount ;		//lock
	HANDLE hSig ;
}NDCondVar ;

typedef NDMutex					ne_mutex ;
typedef NDCondVar 				ne_cond ;

/*define mutex operation*/
NE_COMMON_API NEINT32 initNDMutex(NDMutex *m) ;
NE_COMMON_API NEINT32 tryEntryMutex(NDMutex *m);
NE_COMMON_API void entryMutex(NDMutex *m);
NE_COMMON_API void leaveMutex(NDMutex *m);
NE_COMMON_API void destoryMutex(NDMutex *m);

/*define condition value*/
NE_COMMON_API NEINT32 initNDCondVar(NDCondVar *v) ;
NE_COMMON_API NEINT32 waitCondVar(NDCondVar *v, NDMutex *m);
NE_COMMON_API NEINT32 timewaitCondVar(NDCondVar *v, NDMutex *m, DWORD mseconds);
NE_COMMON_API NEINT32 signalCondVar(NDCondVar *v);
NE_COMMON_API NEINT32 destoryCondVar(NDCondVar *v);
NE_COMMON_API NEINT32 broadcastCondVar(NDCondVar *v) ;

//定义互斥接口
#define ne_mutex_init(m)	initNDMutex(m)	//初始化互斥
#define ne_mutex_lock(m) 	entryMutex(m)	//lock
#define ne_mutex_trylock(m) tryEntryMutex(m) 
#define ne_mutex_unlock(m) 	leaveMutex(m) 
#define ne_mutex_destroy(m) destoryMutex(m) 

//定义条件变量接口
#define ne_cond_init(c)				initNDCondVar(c)
#define ne_cond_destroy(c)			destoryCondVar(c)
#define ne_cond_wait(c, m)			waitCondVar(c, m)
#define ne_cond_timewait(c,m, ms)	timewaitCondVar(c,m,ms)
#define ne_cond_signal(v)			signalCondVar(v) 
#define ne_cond_broadcast(v)		broadcastCondVar(v) 

#endif 
