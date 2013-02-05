#ifndef _NDSRVENGTY_H_
#define _NDSRVENGTY_H_

#include "ne_common/ne_common.h"

/*
 * ���̵߳ķ�װ,ʹ������������һ��������
 * ����������ģ�����:
 * ��������ִ�еĹ���ģ�������ﱻ��Ϊһ���ӷ���.���³�Ϊ"�̷߳�����"
 * ÿ����������һ���߳�,���Լ����߳�������,����Ϣ����,
 * ������������߳̿���ͨ���������ֻ��߷���ID�ҵ�����,���ҷ�����Ϣ.
 */
#define MAX_LISTENNUM	2	
#define NE_SRV_NAME 32 

typedef nethread_t ne_thsrvid_t ;
typedef NEINT32 (*ne_threadsrv_entry)(void *param) ;		//service entry
typedef void (*ne_threadsrv_clean)(void) ;				//service terminal clean up fucnton 

/* thread message data*/
typedef struct ne_thread_msg{
	NEUINT32			msg_id ;
	NEUINT32			data_len ;			//data length�����
	ne_thsrvid_t		from_id,recv_id;	//message sender and receiver
	void				*th_userdata ;		//��Ӧ ne_thsrv_createinfo::data
	struct list_head	list ;
	NEINT8				data[0] ;			//data address
}ne_thsrv_msg;

typedef NEINT32 (*ne_thsrvmsg_func)(ne_thsrv_msg *msg) ;	//message handle function


/* ����������
 * ���ʹ��SUBSRV_RUNMOD_STARTUPģʽ��Ҫ���Լ��ĳ����д�����Ϣ
 * �÷�: 
 *  
		service_entry() {
			//do some initilize
			while(!ne_get_exit) {
				//your code 
				...
				
				if(ne_thsrv_msghandler(NULL) ) 
					return ;
				...
			}
			return ;
		}
 */
enum e_thsrv_runmod{
	SUBSRV_RUNMOD_LOOP = 0,		//for(;;) srv_entry() ;
	SUBSRV_RUNMOD_STARTUP,		//return srv_entry() ; 
	SUBSRV_MESSAGE				//����һ����������Ϣ�����߳�(��Ҫ�ǿ���ʵ��һ���̳߳�,ֻ������Ϣ���߳�)
};

struct ne_thsrv_createinfo
{
	NEINT32 run_module ;					//srv_entry run module (ref e_subsrv_runmod)
	ne_threadsrv_entry srv_entry ;		//service entry
	void *srv_param ;					//param of srv_entry 
	ne_thsrvmsg_func msg_entry ;		//handle received message from other thread!
	ne_threadsrv_clean cleanup_entry ;	//clean up when server is terminal
	nechar_t srv_name[NE_SRV_NAME] ;	//service name
	void *data ;						//user data
};


/*
 * create_service() create a server .
 * @create_info : input service info
 */
NE_SRV_API ne_thsrvid_t ne_thsrv_createex(struct ne_thsrv_createinfo* create_info,NEINT32 priority, NEINT32 suspend ) ;
static __INLINE__ ne_thsrvid_t ne_thsrv_create(struct ne_thsrv_createinfo* create_info)
{
	return ne_thsrv_createex(create_info,NET_PRIORITY_NORMAL, 0) ;
}

/* get service context 
 * if srvid == 0 ,get current service context
 */
NE_SRV_API ne_handle ne_thsrv_gethandle(ne_thsrvid_t srvid );

NE_SRV_API ne_thsrvid_t ne_thsrv_getid(ne_handle);
/*
 * stop service and destroy service context.
 * if force zero wait for service thread exit and return exit code
 * else return 0
 */
NE_SRV_API  NEINT32 ne_thsrv_destroy(ne_thsrvid_t srvid,NEINT32 force);

/*release all services in current host*/
NE_SRV_API void ne_thsrv_release_all() ;

/* �������̵߳������˳�,�����߳�Ҳ�˳�*/
NE_SRV_API void ne_host_eixt() ;
NE_SRV_API NEINT32 ne_host_check_exit() ;

NE_SRV_API NEINT32 ne_thsrv_check_exit(ne_thsrvid_t srv_id) ;
NE_SRV_API NEINT32 ne_thsrv_isexit(ne_handle) ;

NE_SRV_API NEINT32 ne_thsrv_end(ne_thsrvid_t  srv_id) ;		//terminal a service

NE_SRV_API NEINT32 ne_thsrv_suspend(ne_thsrvid_t  srv_id) ;		//wait a service exit
NE_SRV_API NEINT32 ne_thsrv_resume(ne_thsrvid_t  srv_id) ;		//wait a service exit

NE_SRV_API NEINT32 ne_thsrv_wait(ne_thsrvid_t  srv_id) ;		//wait a service exit
//NE_SRV_API struct ne_thread_msg *ne_thsrv_msgnode_create(NEINT32 datalen) ;

/* ������Ϣ �Ƽ�ʹ��ne_send_msg() 
 * ���ʹ��ne_send_msgex() �÷�:
	msg = create_thmsg_node(datalen) ;
	//set message id, receiver id, data, and datalen to ne_thread_msg.
	ne_thsrv_sendex(msg);
 */
NE_SRV_API NEINT32 ne_thsrv_send(ne_thsrvid_t recvid,NEUINT32 msgid,void *data, NEUINT32 data_len) ;
//NE_SRV_API NEINT32 ne_thsrv_sendex(struct ne_thread_msg *msg);

/* �����û���Ϣ
 * �������SUBSRV_RUNMOD_STARTUP ģʽ����������,����Ҫ�ڷ�����������д�����Ϣ
 * һ���÷����ڷ���ѭ���е���ne_message_handler() ,������ָ��context��Ϊ�����Ч��
 * return 0 service(thread) exit 
 * return 1 message has been handled
 */
NE_SRV_API NEINT32 ne_thsrv_msghandler( ne_handle  thsrv_handle) ;	

/* get thread server memory pool 
 * 
 */
NE_SRV_API ne_handle ne_thsrv_local_mempool(ne_handle  thsrv_handle) ;

/* create timer in service which specified by srv_id, if srv_id == 0 create in current service
 * @entry input : entry of timer 
 * param input : parameter of entry
 * return value timer id 
 * reference netimer
 */


NE_SRV_API netimer_t ne_thsrv_timer(ne_thsrvid_t srv_id,ne_timer_entry func,void *param,netime_t interval, NEINT32 run_type ) ;
NE_SRV_API void ne_thsrv_del_timer(ne_thsrvid_t srv_id, netimer_t timer_id ) ;

#endif
