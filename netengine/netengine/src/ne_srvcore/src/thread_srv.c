//#define TEST_MYMM 1	

#ifndef WIN32 
#define USER_P_V_LOCKMSG		1		
#endif 

#include "ne_common/ne_common.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_common/ne_alloc.h"

/* thread contex*/
typedef struct ne_threadsrv_context{
	NEUINT32 size ;				
	NEUINT32 type ;				
	NEUINT32	myerrno;
	ne_close_callback close_entry ;	

	ne_thsrvid_t  thid ;				//server id
	neth_handle th_handle ;				//thread handle
	ne_handle	allocator ;				//memory allocator 
	neatomic_t __exit ;					//if nonzero eixt 
	NEINT32 run_module ;					//srv_entry run module (ref e_subsrv_runmod)
	ne_threadsrv_entry srv_entry ;		//service entry
	void *srv_param ;					//param of srv_entry 
	ne_thsrvmsg_func msg_entry ;				//handle received message from other thread!
	ne_threadsrv_clean cleanup_entry;			//clean up when server is terminal
	ne_mutex msg_lock ;
#ifdef USER_P_V_LOCKMSG
	ne_cond	 msg_cond ;				
#endif
	struct list_head list;				//self list
	struct list_head msg_list ;
	void *_user_data ;					//user data	
	neatomic_t is_suspend;				//0 run 1 suspend
	ndsem_t sem_suspend ;					//received start signal
	ne_handle h_timer ;					//handle of timer 
	nechar_t srv_name[NE_SRV_NAME] ;	//service name
}ne_thsrv_context_t;

struct thsrv_create_param
{
	ne_thsrv_context_t *pcontext ;
	ndsem_t				sem ;

} ;

/*all service module contex*/
static struct ne_srv_entry
{
	NEINT32					status ;
	neatomic_t			__exit ;		//1 exit 
	struct list_head	th_context_list ;
}__s_entry;

struct ne_srv_entry *get_srv_entry() ;

NEINT32 _destroy_service(ne_thsrv_context_t *contex, NEINT32 wait) ;

struct ne_srv_entry *get_srv_entry() 
{
	return &__s_entry;
}

static	void _init_entry_context()
{
	if(1==__s_entry.status)
		return ;
	INIT_LIST_HEAD(&(__s_entry.th_context_list)) ;
	ne_atomic_set(&__s_entry.__exit, 0) ;
	__s_entry.status = 1 ;
}

void ne_host_eixt() 
{
	ne_atomic_swap(&(__s_entry.__exit),1) ;
}

NEINT32 ne_host_check_exit() 
{
	return ne_atomic_read(&(__s_entry.__exit ));
}

NEINT32 ne_thsrv_check_exit(ne_thsrvid_t srv_id)
{
	ne_thsrv_context_t *contex =(ne_thsrv_context_t *)ne_thsrv_gethandle(srv_id) ;
	if(!contex) 
		return 0 ;
	return (ne_host_check_exit() || ne_atomic_read(&(contex->__exit)) );
}

NEINT32 ne_thsrv_isexit(ne_handle h_srvth )
{
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)h_srvth ;
	if(!contex) {
		contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(0) ;
		if(!contex)
			return 0 ;
	}
	return (ne_atomic_read(&(__s_entry.__exit )) || ne_atomic_read(&(contex->__exit)) );

}

