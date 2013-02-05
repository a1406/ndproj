#include "ne_app.h" 

NEINT32 update_client_map(void *param) ;
static ne_thsrvid_t timer_thid =0;

ne_thsrvid_t get_timer_thid()
{
	return timer_thid ;
}
NEINT32 start_timer_srv(netime_t interval)
{
	struct ne_thsrv_createinfo th_info = {
		SUBSRV_RUNMOD_LOOP,	//srv_entry run module (ref e_subsrv_runmod)
		(ne_threadsrv_entry)update_client_map,			//service main entry function address
		(void*)interval,		//param of srv_entry 
		NULL ,					//handle received message from other thread!
		NULL,					//clean up when server is terminal
		_NET("timer-update"),			//service name
		NULL		//user data
	};

	
	timer_thid = ne_thsrv_createex(&th_info, NET_PRIORITY_LOW,0)  ;
	if(!timer_thid )
		return -1;

	return 0 ;
}

/* entry of  timer update client map*/
NEINT32 update_client_map(void *param)
{
	NEINT32 cm_num ;
	NEINT32 sleep = 0 ;
	//NEUINT16 session_id = 0;
	cmlist_iterator_t cm_iterator ;
	struct cm_manager *pmanger  ;
	ne_handle client;
	ne_usermsgbuf_t sendmsg;
	ne_handle listen_haneld = get_listen_handle () ;
	if(!listen_haneld) {
		ne_sleep(1000) ;
		return 0 ;
	}
//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;		
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_SYS ;
	NE_USERMSG_MINID(&sendmsg) = SYM_TEST ;
	//data = NE_USERMSG_DATA(&sendmsg) ;
	strcpy(NE_USERMSG_DATA(&sendmsg) , "hello world!") ;
	NE_USERMSG_LEN(&sendmsg) = 13 + NE_USERMSG_HDRLEN ;
	NE_USERMSG_PARAM(&sendmsg) = 0  ;
	cm_num = 0 ;
	pmanger = ne_listensrv_get_cmmamager(listen_haneld) ;
	for(client = pmanger->lock_first (pmanger,&cm_iterator) ; client; 
				client = pmanger->lock_next (pmanger,&cm_iterator) ) {
		ne_sleep(10) ;		//simulate processor running
		ne_sessionmsg_sendex(client,&sendmsg, ESF_NORMAL) ;
//		if(rand()%9 == 0 ) {
//			ne_session_close(client,0) ;
//		}
		cm_num++ ;
	}
	ne_sleep(1000) ;
	if(cm_num)neprintf("\t\tcurrent connect num=%d\n", cm_num) ;
	return 0;
}
