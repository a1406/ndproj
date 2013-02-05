#include "ne_common/ne_common.h"
#ifdef WIN32
#include <tchar.h>


//如果错误弹出错误提示窗口
DWORD _ErrBox(NEINT8 *file, NEINT32 line)
{
	const NEINT8 *perrdesc=ne_last_error() ;
	NEINT8 buf[4096]={0} ;
	snprintf(buf,4096,"Runtime ERROR in:%s %d line\n %s ",
		file,line,perrdesc);
	MessageBoxA(GetActiveWindow(), buf, "error", MB_OK);
	DebugBreak();
	return 0;
}
const NEINT8 *ne_last_error()
{
	// Get the error code
	HLOCAL hlocal = NULL;   // Buffer that gets the error message string
	DWORD dwError = GetLastError() ;
	NEINT8 *pstrNoErr ="Error number not found." ;
	static NEINT8 buf_desc[1024] ;
	BOOL fOk ;

	if(0==dwError) {
		strncpy(buf_desc,pstrNoErr,sizeof(buf_desc));
		return buf_desc ;
	}
	// Get the error code's textual description
	fOk = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
		NULL, dwError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
		(PTSTR) &hlocal, 0, NULL);

	if (!fOk) {
		// Is it a network-related error?
		HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL, 
		DONT_RESOLVE_DLL_REFERENCES);

		if (hDll != NULL) {
			FormatMessage(
			FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
			hDll, dwError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
			(PTSTR) &hlocal, 0, NULL);
			FreeLibrary(hDll);
		}
	}

	if (hlocal != NULL) {
		strncpy(buf_desc,(NEINT8 *) LocalLock(hlocal),sizeof(buf_desc));
		LocalFree(hlocal);
	} else {
		strncpy(buf_desc,pstrNoErr,sizeof(buf_desc));
	}
	return buf_desc ;
}

#ifdef _DEBUG
NEINT32 MyDbgReport(NEINT8 *file, NEINT32 line, NEINT8 *stm, ...)
{
	NEINT8 buf[1024], *p = buf ;
	va_list arg;
	NEINT32 done;

	if(file) {
		sprintf(p, "%s ", file) ;
		p += strlen(p) ;
	}
	if(line) {
		sprintf(p, "%d ", line) ;
		p += strlen(p) ;
	}

	va_start (arg, stm);
	done = vsprintf (p, stm, arg);
	va_end (arg);

	_CRTTRAC(buf) ;
	return 0;
}
#endif

// 0=multi-CPU, 1=single-CPU, -1=not set yet
static NEINT32 s_multiProcess = -1 ;
static NEINT32 s_spinCount = 0 ;

NEINT32 initNDMutex(NDMutex *m) 
{	
	if(-1==s_multiProcess) 
	{
		SYSTEM_INFO sinf;
		GetSystemInfo(&sinf);
		s_spinCount = sinf.dwNumberOfProcessors ;
		s_multiProcess = (s_spinCount == 1);
	}
	memset(m, 0 , sizeof(*m)) ;
	m->hSig = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(NULL == m->hSig ) 
	{
		return -1 ;
	}
	
	if(0==s_multiProcess)
	{
		InterlockedExchange((LPLONG) &m->_spinCount,  (LONG) s_spinCount);
    }
    
	return 0;
}

//return vlues : 0(true) scuurss , -1(false) failed 
NEINT32 tryEntryMutex(NDMutex *m)
{
	DWORD selfID = GetCurrentThreadId() ;
	NEINT32 spin = m->_spinCount ;
	BOOL owned = FALSE;
	
	do { 
		//owned = (0==InterlockedCompareExchange((&m->lockCount), (neatomic_t) 1, (neatomic_t)0));		
		owned = (ne_compare_swap((&m->lockCount), (neatomic_t)0, (neatomic_t) 1));
		if(owned )
		{
			//enter success 
			m->ownerID = selfID ;
			m->used = 1 ;
		}
		else 
		{
			if(m->ownerID == selfID )
			{
				++(m->used);
				InterlockedIncrement(&m->lockCount) ;
				owned = TRUE ;
			}
		}
		
	} while(!owned && spin-- > 0 ) ;		
	return (owned?0:-1) ;
}

void entryMutex(NDMutex *m)
{
	DWORD selfID ;
	if(0==tryEntryMutex(m))
	{
		//LOCK SUCCESS
		return ;
	}
	
	selfID = GetCurrentThreadId() ;
	if(1==InterlockedIncrement(&m->lockCount) )
	{
		//entry success 
		m->ownerID = selfID ;
		m->used = 1 ;
	}
	else 
	{
		if(m->ownerID == selfID)
		{
			++(m->used) ;
		}
		else 
		{
			WaitForSingleObject(m->hSig , INFINITE);
			//yes I can get 
			//其实这里还需要重新测试条件,
			//不过所有新来的竞争者如果没有得到资源,都需要通过WaitForSingleObject
			//所以,不需要重新测试条件也可以
			m->ownerID = selfID ;
			m->used = 1 ;
		}
	}
}