static NEINT32 _msg_th_func(ne_thsrv_context_t *contex)
{
	NEINT32 ret = 0;
	struct ne_thread_msg *node ;
	struct list_head *pos, *next;
	struct list_head header ;

	ne_mutex_lock(&contex->msg_lock) ;
#ifdef USER_P_V_LOCKMSG
    while(list_empty(&contex->msg_list) ) {
        ne_cond_wait(&contex->msg_cond,&contex->msg_lock) ;
        if(ne_atomic_read(&contex->__exit) || ne_atomic_read(&__s_entry.__exit)) {
            ne_mutex_unlock(&contex->msg_lock) ;
            return -1 ;
        }
    }
#endif
    if(list_empty(&contex->msg_list)) {
        ne_mutex_unlock(&contex->msg_lock) ;
        return 0 ;
    }
    list_add(&header, &contex->msg_list);
    list_del_init(&contex->msg_list) ;
	ne_mutex_unlock(&contex->msg_lock) ;
	
	if(contex->msg_entry ) {
		pos = header.next ;
		while(pos !=  &header){
			NEINT32 hr ;
			next = pos->next ;
			node = list_entry(pos, struct ne_thread_msg, list) ;
			hr = contex->msg_entry(node) ;			
			pos = next ;
			
#ifdef TEST_MYMM 
			free(node) ;
#else 
			ne_pool_free(contex->allocator, node) ;
#endif
			if(-1==hr)
				return -1 ;
			++ret ;
		}
	}
	return ret ;
}
//received message function entry
static NEINT32 _msg_entry(ne_thsrv_context_t *contex)
{
	struct ne_thread_msg *node ;
	struct list_head *pos, *next;
	struct list_head header ;

	if(list_empty(&contex->msg_list) )
		return 0 ;
	ne_mutex_lock(&contex->msg_lock) ;
    list_add(&header, &contex->msg_list);
    list_del_init(&contex->msg_list) ;
	ne_mutex_unlock(&contex->msg_lock) ;
	
	if(contex->msg_entry ) {
		pos = header.next ;
		while(pos !=  &header){
			next = pos->next ;
			node = list_entry(pos, struct ne_thread_msg, list) ;
			contex->msg_entry(node) ;
			pos = next ;
#ifdef TEST_MYMM 
			free(node) ;
#else 
			ne_pool_free(contex->allocator, node) ;
#endif
		}
	}
	return 0 ;
}

/* sub service entry */
static void *_srv_entry(void *p)
{
	NEINT32 ret = 0;
	ne_thsrv_context_t * contex;
	
	struct thsrv_create_param *create_param = (struct thsrv_create_param *) p ;
	if(!p)
		return (void*)-1 ;
	ne_sleep(100) ;
	
	contex = create_param->pcontext ;
        //sem_suspend
	ne_assert(contex) ;
	ne_sem_post(create_param->sem) ;

	ne_sem_wait(contex->sem_suspend,-1) ;		//wait received run command

    neprintf("*** %s server start\n", contex->srv_name) ;
	if(SUBSRV_RUNMOD_LOOP == contex->run_module) {
		while(0==ne_atomic_read(&contex->__exit) && 0==ne_atomic_read(&__s_entry.__exit)) {
			if(-1==contex->srv_entry(contex->srv_param) ) {
				ne_atomic_set(&contex->__exit,1) ;
				ret = -1 ;
				goto	EXIT_SRV ;
			}
			_msg_entry(contex) ;
			if(contex->h_timer) {
				ne_timer_update(contex->h_timer) ;
			}
                //try to timer
//			if(!list_empty(&(contex->timer_list.__head))){
//				ne_timer_handle(contex) ;
//			}
			ret = 0 ;
			while(ne_atomic_read(&contex->is_suspend)>0) {
				ne_sem_wait(contex->sem_suspend,-1) ;
				++ret ;
			}
			if(0==ret)
				ne_sleep(10) ;

			ret = 0 ;
		}	//end while
		
		
	}
	else if (SUBSRV_RUNMOD_STARTUP == contex->run_module){
		ret = contex->srv_entry(contex->srv_param) ;
	}
	else if(SUBSRV_MESSAGE == contex->run_module) {		
		while(0==ne_atomic_read(&contex->__exit) && 0==ne_atomic_read(&__s_entry.__exit)) {	
			NEINT32 hr = _msg_th_func(contex) ;
			if(contex->h_timer) {
				ne_timer_update(contex->h_timer) ;
			}
#ifndef USER_P_V_LOCKMSG
			if(-1== hr )
				break ;
			else if(0==hr) {
				ne_atomic_inc(&contex->is_suspend) ;
				ne_sem_wait(contex->sem_suspend,100) ;
			}
#endif
		}
#ifdef USER_P_V_LOCKMSG
		ne_cond_destroy(&contex->msg_cond) ;
#endif
	}
EXIT_SRV:
	if(contex->cleanup_entry){
		contex->cleanup_entry() ;
		contex->cleanup_entry = NULL ;		//clean up once
	}
        //wait for destroy thread server
    neprintf("%s server is exit\n", contex->srv_name) ;
//	ne_threadexit(ret) ;
	return (void*)ret ;
}

