#ifndef WIN32
#include "ne_common/ne_common.h"
#include "ne_app.h"
#include <signal.h>

NEINT8 *sig_desc[] = {
								 " ",
/*#define	SIGHUP		1	*/   " Hangup (POSIX).  ",
/*#define	SIGINT		2	*/   " Interrupt (ANSI).  ",
/*#define	SIGQUIT		3	*/   " Quit (POSIX).  ",
/*#define	SIGILL		4	*/   " Illegal instruction (ANSI).  ",
/*#define	SIGTRAP		5	*/   " Trace trap (POSIX).  ",
/*#define	SIGABRT		6	*/   " Abort (ANSI).  ",
/*#define	SIGIOT		6	*/   " IOT trap (4.2 BSD).  ",
/*#define	SIGBUS		7	*/   " BUS error (4.2 BSD).  ",
/*#define	SIGFPE		8	*/   " Floating-point exception (ANSI).  ",
/*#define	SIGKILL		9	*/   " Kill, unblockable (POSIX).  ",
/*#define	SIGUSR1		10	*/   " User-defined signal 1 (POSIX).  ",
/*#define	SIGSEGV		11	*/   " Segmentation violation (ANSI).  ",
/*#define	SIGUSR2		12	*/   "  User-defined signal 2 (POSIX).  ",
/*#define	SIGPIPE		13	*/   "  Broken pipe (POSIX).  ",
/*#define	SIGALRM		14	*/   "  Alarm clock (POSIX).  ",
/*#define	SIGTERM		15	*/   "  Termination (ANSI).  ",
/*#define	SIGSTKFLT	16	*/   "  Stack fault.  ",
/*#define	SIGCHLD		17	*/   "  Child status has changed (POSIX).or Same as SIGCHLD (System V).    ",
/*#define	SIGCONT		18	*/   "  Continue (POSIX).  ",
/*#define	SIGSTOP		19	*/   "  Stop, unblockable (POSIX).  ",
/*#define	SIGTSTP		20	*/   "  Keyboard stop (POSIX).  ",
/*#define	SIGTTIN		21	*/   "  Background read from tty (POSIX).  ",
/*#define	SIGTTOU		22	*/   "  Background write to tty (POSIX).  ",
/*#define	SIGURG		23	*/   "  Urgent condition on socket (4.2 BSD).  ",
/*#define	SIGXCPU		24	*/   "  CPU limit exceeded (4.2 BSD).  ",
/*#define	SIGXFSZ		25	*/   "  File size limit exceeded (4.2 BSD).  ",
/*#define	SIGVTALRM	26	*/   "  Virtual alarm clock (4.2 BSD).  ",
/*#define	SIGPROF		27	*/   "  Profiling alarm clock (4.2 BSD).  ",
/*#define	SIGWINCH	28	*/   "  Window size change (4.3 BSD, Sun).  ",
/*#define	SIGIO		29	*/   "  I/O now possible (4.2 BSD).  or Pollable event occurred (System V).",
/*#define	SIGPWR		30	*/   "  Power failure restart (System V).  ",
/*#define SIGSYS		31	*/   "  Bad system call.  "
/*#define SIGUNUSED	31 */

};


/* �źŴ��?��
 * if server program received a signal 
 * it will run here 
 * so I can do some to handle error
 * ���ʹ��ר�ŵ��߳�4����ϵͳ�ź�,
 * ��ô����װһ���źŴ����������߳���. 
 */
void _terminate_server(NEINT32 signo)
{
	NEINT8 *msg = get_signal_desc(signo);
	
	ne_logmsg("received signed %s server exit\n",msg);
	
	ne_host_eixt() ;
	
	end_server(0) ;
	ne_logmsg("good bye\n");
	exit(0) ;
}
/* install signal handle function 
 * because server program must backup user data
 * so it must know it's signal station.
 */
