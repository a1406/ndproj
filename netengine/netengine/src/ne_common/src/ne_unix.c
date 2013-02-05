#ifndef WIN32
#include "ne_common/ne_common.h"
#include <time.h>  

NEINT32 mythread_cond_timewait(pthread_cond_t *cond,
							pthread_mutex_t *mutex, NEUINT32 mseconds)
{
	struct timeval now;
	struct timespec timeout;
	NEINT32 retcode;
	
	if(-1==clock_gettime(CLOCK_REALTIME,&timeout) ){
		return -1 ;
	}

	timeout.tv_sec += mseconds /1000 ;
	timeout.tv_nsec += (mseconds %1000 ) * 1000000;
	return  pthread_cond_timedwait(cond, mutex, &timeout) ;
	
}


void pthread_sleep(NEUINT32 msec)
{
	pthread_cond_t _cond = PTHREAD_COND_INITIALIZER ;
	pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER ;
	
	mythread_cond_timewait(&_cond,&_mutex, msec) ;
}

//等待信号返回
//return NDSEM_SUCCESS wait success, NDSEM_ERROR error , NDSEM_TIMEOUT timeout
NEINT32 _unix_sem_timewait(ndsem_t *sem , NEUINT32 waittime) 
{
	if((NEUINT32)-1==waittime) {
		return sem_wait(sem) ;
	}
	else if(waittime){
		NEINT32 ret ;
		struct timespec ts ;
		
		if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
			return -1 ;
		}
		ts.tv_sec += waittime /1000 ;
		ts.tv_nsec += (waittime %1000 ) * 1000000;
		
		//这里不能使用 (ret = sem_timedwait(sem, &ts)) == -1 && errno == EINTR 这个判断
		//否则ctrl+c将不能使程序正常退出
		//while ((ret = sem_timedwait(sem, &ts)) == -1 && errno == EINTR)
		//	continue ;
		//check what happened
		ret = sem_timedwait(sem, &ts);
		if (ret == -1) {
			if (errno == ETIMEDOUT)
				return NDSEM_TIMEOUT ;
			else
				return NDSEM_ERROR ;
		} 
		else
			return NDSEM_SUCCESS ;
	}
	else {
		return  NDSEM_TIMEOUT;
	}
}

netime_t ne_time(void)
{
	netime_t inteval ;
	struct timeval tmnow;
	static struct timeval __start_time ;	//程序启动时间
	static NEINT32 __timer_inited = 0 ;
	
	if(0==__timer_inited) {
		gettimeofday(&__start_time, NULL) ;
		__timer_inited = 1 ;
	}
	gettimeofday(&tmnow, NULL) ;
	
	inteval = (tmnow.tv_sec - __start_time.tv_sec) * 1000 +  
		(tmnow.tv_usec - __start_time.tv_usec) / 1000 ;
	return inteval ;
}


#include <sched.h>

//创建线程函数
neth_handle ne_createthread(NETH_FUNC func, void* param,nethread_t *thid,NEINT32 priority)
{
	pthread_t threadid ;
	
	struct sched_param schparam ;
	pthread_attr_t attr ;
	
	pthread_attr_init(&attr);

	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate (&attr,PTHREAD_CREATE_JOINABLE);
	
	if(priority == NET_PRIORITY_HIGHT) {
		struct sched_param param ;
		
		pthread_attr_getschedparam(&attr, &param) ;
		
		++(param.sched_priority);
		pthread_attr_setschedparam(&attr, &param); 
	}
	else if(NET_PRIORITY_LOW) {
		struct sched_param param ;
		
		pthread_attr_getschedparam(&attr, &param) ;
		
		--(param.sched_priority);
		pthread_attr_setschedparam(&attr, &param); 
		
	}

	if(-1 == pthread_create(&threadid, &attr, func, param) ){
		return 0 ;
	}
	
	if(thid) *thid = (nethread_t)threadid ;
	return threadid ;
}

NEINT32 ne_threadsched(void) 
{
	return sched_yield() ; 
}

void ne_threadexit(NEINT32 exitcode)
{
	pthread_exit((void*)exitcode );
}
//等待一个线程的结束
NEINT32 ne_waitthread(neth_handle handle) 
{
	nethread_t selfid = ne_thread_self() ;
	if(selfid!=(nethread_t)handle){
		return pthread_join(handle,NULL)  ;
	}
	return -1 ;
}

NEINT32 ne_terminal_thread(neth_handle handle,NEINT32 exit_code)
{
	return pthread_cancel(handle) ;
}

#include <sys/ioctl.h>
#include <termios.h>

#ifdef __LINUX__
NEINT32 ne_getch(void)
{
	NEINT8 ch;
	struct termios save, ne;
	ioctl(0, TCGETS, &save);
	ioctl(0, TCGETS, &ne);
	ne.c_lflag &= ~(ECHO | ICANON);
	ioctl(0, TCSETS, &ne);
	read(0, &ch, 1);
	ioctl(0, TCSETS, &save);
	return ch;
}
#else
NEINT32 ne_getch( void )
{
	NEINT32 c = 0;
	struct termios org_opts, new_opts;
	NEINT32 res = 0;
	  //-----  store old settings -----------
	res = tcgetattr( STDIN_FILENO, &org_opts );
	assert( res == 0 );
	  //---- set new terminal parms --------
	memcpy( &new_opts, &org_opts, sizeof(new_opts) );
	new_opts.c_lflag &= ~( ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL );
	tcsetattr( STDIN_FILENO, TCSANOW, &new_opts );
	c = getchar();
	  //------  restore old settings ---------
	res = tcsetattr( STDIN_FILENO, TCSANOW, &org_opts );
	assert( res == 0 );
	return( c );
}
#endif

NEINT32 kbhit ( void )
{
	struct timeval tv;
	struct termios old_termios, new_termios;
	NEINT32            error;
	NEINT32            count = 0;
	tcgetattr( 0, &old_termios );
	new_termios = old_termios;
	/*
	 * raw mode
	 */
	new_termios.c_lflag     &= ~ICANON;
	/*
	 * disable echoing the NEINT8 as it is typed
	 */
	new_termios.c_lflag     &= ~ECHO;
	/*
	 * minimum chars to wait for
	 */
	new_termios.c_cc[VMIN] = 1;
	/*
	 * minimum wait time, 1 * 0.10s
	 */
	new_termios.c_cc[VTIME] = 1;
	error = tcsetattr( 0, TCSANOW, &new_termios );
	tv.tv_sec = 0;
	tv.tv_usec = 100;
	/*
	 * insert a minimal delay
	 */
	select( 1, NULL, NULL, NULL, &tv );
	error                   += ioctl( 0, FIONREAD, &count );
	error                   += tcsetattr( 0, TCSANOW, &old_termios );
	return( error == 0 ? count : -1 );
}  /* end of kbhit */


#include <sys/resource.h>
NEINT32 set_maxopen_fd(NEINT32 max_fd)
{
	struct rlimit rt;
    /* 设置每个进程允许打开的最大文件数 */
    /*需要以root运行*/
    rt.rlim_max = rt.rlim_cur = max_fd;
    setrlimit(RLIMIT_NOFILE, &rt) ;
    
    return  getdtablesize() ;
}

#endif 