/*create a service */
ne_thsrvid_t ne_thsrv_createex(struct ne_thsrv_createinfo* create_info,NEINT32 priority, NEINT32 suspend )
{	
	struct thsrv_create_param create_param ;
	ne_thsrv_context_t * contex;
	_init_entry_context () ;
	if(!create_info )
		return 0;
	if(create_info->run_module!=SUBSRV_MESSAGE && NULL==create_info->srv_entry) {
		return 0 ;
	}
	contex = malloc(sizeof (ne_thsrv_context_t) ) ;
	if(!contex)
		return 0 ;
	
	create_param.pcontext = contex ;	
	if(-1==ne_sem_init(create_param.sem)) {
		free(contex) ;
		ne_logfatal("create thread server error for sem init error=%s !\n" AND ne_last_error()) ;
		return 0 ;
	}

	if(-1==ne_sem_init(contex->sem_suspend)) {
		ne_sem_destroy(create_param.sem) ;
		free(contex) ;
		ne_logfatal("create thread server error for sem suspend init error=%s !\n" AND ne_last_error()) ;
		return 0 ;
	}
	contex->size = sizeof(ne_thsrv_context_t) ;
	contex->type = ('t'<< 8) | 'h' ;
	contex->close_entry = (ne_close_callback )_destroy_service;

	contex->thid = 0  ;
	contex->th_handle = 0 ;			//thread handle
	contex->myerrno = 0 ;
	ne_atomic_set(&contex->__exit,0);
	contex->srv_entry = create_info->srv_entry ;
	contex->srv_param = create_info->srv_param ;
	contex->msg_entry = create_info->msg_entry ;
	contex->cleanup_entry = create_info->cleanup_entry;
	contex->_user_data = create_info->data ;
	contex->run_module = create_info->run_module ;
	contex->h_timer = NULL ;
	ne_atomic_swap(&contex->is_suspend ,1) ;
	nestrncpy(contex->srv_name,create_info->srv_name,sizeof(contex->srv_name));

	INIT_LIST_HEAD(&(contex->list)) ;
	INIT_LIST_HEAD(&(contex->msg_list)) ;
	ne_mutex_init(&(contex->msg_lock)) ;

#ifdef USER_P_V_LOCKMSG
	if(SUBSRV_MESSAGE==create_info->run_module){
		if(-1==ne_cond_init(&contex->msg_cond)  ) {
			ne_logfatal("init cond error in message thread!\n") ;
			return -1 ;
		}
            //contex->allocator = ne_pool_create(EMEMPOOL_UNLIMIT) ;
	}
        //else 
#else 
#endif
	
	contex->allocator = ne_pool_create(EMEMPOOL_HUGE*128) ;
	if(NULL==contex->allocator) {
		contex->allocator = ne_global_mmpool() ;
	}

        //create_param 
	contex->th_handle = ne_createthread(_srv_entry, &create_param, &(contex->thid),priority) ;
	if(!contex-> th_handle ){
		ne_sem_destroy(create_param.sem) ;
		free(contex) ;
		return 0;
	}
	list_add(&(contex->list), &(__s_entry.th_context_list)) ;
	ne_sem_wait(create_param.sem, -1) ;
    neprintf("%s server create success id=0x%x!\n", contex->srv_name, (NEINT32) contex->th_handle) ;	
	ne_sem_destroy(create_param.sem) ;

	if(0==suspend) {
		ne_atomic_swap(&contex->is_suspend ,0) ;
		ne_sem_post(contex->sem_suspend);
	}
	return contex->thid ;
}

NEINT32 ne_thsrv_suspend(ne_thsrvid_t  srv_id) 
{
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(srv_id ) ;
	if(!contex || SUBSRV_MESSAGE==contex->run_module){
            //ne_assert(0) ;
		return -1;
	}
	ne_atomic_inc(&contex->is_suspend) ;
	return 0 ;
}

NEINT32 ne_thsrv_resume(ne_thsrvid_t  srv_id) 
{
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(srv_id ) ;
	if(!contex||SUBSRV_MESSAGE==contex->run_module)
		return -1;
	ne_atomic_dec(&contex->is_suspend) ;
	ne_sem_post(contex->sem_suspend) ;
	return 0 ;
}

