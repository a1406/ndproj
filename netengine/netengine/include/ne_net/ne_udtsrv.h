#ifndef _NEUDTSRV_H_
#define _NEUDTSRV_H_

#include "ne_common/ne_common.h"
//#include "ne_net/ne_srv.h"

#define UDT_MAX_CONNECT  10000
#define UDP_SERVER_TYPE  ('u'<<8 | 's')
//�������ӹ��̹�
/************************************************************************/

typedef NEINT32 (*data_recv_callback) (ne_udt_node *new_socket, void *data, size_t len) ;//call back function when data come id
typedef NEINT32 (*data_notify_callback)(ne_udt_node *close_socket, size_t data_len) ;		//֪ͨ�����ݵ��� data_len = 0 closed

typedef struct _ne_udtsrv
{
	/*Ϊ�˱��ֺ� struct ne_srv_node �ļ���,ǰ��11����Աλ��˳���ܸı�*/
	NEUINT32 size ;						/*����Ĵ�С*/
	NEUINT32 type;						/*�������*/	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			/*����ͷź���*/
	NEINT32			listen_status ;				//
	nesocket_t  listen_fd	 ;				//udp socket����accept�����ӹ������socket
	NEINT32			port ;						/* listen port */	
	void		*user_msg_entry ;			//�û���Ϣ���
	ne_handle	cm_alloctor;				//���ӿ������
	
	accept_callback		connect_in_callback ;		//���ӽ���֪ͨ����
	pre_deaccept_callback	pre_out_callback ;		//������Ҫ�˳�,���˳�֮ǰִ�еĺ���,�������-1�������ͷ�����,��һ�μ���
	deaccept_callback	connect_out_callback ;		//�����˳�֪ͨ����
	struct cm_manager	conn_manager ;				/* client map mamager */

	//udt server �����еĽṹ
	data_recv_callback income_entry ;				//���ݴ�����,���ʹ�ô˺������������Ժ������Զ���ɾ��
	data_notify_callback data_notify_entry ;		//�����װ�˸ú���,������ִ��income_entry ����(���������Ժ����ݲ��ᱻɾ��)

	ne_mutex	send_lock ;		//����metux 
	SOCKADDR_IN self_addr ; 	//�Լ��ĵ�ַ

} ne_udtsrv;

static __INLINE__ void root_send_lock(ne_udtsrv *root)
{
	ne_assert(root) ;
	ne_mutex_lock(&root->send_lock);
}

static __INLINE__ void root_send_unlock(ne_udtsrv *root)
{
	ne_assert(root) ;
	ne_mutex_unlock(&root->send_lock);
}

ne_udt_node *search_socket_node(struct list_head *head,NEINT32 session_id) ;

//�ͷ�һ���Ѿ��رյ�����
NE_NET_API void release_dead_node(ne_udt_node *socket_node,NEINT32 needcallback) ;

//����fin������ر�����
void _close_listend_socket(ne_udt_node* socket_node) ;

NE_NET_API ne_udtsrv* udt_socket(NEINT32 af,NEINT32 type,NEINT32 protocol );

NE_NET_API NEINT32 udt_socketex(ne_udtsrv* udt_root );

NE_NET_API NEINT32 udt_bind(ne_udtsrv* listen_node ,const struct sockaddr *addr, NEINT32 namelen);

NE_NET_API NEINT32 udt_listen(ne_udtsrv* listen_node,NEINT32 listen_number);

//��������һ��udp����˿�
//���ʹ�ôκ��� udt_socket, udt_bind, udt_listen �����Բ���ʾ����
NE_NET_API ne_udtsrv* udt_open_srv(NEINT32 port);

NE_NET_API NEINT32 udt_open_srvex(NEINT32 port, ne_udtsrv *listen_root) ;

//�ر�udt����
NE_NET_API void udt_close_srv(ne_udtsrv *root , NEINT32 force);
NE_NET_API void udt_close_srvex(ne_udtsrv *root , NEINT32 force) ;

//��socket_node��ӵ�root�ĵȴ�������
NE_NET_API NEINT32 udt_pre_acceptex(ne_udtsrv *root, ne_udt_node *socket_node);

//read income data and accept new connect
NE_NET_API NEINT32 udt_bindio_handler(ne_udtsrv *root, accept_callback accept_entry, data_recv_callback recv_entry,deaccept_callback release_entry);
NE_NET_API void udt_set_notify_entry(ne_udtsrv *root,data_notify_callback entry);

/* ��װ���ӽڵ������� 
 * ����,�ͷź͹���
 */
NE_NET_API void udt_install_connect_manager(ne_udtsrv *root,cm_alloc alloc_entry ,cm_dealloc dealloc_entry) ;

/*��������UDT������,������Ӧ������*/
NE_NET_API NEINT32 doneIO(ne_udtsrv *root, netime_t timeout) ;

//����ÿ��udt_socket��״̬
//��ʱ����ÿ������
NE_NET_API void update_all_socket(ne_udtsrv *root) ;

//��ȡ�������������
NE_NET_API NEINT32 read_udt_handler(ne_udtsrv *root, netime_t timeout) ;

NE_NET_API void init_udt_srv_node(ne_udtsrv *root) ;
#endif 
