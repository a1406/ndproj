#ifndef _NE_SRV_H_
#define _NE_SRV_H_

#include "ne_net/ne_sock.h"
#include "ne_common/ne_common.h"

#define START_SESSION_ID 1024  
#define SESSION_ID_MASK   0x8000

#define TCP_SERVER_TYPE ('t'<<8 | 's' )		//����tcp�������
/*����ӵĹ���ڵ�*/
struct cm_manager;

typedef void *(*cm_alloc)(ne_handle alloctor) ;							//���ض��������Ϸ���һ�����ӿ��ڴ�
typedef void (*cm_init)(void *socket_node, ne_handle h_listen) ;		//��ʼ��һ��socket
typedef void (*cm_dealloc)(void *socket_node,ne_handle alloctor) ;		//�ӷ��������ͷ����ӿ��ڴ�
typedef void(*cm_walk_callback)(void *socket_node, void *param) ;
typedef NEUINT16 (*cm_accept)(struct cm_manager *root, void *socket_node);
typedef NEINT32 (*cm_deaccept)(struct cm_manager *root, NEUINT16 session_id);	//����0�ɹ�����1ʧ�����û�з��سɹ����ܿ��������ü�����Ϊ0
//typedef NEINT32 (*cm_inc_ref)(struct cm_manager *root, NEUINT16 session_id);	//�������ô��� ����0�ɹ�����1ʧ��
//typedef void (*cm_dec_ref)(struct cm_manager *root, NEUINT16 session_id);	//�������ô���
typedef void *(*cm_lock)(struct cm_manager *root, NEUINT16 session_id);
typedef void *(*cm_trylock)(struct cm_manager *root, NEUINT16 session_id);
typedef void (*cm_unlock)(struct cm_manager *root, NEUINT16 session_id);
typedef void (*cm_walk_node)(struct cm_manager *root,cm_walk_callback cb_entry, void *param);
typedef void *(*cm_search)(struct cm_manager *root, NEUINT16 session_id);

typedef struct cmlist_iterator
{
	NEUINT16 session_id ;
	NEUINT16 numbers ;
}cmlist_iterator_t ;

typedef void* (*cm_lock_first)(struct cm_manager *root,cmlist_iterator_t *it);	//��ס�����е�һ��,����session_id 0
typedef void* (*cm_lock_next)(struct cm_manager *root, cmlist_iterator_t *it);	//��ס��һ��,ͬʱ�ͷŵ�ǰ����session_id��ǰ����ס��ID,���session_id�Ѿ�����ס����һ��ID,�����ͷŵ�ǰ����ס�Ķ���
typedef void (*cm_unlock_iterator)(struct cm_manager *root, cmlist_iterator_t *it) ;
/*��¼�Ѿ�ACCEPT�Ľڵ�*/
struct cm_node 
{
	neatomic_t __used;			//used statusָʾ�˽ڵ��Ƿ�ʹ��
	NEINT32		 is_mask ;			//�Ƿ���������λ(Ϊ�˷�ֹ��������ͬһ��slot����ͬ��sessionid,��Ҫ���´β���sessionʱ����mask)
//	NEINT32	wait_realease;			//�ȴ����ͷ�(���ʹ�ü���û�б���0,�������ô˱�־,�´μ����ͷ�,֪���ͷųɹ�Ϊֹ
	ne_mutex	lock ;			//��udt_socket�Ļ���
	void *client_map ;
};

struct cm_manager
{
	NEINT32				max_conn_num;	//������Ӹ���
	neatomic_t		connect_num;	//��ǰ��������
	struct cm_node	*connmgr_addr;	//����udt���ӵ���ʼ��ַ

	cm_alloc		alloc;
	cm_init			init ;		//��ʼ������
	cm_dealloc		dealloc ;

	//define connect manager function 
	cm_accept		accept ;
	cm_deaccept		deaccept ;
//	cm_inc_ref		inc_ref ;
//	cm_dec_ref		dec_ref ;
	cm_lock			lock;
	cm_trylock		trylock ;
	cm_unlock		unlock ;
	cm_walk_node	walk_node ;
	cm_search		search;

	cm_lock_first	lock_first ;
	cm_lock_next	lock_next ;
	cm_unlock_iterator	unlock_iterator;
};

typedef NEINT32 (*accept_callback) (void* income_handle, SOCKADDR_IN *addr, ne_handle listener) ;	//�������ӽ�����ִ�еĻص�����
typedef void (*deaccept_callback) (void* exit_handle, ne_handle listener) ;					//�������ӱ��ͷ�ʱִ�еĻص�����
typedef NEINT32 (*pre_deaccept_callback) (void* handle, ne_handle listener) ;					//�ͷ�����ǰִ�еĻص�����

struct ne_srv_node
{
	/*Ϊ�˱��ֺ� ne_udtsrv �ļ���,ǰ��8����Աλ��˳���ܸı�*/
	NEUINT32 size ;						/*����Ĵ�С*/
	NEUINT32 type ;						/*�������*/	
	NEUINT32	myerrno;
	ne_close_callback close_entry ;			/*����ͷź���*/
	
	NEINT32					status;				/*socket state in game 0 not read 1 ready*/
	nesocket_t			fd ;				/* socket file description */
	NEINT32					port ;				/* listen port */
	
	void				*user_msg_entry ;	//�û���Ϣ���
	ne_handle			cm_alloctor;		//���ӷ�����(�ṩ���ӿ��ڴ�ķ���

	accept_callback		connect_in_callback ;		//���ӽ���֪ͨ����
	pre_deaccept_callback	pre_out_callback ;		//������Ҫ�˳�,���˳�֮ǰִ�еĺ���,�������-1�������ͷ�����,��һ�μ���
	deaccept_callback	connect_out_callback ;		//�����˳�֪ͨ����
	struct cm_manager	conn_manager ;				/* ���ӹ�����*/
};

static __INLINE__ ne_handle ne_srv_get_allocator(struct ne_srv_node *node)
{
	return node->cm_alloctor;
}

static __INLINE__ void ne_srv_set_allocator(struct ne_srv_node *node,ne_handle a)
{
	node->cm_alloctor = a;
}

NE_NET_API void ne_tcpsrv_node_init(struct ne_srv_node *node);
/*����tcp���*/
//NE_NET_API void _tcp_encrypt(struct ne_tcp_node *socket_addr,	struct ndnet_msg*msg_buf) ;

//NE_NET_API NEINT32 _tcp_decrypt(struct ne_tcp_node *socket_addr,	struct ndnet_msg*msg_buf) ;
NE_NET_API NEINT32 ne_tcpsrv_open(NEINT32 port,NEINT32 listen_nums,struct ne_srv_node *node) ;	/*open net server*/

NE_NET_API void ne_tcpsrv_close(struct ne_srv_node *node) ; /* close net server*/
NE_NET_API NEINT32 tcpsrv_get_accept(nesocket_t listen_fd,struct ne_tcp_node *node) ;


//�趨���������
NE_NET_API NEINT32 cm_listen(struct cm_manager *root, NEINT32 max_num) ;
NE_NET_API void cm_destroy(struct cm_manager *root) ;

//�õ����������
NE_NET_API NEINT32 ne_srv_capacity(struct ne_srv_node *srvnode) ;
#endif