NEINT32 _destroy_service(ne_thsrv_context_t *contex, NEINT32 wait)
{
	NEINT32 ret = 0;
	struct list_head *pos;
	
	ne_assert(contex);

	ne_atomic_swap(&contex->__exit,1);
#ifdef USER_P_V_LOCKMSG
	if(SUBSRV_MESSAGE==contex->run_module) {
		ne_mutex_lock(&contex->msg_lock) ;
        ne_cond_signal(&contex->msg_cond) ;
		ne_mutex_unlock(&contex->msg_lock) ;
	}
	else 
#endif 
	{
		while(ne_atomic_read(&contex->is_suspend)>0) {
			ne_atomic_swap(&contex->is_suspend,0) ;
			ne_sem_post(contex->sem_suspend) ;
		}
	}

	if(contex->th_handle && wait){
		neprintf("waiting for %s id=%d\n", contex->srv_name, (NEINT32)contex->th_handle) ;
		ret = ne_waitthread(contex->th_handle) ;
		
		neprintf("success waiting for %s success ret =%d\n", contex->srv_name, ret) ;
	}
	list_del(&(contex->list)) ;

        //destroy timer 
	if(contex->h_timer) {
		ne_timer_destroy(contex->h_timer, wait) ;
		contex->h_timer = 0 ;
	}
        //destroy message context
	
	pos = contex->msg_list.next ;
	while(pos !=  &contex->msg_list){
		struct ne_thread_msg *node = list_entry(pos, struct ne_thread_msg, list) ;
		pos = pos->next ;
		ne_pool_free(contex->allocator, node) ;
	}

	if(contex->cleanup_entry)
		contex->cleanup_entry() ;
	ne_sem_destroy(contex->sem_suspend) ;

	ne_mutex_destroy(&contex->msg_lock) ;
	if(contex->allocator) {
		ne_handle h_alloc = ne_global_mmpool() ;
		if(h_alloc!=contex->allocator) {
			ne_pool_destroy(contex->allocator, 0) ;
		}
		contex->allocator = 0 ;
	}
	free(contex) ;
	return ret ;
}

NEINT32 ne_thsrv_destroy(ne_thsrvid_t srvid,NEINT32 force)
{
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(srvid ) ;
	if(!contex)
		return -1;
	
	return _destroy_service(contex, !force);
}

//find service context
ne_handle ne_thsrv_gethandle(ne_thsrvid_t srvid )
{
	struct list_head *pos, *next ;
	ne_thsrv_context_t *context ;
	struct ne_srv_entry *srventry ;
	if(0==srvid) {
		srvid = ne_thread_self() ;
	}

	srventry = get_srv_entry()  ;
	pos = srventry->th_context_list.next ;
	while(pos != &(srventry->th_context_list)) {
		next = pos->next ;
		context = list_entry(pos,ne_thsrv_context_t,list) ;
		if(ne_thread_equal(context->thid,srvid))
			return (ne_handle)context ;
		pos = next ;
	}
	return NULL ;
}

ne_thsrvid_t ne_thsrv_getid(ne_handle handle)
{
	return ((ne_thsrv_context_t *)handle)->thid ;
}

void ne_thsrv_release_all() 
{
	struct list_head *pos ,*next;
	ne_thsrv_context_t *node ;
	struct ne_srv_entry *entry = get_srv_entry() ;

//	ne_host_eixt() ;

    neprintf("release all thread server\n") ;
	pos = entry->th_context_list.next ;
	while(pos !=&(entry->th_context_list))  {
		next = pos->next ;
		node = list_entry(pos,ne_thsrv_context_t,list) ;
		_destroy_service(node,1) ;
		pos = next ;
	}
	__s_entry.status = 0 ;
	INIT_LIST_HEAD(&(__s_entry.th_context_list)) ;
}