void leaveMutex(NDMutex *m)
{
	ne_assert(m->ownerID==GetCurrentThreadId()) ;
	if(--(m->used) > 0 )
	{
		//alread owned by myslef 
		InterlockedDecrement(&m->lockCount) ; 
	}
	else 
	{
		m->ownerID = 0 ;
		if(InterlockedDecrement(&m->lockCount) > 0)
		{
			SetEvent(m->hSig) ;
		}
	}
}

//destory 
void destoryMutex(NDMutex *m)
{
	CloseHandle(m->hSig);
}

NEINT32 initNDCondVar(NDCondVar *v) 
{
	memset(v, 0 , sizeof(*v)) ;
	v->hSig = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(NULL == v->hSig ) {
		return -1 ;
	}
	return 0 ;
}

NEINT32 waitCondVar(NDCondVar *v , NDMutex *m)
{
	NEINT32 hr ;
	DWORD self = GetCurrentThreadId() ;
	
	if(m->ownerID != self) 
		return -1 ;
	
	InterlockedIncrement(&v->lockCount) ;
	leaveMutex(m) ;
	
	hr = WaitForSingleObject(v->hSig , INFINITE) ;	
	hr = (WAIT_OBJECT_0==hr)? 0:-1 ;
	
	//当broadcast的时候,这里可能会出现多个消费者竞争
	//不是一个很好的解决办法
	entryMutex(m) ;
	return (NEINT32)hr ;
}

NEINT32 timewaitCondVar(NDCondVar *v, NDMutex *m, DWORD mseconds)
{
	NEINT32 hr ;
	DWORD self = GetCurrentThreadId() ;
	
	if(m->ownerID != self) 
		return -1 ;
	if(0==mseconds) 
	{
		return 0 ;
	}
	
	InterlockedIncrement(&v->lockCount) ;
	leaveMutex(m) ;
	
	hr = WaitForSingleObject(v->hSig , mseconds);	
//	InterlockedDecrement(&v->lockCount) ;
	hr = (WAIT_OBJECT_0==hr)? 0:-1 ;
	
	entryMutex(m) ;
	return (NEINT32)hr;
}

NEINT32 signalCondVar(NDCondVar *v)
{
	neatomic_t oldval ;
	while( (oldval=ne_atomic_read(&v->lockCount)) > 0 ){
		//if(InterlockedCompareExchange((&v->lockCount),(oldval-1),oldval) > 0 )	{
		if(ne_compare_swap((&v->lockCount),oldval,(oldval-1)) )	{
		
			SetEvent(v->hSig) ;
			break ;
		}
	}
	return 0 ;
}

NEINT32 broadcastCondVar(NDCondVar *v)
{
	neatomic_t oldval ;
	while( (oldval=ne_atomic_read(&v->lockCount)) > 0 ){
		//if(InterlockedCompareExchange((&v->lockCount),(oldval-1),oldval) > 0 )	{
		if(ne_compare_swap((&v->lockCount),oldval,(oldval-1)) )	{
			SetEvent(v->hSig) ;
		}
	}
	return 0 ;
	/*NOTE!!!
	 *这个实现并不好,可能会有问题
	 *但现在我还没有别的好办法
	 */
/*	register NEINT32 oldval = v->lockCount ;
	oldval =(NEINT32) InterlockedCompareExchange((LPVOID*)(&v->lockCount),(LPVOID)oldval,(LPVOID)oldval)  ;
	while(oldval-->0)
	{
		SetEvent(v->hSig) ;
	}
	*/
	return 0 ;
}
NEINT32 destoryCondVar(NDCondVar *v)
{
	CloseHandle(v->hSig);
	return 0 ;
}


#pragma comment (lib, "Winmm.lib") 
netime_t ne_time(void) 
{
	return timeGetTime() ;
}
//create thread 

WINBASEAPI BOOL WINAPI SwitchToThread(VOID );
/*create thread function
 * return thread handle 
 * if return 0 or NULL failed .
 * but in linux success return 1 else return 0

 *@func : thread function 
 *@param: thread function parameter
 *@thid : output thread id
 */

neth_handle ne_createthread(NETH_FUNC func, void* param, nethread_t *thid,NEINT32 priority)
{
	DWORD dwID = 0 ;
	HANDLE h = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, param, 0 ,&dwID) ;
	if(h && thid) {
		*thid = dwID ;				
		if(priority==NET_PRIORITY_HIGHT) {
			SetThreadPriority(h,THREAD_PRIORITY_ABOVE_NORMAL) ;
		}
		else if(NET_PRIORITY_LOW) {
			SetThreadPriority(h,THREAD_PRIORITY_BELOW_NORMAL) ;
		}
	}
	return h ;
}
NEINT32 ne_threadsched(void) 
{
	BOOL ret = SwitchToThread() ;
	return ret ? 0:-1 ;
}

void ne_threadexit(NEINT32 exitcode)
{
	ExitThread(exitcode) ;
}
//等待一个线程的结束
NEINT32 ne_waitthread(neth_handle handle) 
{
	DWORD dwRet = WaitForSingleObject(handle,INFINITE) ;
	if(dwRet==WAIT_OBJECT_0) {
		return 0 ;
	}
	else {
		return -1 ;
	}
}

NEINT32 ne_terminal_thread(neth_handle handle,NEINT32 exit_code)
{
	BOOL ret = TerminateThread(handle,(DWORD)exit_code) ;
	return ret?0:-1;
}

#endif
	
