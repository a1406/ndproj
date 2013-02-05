#ifndef _NE_SRVNETIO_H_
#define _NE_SRVNETIO_H_

#include "ne_net/ne_netlib.h"
#include "ne_srvcore/client_map.h"

/*
 * ��ÿ���ͻ������ӵ��������Ժ�,���ڷ������ϲ���һ�����ڵ�����,
 * �Ұ�������ӳ�Ϊһ��session.
 */

typedef struct netui_info  *ne_session_handle ;

//close tcp connect
NEINT32 tcp_client_close(struct ne_client_map* cli_map, NEINT32 force) ;
NEINT32 tcp_client_close_fin(struct ne_client_map* cli_map, NEINT32 force);
//�ͷ�������
#define tcp_release_death_node(c, f) tcp_client_close((struct ne_client_map*)c, f) 
/* close connect*/
NE_SRV_API NEINT32 ne_session_close(ne_handle session_handle, NEINT32 force);

//�����������ӽڵ���ͻ��˷�������
//����һ�������ʽ������
/*
static __INLINE__ NEINT32 ne_session_sendex(ne_handle session_handle,ne_packhdr_t  *msg_buf, NEINT32 flag) 
{
	return ne_connector_send(session_handle, msg_buf, flag) ;
}
*/
//������Ϣ����������
static __INLINE__ NEINT32 ne_sessionmsg_sendex(ne_handle session_handle,ne_usermsghdr_t  *msg, NEINT32 flag)
{
	
	ne_assert(session_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(session_handle, (ne_packhdr_t *)msg, flag) ;
}

/*��ͻ��˷���������Ϣ*/
/*
#define ne_sessionmsg_send(session,msg) ne_sessionmsg_sendex((ne_handle)(session),(ne_usermsghdr_t*)(msg),ESF_NORMAL) 
#define ne_sessionmsg_writebuf(session,msg) ne_sessionmsg_sendex((ne_handle)(session),(ne_usermsghdr_t*)(msg),ESF_WRITEBUF) 
#define ne_sessionmsg_send_urgen(session,msg) ne_sessionmsg_sendex((ne_handle)(session),(ne_usermsghdr_t*)(msg),ESF_URGENCY) 
#define ne_sessionmsg_post(session,msg) ne_sessionmsg_sendex((ne_handle)(session),(ne_usermsghdr_t*)(msg),ESF_POST)
*/
/* ���ͻ������*/
NE_SRV_API NEINT32 ne_session_flush_sendbuf(ne_handle session_handle, NEINT32 flag)  ;

//broadcast netmessage 
// @send_sid session id of sender 
NE_SRV_API NEINT32 ne_session_broadcast(ne_handle listen_handle, ne_usermsghdr_t *msg, NEUINT16 send_sid) ;

#define ne_session_flush(session)		ne_session_flush_sendbuf((ne_handle)session,0)
#define ne_session_flush_force(session)	ne_session_flush_sendbuf((ne_handle)session,1)
#define ne_session_tryto_flush(session)	ne_session_flush_sendbuf((ne_handle)session,2)

static __INLINE__ ne_handle ne_session_getlisten(ne_handle session_handle)
{
	return (ne_handle) (((struct netui_info*)session_handle)->srv_root );
}

//get connect session id 
/*static __INLINE__ NEUINT16 ne_sessionid_get(ne_handle session_handle) 
{
	return ((ne_netui_handle)session_handle)->session_id ;
};
*/

/* check connection is timeout return 1 timeout need to be close*/
NEINT32 check_operate_timeout(ne_handle nethandle, netime_t tmout) ;

NEINT32 tryto_close_tcpsession(ne_session_handle nethandle, netime_t connect_tmout ) ;
//����Ҫ���ͷŵĻ滰��ӵ��ͷŶ���
//void addto_wait_free(ne_session_handle nethandle) ;
#endif