NEINT32 ne_thsrv_send(ne_thsrvid_t srvid,NEUINT32 msgid,void *data, NEUINT32 data_len) 
{
	size_t size = data_len + sizeof(struct ne_thread_msg );
	struct ne_thread_msg *msg_addr;	
	
	ne_thsrv_context_t *contex =(ne_thsrv_context_t *) ne_thsrv_gethandle(srvid) ;
	if(!contex) {
		ne_assert(contex) ;
		return -1 ;
	}
#ifdef TEST_MYMM 
	msg_addr = malloc(contex->allocator, size) ; //(struct ne_thread_msg *)malloc(size) ;
#else 
	msg_addr = ne_pool_alloc(contex->allocator, size) ; //(struct ne_thread_msg *)malloc(size) ;
#endif 
	ne_assert(msg_addr) ;
	if(!msg_addr)
		return -1 ;
	msg_addr->data_len = (NEUINT16)data_len ;
	INIT_LIST_HEAD(&(msg_addr->list)) ;


	msg_addr->msg_id = msgid;
	msg_addr->from_id = ne_thread_self();
	msg_addr->recv_id = srvid;
	msg_addr->th_userdata = contex->_user_data ;

	if(data_len>0 && data) {
		memcpy(msg_addr->data, data, data_len) ;
	}

#ifdef USER_P_V_LOCKMSG
	if(SUBSRV_MESSAGE==contex->run_module) {
		ne_mutex_lock(&contex->msg_lock) ;
        list_add_tail(&msg_addr->list, &contex->msg_list);
        ne_cond_signal(&contex->msg_cond) ;
		ne_mutex_unlock(&contex->msg_lock) ;
		return 0 ;
	}
	else 
#endif
	{
		ne_mutex_lock(&contex->msg_lock) ;
        list_add_tail(&msg_addr->list, &contex->msg_list);
		ne_mutex_unlock(&contex->msg_lock) ;
	}
#ifndef USER_P_V_LOCKMSG
	if(ne_atomic_read(&contex->is_suspend)>0) {
		ne_atomic_swap(&contex->is_suspend,0) ;	
		ne_sem_post(contex->sem_suspend) ;
	}
	else 
		ne_sem_post(contex->sem_suspend) ;
#endif
	return 0 ;
}

NEINT32 ne_thsrv_msghandler(ne_handle srv_handle) 
{
	ne_thsrv_context_t *current_contex = (ne_thsrv_context_t *)srv_handle;
	if(!current_contex)
		current_contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(0) ;
	
	ne_assert(current_contex);
	ne_assert(ne_thread_equal(current_contex->thid, ne_thread_self())) ;	
	ne_assert(SUBSRV_RUNMOD_STARTUP==current_contex->run_module) ;
	
	if(ne_atomic_read(&(current_contex->__exit)) || ne_atomic_read(&(__s_entry.__exit))){
		return 0 ;
	}
	_msg_entry(current_contex) ;
	if(current_contex->h_timer) {
		ne_timer_update(current_contex->h_timer) ;
	}
	while(ne_atomic_read(&current_contex->is_suspend)>0) {
		ne_sem_wait(current_contex->sem_suspend,-1) ;
	}
	return 1 ;
}

//terminal a service
NEINT32 ne_thsrv_end(ne_thsrvid_t  srv_id) 
{
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(srv_id) ;
	if(contex) {
		ne_atomic_swap(&(contex->__exit),1) ;
		return 0 ;
	}
	return -1 ;
}

NEINT32 ne_thsrv_wait(ne_thsrvid_t  srv_id)
{
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(srv_id) ;
	if(contex) {
		NEINT32 ret = ne_waitthread(contex->th_handle) ;
		ne_close_handle(contex->th_handle);
	}
	return -1 ;
}

ne_handle ne_thsrv_local_mempool(ne_handle  thsrv_handle) 
{
	return ((ne_thsrv_context_t*)thsrv_handle)->allocator ;
}

netimer_t ne_thsrv_timer(ne_thsrvid_t srv_id,ne_timer_entry func,void *param,netime_t interval, NEINT32 run_type )
{	
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(srv_id) ;
	if(!contex) {
		return 0 ;
	}
	if(!contex->h_timer) {
		contex->h_timer = ne_timer_create(contex->allocator) ;
		if(!contex->h_timer)
			return 0 ;
	}
	return ne_timer_add(contex->h_timer, func, param, interval, run_type );
}

void ne_thsrv_del_timer(ne_thsrvid_t srv_id, netimer_t timer_id )
{
	ne_thsrv_context_t *contex = (ne_thsrv_context_t *)ne_thsrv_gethandle(srv_id) ;
	if(!contex || !contex->h_timer) {
		return ;
	}
	ne_timer_del(contex->h_timer, timer_id) ;
}
