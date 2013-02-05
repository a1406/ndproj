#ifndef _NE_ATOMIC_H_
#define _NE_ATOMIC_H_

/*
 atomic operation type : neatomic_t 

//atomic operate
NEINT32 ne_compare_swap(neatomic_t *desc,neatomic_t cmp, neatomic_t exch)
{
	neatomic_t tmp = *desc ;
	if(*desc==cmp) {
		*desc = exch ;
		return 1 ;
	}
	
	return 0 ;
}
		
neatomic_t ne_atomic_inc(neatomic_t *p)
{
	++(*p ) ;
	return *p ;
}
neatomic_t ne_atomic_dec(neatomic_t *p) 
{
	--(*p ) ;
	return *p ;
}	
neatomic_t ne_atomic_add(neatomic_t *p,NEINT32 step) 
{
	neatomic_t tmp = *desc ;
	
	*desc += step ;
	
	return tmp ;
}
neatomic_t ne_atomic_sub(neatomic_t *p,NEINT32 step)
{
	neatomic_t tmp = *desc ;
	
	*desc -= step ;
	
	return tmp ;
}
NEINT32  ne_atomic_swap(neatomic_t *p, neatomic_t val)
{

	neatomic_t tmp = *desc ;
	
	*desc = val ;
	
	return tmp ;

}

NEINT32 ne_testandset(neatomic_t *p)
{
	NEINT32 old = (NEINT32)*p ;
	if(p==0) 
		p =1 ;
	return old;
}
	
void ne_atomic_set(neatomic_t* p, neatomic_t val);
void ne_atomic_read(neatomic_t *p);
  */
#if defined(WIN32 )

#if _MSC_VER < 1300 // 1200 == VC++ 6.0

typedef LONG neatomic_t ;
#else 
typedef  LONG neatomic_t ;
#endif

//atomic operate
__INLINE__ NEINT32 ne_compare_swap(neatomic_t *lpDest,neatomic_t lComp,neatomic_t lExchange)
{
	//neatomic_t last = *lpDest ;
//#ifdef InterlockedCompareExchangePointer
//	return (InterlockedCompareExchange(lpDest, lExchange,lComp)==last);
//#else 
#if _MSC_VER < 1300 // 1200 == VC++ 6.0
	return (NEINT32) (lComp==(LONG)InterlockedCompareExchange((PVOID*)lpDest, (PVOID)lExchange,(PVOID)lComp) );
#else 
	return (NEINT32) (lComp==(LONG)InterlockedCompareExchange(lpDest, lExchange, lComp) );
#endif
//#endif
}
#define ne_atomic_inc(p)	InterlockedIncrement(p) 
#define ne_atomic_dec(p)	InterlockedDecrement(p) 
#define ne_atomic_add(p,step)	InterlockedExchangeAdd((NEINT32*)(p),step) 
#define ne_atomic_sub(p,step)	InterlockedExchangeAdd((NEINT32*)(p),-(step))
#define ne_atomic_swap(p, val)	InterlockedExchange(p,val) 
#define ne_testandset(p)	InterlockedExchange(p,1) 

#define ne_atomic_set(p, val)    InterlockedExchange((p), val)
#define ne_atomic_read(p)        (*(p))

#elif defined(__LINUX__)

/*
#include <asm/atomic.h>
#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif
/*/
#define LOCK "lock ; "
/**/
typedef NEINT32 neatomic_t ;
static __INLINE__ NEINT32 ne_testandset (neatomic_t  *p)
{
  NEINT32 ret;

  __asm__ __volatile__(
       "xchgl %0, %1"
       : "=r"(ret), "=m"(*p)
       : "0"(1), "m"(*p)
       : "memory");

  return ret;
}
static __INLINE__ NEINT32 ne_compare_swap (neatomic_t *p, neatomic_t cmpval, neatomic_t exchval)
{
	NEINT8 ret;
	neatomic_t readval;
	__asm__ __volatile__ (LOCK" cmpxchgl %3, %1; sete %0"
		: "=q" (ret), "=m" (*p), "=a" (readval)
		: "r" (exchval), "m" (*p), "a" (cmpval)
		: "memory");
	return ret;
}

/* Atomically swap memory location [32 bits] with `newval'*/
/*
	ret = *p ;
	*p = newval ;
	return ret ;
*/

static __INLINE__ neatomic_t  ne_atomic_swap(neatomic_t * ptr, neatomic_t x )
{
	__asm__ __volatile__("xchgl %0,%1"
			:"=r" (x)
			:"m" (*ptr), "0" (x)
			:"memory");
		
	return x;
}

/* (*val)++ 
	return *val;
 */
static __INLINE__ NEINT32 ne_atomic_inc(NEINT32 *val)
{
	register NEINT32 oldval;
	do {
		oldval = *val ;
	}while (!ne_compare_swap(val, oldval, oldval+1)) ;
	return oldval+1 ;
}

/* (*val)++
	return *val;
 */
static __INLINE__ NEINT32 ne_atomic_dec(NEINT32 *val)
{
	register NEINT32 oldval;
	do {
		oldval = *val ;
	}while (!ne_compare_swap(val, oldval, oldval-1)) ;
	return oldval-1 ;
}

/* (*val)+= nstep 
	return *val;
 */
static __INLINE__ NEINT32 ne_atomic_add(NEINT32 *val, NEINT32 nstep)
{
	register NEINT32 oldval;
	do {
		oldval = *val ;
	}while (!ne_compare_swap(val, oldval, oldval+nstep)) ;
	return oldval;
}

/* (*val)-=nstep
	return *val;
 */
static __INLINE__ NEINT32 ne_atomic_sub(NEINT32 *val, NEINT32 nstep)
{
	register NEINT32 oldval;
	do {
		oldval = *val ;
	}while (!ne_compare_swap(val, oldval, oldval-nstep)) ;
	return oldval;
}

#define ne_atomic_set(p, val)    ne_atomic_swap(p, val)
#define ne_atomic_read(p)        (*(p))

#elif defined(__BSD__)

typedef NEINT32 neatomic_t ;

#include <sys/types.h>
#include <machine/atomic.h>
//#define ne_atomic_swap(p, val) (atomic_store_rel_int(p,val),*(p)) 
static inline NEINT32 ne_testandset(volatile NEINT32 *p) {return !atomic_cmpset_rel_int(p,0,1);}
#define ne_compare_swap(p,compare,exchange) atomic_cmpset_rel_int(p,compare,exchange)
static inline NEINT32 ne_atomic_swap(volatile neatomic_t *p ,neatomic_t exch)
{
	register NEINT32 oldval;
	do {
		oldval = *p ;
	}while (!atomic_cmpset_rel_int(p, oldval, exch)) ;
	return oldval ;
}
#define ne_atomic_add(p, val)  atomic_fetchadd_int(p, val)
#define ne_atomic_sub(p,val)  atomic_subtract_int(p, val)

#define ne_atomic_inc(p) (atomic_add_rel_int(p,1),*(p))
#define ne_atomic_dec(p) (atomic_subtract_rel_int(p,1),*(p))

#define ne_atomic_set(p, val)    atomic_set_int(p, val)
#define ne_atomic_read(p)        (*(p))

#else 
#error unknow platform!
#endif

#endif
