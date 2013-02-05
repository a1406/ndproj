#ifndef _NE_UINX_H_
#define _NE_UINX_H_

#ifndef WIN32
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <assert.h>

#define NE_COMMON_API CPPAPI

#ifndef __INLINE__
	#define __INLINE__			inline
#endif

typedef NEUINT32		HANDLE ;
typedef void				*HINSTANCE ;

#define LOWORD(a)			((a) & 0xffff)
#define HIWORD(a)			(((a) >>16) & 0xffff)

#define max(a,b) 			(((a)>(b))? (a) : (b))
#define min(a,b) 			(((a)>(b))? (b) : (a))

NE_COMMON_API NEINT32 ne_getch(void);
NE_COMMON_API NEINT32 kbhit ( void );
#define getch	ne_getch

#define NDSEM_SUCCESS		0
#define NDSEM_ERROR			-1
#define NDSEM_TIMEOUT		1

typedef sem_t 				ndsem_t ;			//信号变量
NE_COMMON_API NEINT32 _unix_sem_timewait(ndsem_t *sem , NEUINT32 waittime)  ;
#define ne_sem_wait(s, timeout)		_unix_sem_timewait(&(s), timeout) //sem_wait(&(s))		//等待信号
#define ne_sem_post(s)		sem_post(&(s))		//发送信号
#define ne_sem_init(s)		sem_init(&(s),0,0)	//initilize semahpore resource, return 0 on success , error return -1
#define ne_sem_destroy(s)   sem_destroy(&(s)) 		//destroy semahpore resource


#define ne_thread_self()	pthread_self()		//得到现成自己的id
#define ne_processid()		getpid()			//得到进程ID


NE_COMMON_API void pthread_sleep(NEUINT32 msec) ;
#define ne_sleep			pthread_sleep  		//睡眠1/1000 second

#define ne_assert(a)		assert(a)


#define __FUNC__ 	__ASSERT_FUNCTION
//last error 
#define ne_last_errno() errno
#define ne_last_error()	strerror(errno)
static __INLINE__ void _showerror(NEINT8 *file, NEINT32 line,const NEINT8 *func) { 
	fprintf(stderr,"[%s:%d(%s)] last error:%s\n",file, line, func, ne_last_error()) ;}
#define ne_showerror() _showerror(__FILE__,__LINE__,__FUNC__)
#define ne_msgbox(msg,title) fprintf(stderr,"%s:%s\n", title, msg) 

//define truck functon

#ifdef NE_DEBUG
#define NETRACF(msg,arg...) do{fprintf(stderr,"%s:%d",__FILE__,__LINE__) ; fprintf(stderr, msg,##arg);}while(0)
#define NETRAC(msg,arg...)   fprintf(stderr, msg,##arg)
#define _CRTTRAC(msg,arg...) fprintf(stderr, msg,##arg)
#else 
#define NETRACF(msg) (void) 0
#define NETRAC(msg) (void) 0
#define _CRTTRAC(msg) (void) 0
#define ne_msgbox_dg(text, cap,flag) (void) 0
#endif

#define ne_exit(code) exit(code)

NE_COMMON_API NEINT32 mythread_cond_timewait(pthread_cond_t *cond,
							pthread_mutex_t *mutex, 
							NEUINT32 mseconds);

typedef pthread_mutex_t		ne_mutex ;
typedef pthread_cond_t		ne_cond;

#define ne_mutex_init(m)	pthread_mutex_init((m), NULL) 
#define ne_mutex_lock(m) 	pthread_mutex_lock(m) 
#define ne_mutex_trylock(m)	pthread_mutex_trylock(m) 
#define ne_mutex_unlock(m) 	pthread_mutex_unlock(m) 
#define ne_mutex_destroy(m) pthread_mutex_destroy(m)

#define ne_cond_init(m)		pthread_cond_init((m), NULL) 
#define ne_cond_destroy(c)  pthread_cond_destroy(c)
#define ne_cond_wait(c, m)			pthread_cond_wait(c, m)
#define ne_cond_timewait(c,m, ms) 	pthread_cond_timedwait(c,m,ms)
#define ne_cond_signal(v)			pthread_cond_signal(v) 
#define ne_cond_broadcast(v)		pthread_cond_broadcast(v)


typedef pthread_rwlock_t		ne_rwlock ;
#define ne_rwlock_init(m) pthread_rwlock_init((m), NULL)
#define ne_rdlock(m) do {int __neret = pthread_rwlock_rdlock(m); if (__neret != 0) error(-1, __neret, ""); } while (0)
#define ne_rdtrylock(m) do {int __neret = pthread_rwlock_tryrdlock(m); if (__neret != 0) error(-1, __neret, ""); } while (0)
#define ne_wrlock(m) do {int __neret = pthread_rwlock_wrlock(m); if (__neret != 0) error(-1, __neret, ""); } while (0)
#define ne_wrtrylock(m) do {int __neret = pthread_rwlock_trywrlock(m); if (__neret != 0) error(-1, __neret, ""); } while (0)
#define ne_rwunlock(m) do {int __neret = pthread_rwlock_unlock(m); if (__neret != 0) error(-1, __neret, ""); } while (0)
#define ne_rwlock_destroy(m) pthread_rwlock_destroy(m)

#endif 
#endif
