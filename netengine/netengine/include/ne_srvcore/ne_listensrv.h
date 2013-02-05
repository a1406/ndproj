#ifndef _NE_LISTENSRV_H_
#define _NE_LISTENSRV_H_

//#include "ne_srvcore/ne_srvlib.h"
#include "ne_srvcore/client_map.h"
#include "ne_srvcore/ne_session.h"
#include "ne_net/ne_udt.h"
#include "ne_srvcore/ne_udtsrv.h"
#include "ne_srvcore/ne_userth.h"

//#define _MAX_CLIENTS	 2		//fot test
#define CONNECTION_TIMEOUT		60 //time out between twice read/write (second)
//#define _AFFIX_DATA_LENT 12		//for test
#define LISTEN_INTERVAL  100    //10hz

enum ENE_LISTEN_MOD{
	NE_LISTEN_COMMON = 0 ,
	NE_LISTEN_OS_EXT,
	NE_LISTEN_UDT_STREAM			//ʹ��UDT����STREAM
	//NE_LISTEN_UDT_DATAGRAM			//ʹ�� reliable udp
};

#define _IS_UDT_MOD(iomod) ((iomod)==NE_LISTEN_UDT_STREAM )

typedef struct swap_list{
	NEINT32 inited ;
	struct list_head list ;
	ne_mutex		 lock ;
}swap_list_t;

static __INLINE__ void ne_swap_list_init(swap_list_t *sw)
{
	sw->inited = 1 ;
	INIT_LIST_HEAD(&sw->list) ;
	ne_mutex_init(&sw->lock) ;
}
static __INLINE__ void ne_swap_list_destroy(swap_list_t *sw)
{
	if(sw->inited) {
		ne_mutex_destroy(&sw->lock) ;
		sw->inited = 0 ;
	}
}


/* listen service contex*/
struct listen_contex
{
	union {
		struct ne_srv_node	tcp;		//server port
		ne_udtsrv			udt;		//udt socket port
	};
	NEINT32 io_mod ;						//listen module
//	NEINT32 single_thread_mod ;				//�Ƿ�ʹ�õ��߳�ģʽ
	netime_t	operate_timeout ;				//timeout between twice net read/write (if timeout session would be close)
	volatile ne_thsrvid_t  listen_id ;			//listen thread server id
	volatile ne_thsrvid_t  sub_id ;				//sub thread server id 
	void	 *user_data ;						//user data of listener
	ne_mqc_t	 mqc;							//���߳�ģʽ���ݽṹ
	struct list_head conn_list ;				//���Ӷ���
	swap_list_t		wait_free ;					//�ȴ����ͷŵĶ���
#ifdef USER_UPDATE_LIST
	swap_list_t		wait_add ;					//�ȴ�����ӵĶ���
#endif	
//	ne_close_callback	session_release_entry ;	//session �ĵײ�رպ���
//	ne_mutex			wf_lock ;				//lock of wait_free
//	struct list_head	wait_free;				//����ȴ��ͷŵĶ���
} ;

#ifdef IMPLEMENT_LISTEN_HANDLE
	typedef struct listen_contex *ne_listen_handle ;
#else 
	typedef ne_handle	ne_listen_handle ;
#endif

/*������,�������������߳�*/
NE_SRV_API NEINT32 ne_listensrv_open(NEINT32 port, ne_listen_handle handle) ;
/*�ر�����رռ����߳�*/
NE_SRV_API NEINT32 ne_listensrv_close(ne_listen_handle handle, NEINT32 force) ;
/*���ö�Ӧ���ӵ�������Բ������ڴ�*/
NE_SRV_API NEINT32 ne_listensrv_session_info(ne_listen_handle handle, NEINT32 max_client,size_t session_size) ;
/*�������ӽ�����˳��Ļص�����*/
NE_SRV_API void ne_listensrv_set_entry(ne_listen_handle handle, accept_callback income,pre_deaccept_callback pre_close, deaccept_callback outcome) ;
/*
 * ����session��alloc ��dealloc ��Ҫinitilizer ����
 * һ�������Ҫ��������,�����㲻��ʹ��Ĭ�ϵķ���ͳ�ʼ������.
 */
NE_SRV_API void ne_listensrv_alloc_entry(ne_listen_handle handle, cm_alloc al, cm_dealloc dealloc, cm_init init) ;

NE_SRV_API ne_handle ne_listensrv_get_cmallocator(ne_listen_handle handle) ;

//�õ����ӹ�����
NE_SRV_API struct cm_manager *ne_listensrv_get_cmmamager(ne_listen_handle handle) ;

//�ڼ������Ͻ�����ʱ��
//ֻ���ڵ��߳�ģʽ��ʱ����б�Ҫ�ڼ������ϴ�����ʱ
//�������Ա�֤��ʱ������Ϣ������һ���߳���ִ��
//����Ƕ��߳�ģʽ���Խ����������߳���ִ�ж�ʱ����������ʱ������ȷһЩ
NE_SRV_API netimer_t ne_listensrv_timer(ne_listen_handle h_listen,ne_timer_entry func,void *param,netime_t interval, NEINT32 run_type ) ;
NE_SRV_API void ne_listensrv_del_timer(ne_listen_handle h_listen, netimer_t timer_id ) ;

/* �ͷ��Ѿ����ر�,���ǻ���ʹ�õĻ滰*/
//void release_dead_cm(struct listen_contex * lc) ;
//�õ�listen file description
static __INLINE__ nesocket_t get_listen_fd(struct listen_contex * handle)
{
	return handle->tcp.fd;
}

struct ne_client_map * accetp_client_connect(struct listen_contex *listen_info) ;

/*listen_msg_entry ������Ϣ����ص�,����connect socket�Ļ�����*/
//NEINT32 listen_msg_entry(struct ne_tcp_node*node,struct ndnet_msg *msg,void *param)  ;

NEINT32 srv_stream_data_entry(void *node,ne_packhdr_t *msg,void *param) ;
/*deal with received net message*/
/*NE_SRV_API*/ NEINT32 ne_do_netmsg(struct ne_client_map *cli_map,struct ne_srv_node *srv_node) ;

NE_SRV_API void udt_clientmap_init(struct ne_udtcli_map *node, ne_handle h_listen ) ; 

static __INLINE__ void ne_set_connection_timeout(ne_listen_handle h, NEINT32 second)
{
	((struct listen_contex*)h)->operate_timeout = second * 1000 ;
}

static __INLINE__ netime_t ne_get_connection_timeout(ne_listen_handle h)
{
	return ((struct listen_contex*)h)->operate_timeout;
}

#endif
