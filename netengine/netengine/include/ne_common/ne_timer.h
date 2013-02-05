#ifndef _NE_TIMER_H_
#define _NE_TIMER_H_

typedef NEUINT32 netimer_t ;
// return -1 to cancel the timer
typedef NEINT32 (*ne_timer_entry)(netimer_t timer_id, void *param) ;

enum eTimeType
{
	ETT_LOOP = 0 ,			//循环执行的定时器
	ETT_ONCE 				//执行一次的定时器
};

/* 增加一个计时执行函数*/
NE_COMMON_API netimer_t ne_timer_add(ne_handle handle,
	ne_timer_entry func, void *param,
	netimer_t interval, NEINT32 run_type );
NE_COMMON_API NEINT32 ne_timer_del(ne_handle handle, netimer_t id) ;

NE_COMMON_API NEINT32 ne_timer_destroy(ne_handle timer_handle, NEINT32 force) ;
/* create timer root */
NE_COMMON_API ne_handle ne_timer_create(ne_handle pallocator) ;
NE_COMMON_API NEINT32 ne_timer_update(ne_handle handle)  ;
#endif
