#ifndef _NE_USERTH_H_
#define _NE_USERTH_H_ 

/*
 * ��һ��ר�ŵ��߳��������û���������������Ϣ�Ͷ�ʱ���¼�
 * ��������Ŀ�Ŀ���ʹ�����������߼��������͵��߳���һ����
 * ������Ϣ�������ͷ��ͺ������÷��Ͻ�������ͬ.
 * ʹ�õ������û��̴߳�����Ϣʱ,ÿ�δ�����Ϣ����������ÿ����Ϣ���ڵ�session.
 * ��������Ϣ��������ֻ��ʹ��sessionid.
 * ���ִ���ʽ�ڱ��ϰ���÷��Ͻ�����һЩ�����ݵĵط�.

 * ���߳�ģʽ,ֻ���û��߼��Ͽ��������߳�.
 * ����Ϣ����ͷ��ͺ����Ͻ�����һЩ����
 * ��Ϣ������ʹ�õ�ʱsessionID����session���,
 * ���ͺ���Ҳһ��,�����ͱ����˶�session������,�����߼�����.
 *
 */

enum e_msg_kind{
	E_MSG_KINE_DATA = 0 ,		//������Ϣ
	E_MSG_KINE_ACCEPT ,			//accept��Ϣ
	E_MSG_KINE_CLOSE			//
};

//���߳�ģʽ�Ľ���͹رպ���
typedef NEINT32(* st_accept_entry) (NEUINT16 sessionid, SOCKADDR_IN *addr, ne_handle listener)  ;
typedef void(* st_deaccept_entry) (NEUINT16 sessionid, ne_handle listener)  ;

//���߳�ģʽ��Ϣ�����߳����ݽṹ
typedef struct _msg_queuq_context
{
	neatomic_t connect_num ;
	nethread_t thid ;
	//�����û���Ϣ������
	st_accept_entry income;
	st_deaccept_entry outcome;
}ne_mqc_t;


NE_SRV_API NEINT32 addto_queue(ne_netui_handle  nethandle, NEUINT16 msgkind, ne_usermsgbuf_t *msg,ne_handle h_listen) ;
#define ne_singleth_msghandler(nethandle, msg, h_listen) addto_queue((ne_netui_handle )nethandle,E_MSG_KINE_DATA, (ne_usermsgbuf_t*)msg, (ne_handle)h_listen)

//����Ϊ���߳�ģʽ
//��������������֮ǰ�������������
NE_SRV_API NEINT32 ne_listensrv_set_single_thread(ne_handle h_listen ) ;
NE_SRV_API NEINT32 ne_listensrv_set_entry_st(ne_handle h_listen, st_accept_entry income, st_deaccept_entry outcome) ;
/*��listen����ϰ�װ��Ϣ������, ���߳�ģʽ*/
static __INLINE__ NEINT32 ne_srv_msgentry_install_st(ne_handle listen_handle, ne_usermsg_func1 fn, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) 
{
	return ne_srv_msgentry_install( listen_handle, (ne_usermsg_func)fn, maxid, minid,  level) ;
}
//���߳�ģʽ�µķ��ͺ���
NE_SRV_API NEINT32 ne_st_send(NEUINT16 sessionid , ne_usermsghdr_t  *msg, NEINT32 flag, ne_handle h_listen ) ;

//���߳�ģʽ�µĹرպ���
NE_SRV_API NEINT32 ne_st_close(NEUINT16 sessionid , NEINT32 flag, ne_handle h_listen ) ;

//��������
NE_SRV_API void ne_st_set_crypt(NEUINT16 sessionid, void *key, NEINT32 size,ne_handle h_listen);
#endif 