NEINT32 installed_sig_handle(void)
{
	NEINT32 i ;
	sigset_t blockmask, oldmask  ;
	struct sigaction sact ,oldact;
	
	sigemptyset(&blockmask);
	sigemptyset(&oldmask);
	
	sigaddset(&blockmask, SIGINT);
	sigaddset(&blockmask, SIGTERM);
	
	sact.sa_handler = _terminate_server ;
	
	sigemptyset(&sact.sa_mask );
	sigaddset(&sact.sa_mask, SIGINT);
	
	sigprocmask(SIG_BLOCK ,&blockmask, &oldmask );
	
	for(i=SIGHUP; i<SIGCONT; i++){
		if( (NEINT32)SIGKILL==i ||
			(NEINT32)SIGALRM==i ||
			(NEINT32)SIGPIPE==i )
			continue ;
		if(-1==sigaction(i, &sact, &oldact))	{
			ne_logmsg("Installed %d signal function error!", i );
			return -1 ;
		}
	}
	
	sigprocmask(SIG_UNBLOCK ,&blockmask, NULL );
	return 0 ;
}

/*�ȴ��ź���ں���* */
NEINT32 wait_signal_entry()
{
	NEINT32 i = 0 , intmask;
	static NEINT32 _inited=0;
	static sigset_t blockmask ;
	
	if(0==_inited) {
		sigemptyset(&blockmask) ;
		
		for(i=SIGHUP; i<32; i++)
		{
			/*if( (NEINT32)SIGALRM==i ||
				(NEINT32)SIGCHLD==i ||
				(NEINT32)SIGIOT==i  ||
				(NEINT32)SIGPIPE==i ||
				(NEINT32)SIGTRAP== i )
				continue ;
			*/
			sigaddset(&blockmask,i) ;
		}
		/*sigaddset(&blockmask,SIGTTOU) ;
		sigaddset(&blockmask,SIGTTIN) ;
		sigaddset(&blockmask,SIGXCPU) ;
		sigaddset(&blockmask,SIGXFSZ) ;
		sigaddset(&blockmask,SIGPWR) ;
		*/
	}

	sigwait(&blockmask, &intmask) ;
	printf_dbg( "recv signal  %d \n",intmask);
	if(0==intmask){
		printf_dbg( "recvd a system signal %d\n" AND intmask);
		return 0;
	}
	else if( (NEINT32)SIGALRM==intmask ||
			(NEINT32)SIGCHLD==intmask ||
			(NEINT32)SIGIOT==intmask  ||
			(NEINT32)SIGPIPE==intmask ) {
		printf_dbg( "recv signal but not handle %d \n" AND intmask);
		return 0;
	}
	printf_dbg("received %d signed %s program would exit\n"  AND  intmask AND get_signal_desc(intmask));
	
	return -1;
}

/*��ֹ�ź�*/
NEINT32 block_signal(void) 
{
	NEINT32 i ;
	sigset_t blockmask ;
	
	/*mask all signal */
	sigemptyset(&blockmask);

	for(i=SIGHUP; i<32; i++){
		if(	(NEINT32)SIGALRM==i ||
			(NEINT32)SIGPROF==i ||
			(NEINT32)SIGFPE==i  ||
			(NEINT32)SIGKILL==i )
			continue ;
		sigaddset(&blockmask,i) ;
	}
	sigprocmask(SIG_BLOCK ,&blockmask, NULL );

	return 0;
}

NEINT32 ignore_all_signal(void) 
{
	NEINT32 i ;
	
	sigset_t blockmask ;
	
	sigemptyset(&blockmask);

	for(i=SIGHUP; i<32; i++){		
		//sigaddset(&blockmask,i) ;
		signal(i,SIG_IGN) ;
	}
	signal(i,SIG_DFL) ;
	//sigprocmask(SIG_BLOCK ,&blockmask, NULL );

	return 0;
}
NEINT32 unblock_signal(void)
{
	NEINT32 i ;
	sigset_t blockmask ;

	/*mask all signal */
	sigemptyset(&blockmask);

	for(i=SIGHUP; i<32; i++){
		if(	(NEINT32)SIGALRM==i ||
			(NEINT32)SIGPROF==i ||
			(NEINT32)SIGFPE==i  ||
			(NEINT32)SIGKILL==i )
			continue ;
		sigaddset(&blockmask,i) ;
	}
	sigprocmask(SIG_UNBLOCK ,&blockmask, NULL );

	return 0;
}
/*�õ����յ����ź�*/
NEINT8 *get_signal_desc(NEINT32 signo)
{
	NEINT8 *p = "undefined message " ;
	if( signo <= 31 && signo >=0)
		return sig_desc[signo] ;
	else 
		return p ;
}
#endif
