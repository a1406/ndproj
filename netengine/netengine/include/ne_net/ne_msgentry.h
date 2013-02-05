#ifndef _NE_MSGENTRY_H_
#define _NE_MSGENTRY_H_

#include "ne_net/ne_netpack.h"
#include "ne_net/ne_netui.h"

enum  privalige_level{
	EPL_NONE = 0 ,			//û���κ�Ȩ��
	EPL_CONNECT,			//��ͨȨ��
	EPL_LOGIN ,				//��½Ȩ��
	EPL_HIGHT				//����
};

#define SUB_MSG_NUM		64		//ÿһ����Ϣ�����ж��ٸ�����Ϣ��
#define MAX_MAIN_NUM	256		//������ӵ�е���Ϣ����

typedef NEUINT16	nemsgid_t ;		//��Ϣ���
typedef NEUINT32	nemsgparam_t ;	//��Ϣ����


#pragma pack(push)
#pragma pack(1)
//�û���Ϣ���� ͷ
typedef struct ne_usermsghdr_t
{
	ne_packhdr_t	packet_hdr ;		//��Ϣ��ͷ
	nemsgid_t		maxid ;		//����Ϣ�� 16bits
	nemsgid_t		minid ;		//����Ϣ�� 16bits
	nemsgparam_t	param;		//��Ϣ����
}ne_usermsghdr_t ;

#define NE_USERMSG_HDRLEN sizeof(ne_usermsghdr_t)
//�û����ݳ���
#define NE_USERMSG_DATA_CAPACITY  (NE_PACKET_SIZE-sizeof(ne_usermsghdr_t) )		
//�����û�����
typedef struct ne_usermsgbuf_t
{
	ne_usermsghdr_t msg_hdr ;
	NEINT8			data[NE_USERMSG_DATA_CAPACITY] ;
}ne_usermsgbuf_t;

#pragma pack(pop)

static __INLINE__ void ne_usermsghdr_init(ne_usermsghdr_t *hdr)
{
	memset(hdr, 0, sizeof(*hdr)) ;
	hdr->packet_hdr.length = NE_USERMSG_HDRLEN ;
	hdr->packet_hdr.version = 1 ;
}
//#define NE_USERMSG_INITILIZER {{NE_USERMSG_HDRLEN,1,0,0,0},0,0,0} 
//#define NE_PACKHDR_INIT = (ne_packhdr_t){NE_USERMSG_HDRLEN,1,0,0,0};
#define ne_netmsg_hton(m)		//��������Ϣת���������ʽ
#define ne_netmsg_ntoh(m)		//��������ʽת���������Ϣ

#define NE_USERMSG_LEN(m)	((ne_packhdr_t*)m)->length
#define NE_USERMSG_VERSION(m)	((ne_usermsghdr_t*)m)->version
#define NE_USERMSG_MAXID(m)	((ne_usermsghdr_t*)m)->maxid 
#define NE_USERMSG_MINID(m)	((ne_usermsghdr_t*)m)->minid 
#define NE_USERMSG_PARAM(m)	((ne_usermsghdr_t*)m)->param
#define NE_USERMSG_DATA(m)	(((ne_usermsgbuf_t*)m)->data)
#define NE_USERMSG_DATALEN(m)	(((ne_packhdr_t*)m)->length - NE_USERMSG_HDRLEN)

#define NE_USERMSG_ENCRYPT(m)	((ne_usermsghdr_t*)m)->packet_hdr.encrypt

/* �û��Զ�����Ϣ������
 * ���������ݵ���ʱ�������Ϣ��ִ����Ӧ�Ĵ�����.
 * ��������ֵ: ����ʱ����-1, ϵͳ���Զ��رն�Ӧ������.
 * ��Ҫ�ر�����ʱ,ֻ��Ҫ�ڴ������з���-1����
 */
typedef NEINT32 (*ne_usermsg_func)(ne_handle  handle, ne_usermsgbuf_t *msg , ne_handle listener);

//ʹ��session id����Ϣ������
typedef NEINT32 (*ne_usermsg_func1)(NEUINT16 session_id, ne_usermsgbuf_t *msg , ne_handle listener); 

/* Ϊ���Ӿ��������Ϣ��ڱ�
 * @mainmsg_num ����Ϣ�ĸ���(�ж�������Ϣ
 * @base_msgid ����Ϣ��ʼ��
 * return value : 0 success on error return -1
 */
NE_NET_API NEINT32 ne_msgtable_create(ne_handle  handle, NEINT32 mainmsg_num, NEINT32 base_msgid) ;

NE_NET_API void ne_msgtable_destroy(ne_handle  handle) ;

/*��handle���Ӿ���ϰ�װ��Ϣ������*/
NE_NET_API NEINT32 ne_msgentry_install(ne_handle  handle, ne_usermsg_func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) ;

/* Ϊlisten���������Ϣ��ڱ�
 * @mainmsg_num ����Ϣ�ĸ���(�ж�������Ϣ
 * @base_msgid ����Ϣ��ʼ��
 * return value : 0 success on error return -1
 */
NE_NET_API NEINT32 ne_srv_msgtable_create(ne_handle listen_handle, NEINT32 mainmsg_num, NEINT32 base_msgid) ;
NE_NET_API void ne_srv_msgtable_destroy(ne_handle listen_handle) ;

/*��listen����ϰ�װ��Ϣ������*/
NE_NET_API NEINT32 ne_srv_msgentry_install(ne_handle listen_handle, ne_usermsg_func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) ;

/* ����������Ϣ */
/*
static __INLINE__ NEINT32 ne_connectmsg_send(ne_handle  connector_handle, ne_usermsgbuf_t *msg ) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg, ESF_NORMAL) ;
}
*/
/*Ͷ����Ϣ,���ɿ��ķ���,��Ϣ���ܻᶪʧ*/
/*
static __INLINE__ NEINT32 ne_connectmsg_post(ne_handle  connector_handle, ne_usermsgbuf_t *msg ) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg, ESF_POST) ;
}
 */
/* ���ͽ������� */
  /* 
static __INLINE__ NEINT32 ne_connectmsg_send_urgen(ne_handle  connector_handle, ne_usermsgbuf_t *msg ) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg,  ESF_URGENCY) ;
}
  */
/* ������չ����*/
   /*
  static __INLINE__ NEINT32 ne_connectmsg_sendex(ne_handle  connector_handle, ne_usermsgbuf_t *msg , NEINT32 flag) 
{
	ne_assert(connector_handle) ;
	ne_assert(msg) ;
	ne_netmsg_hton(msg) ;
	return ne_connector_send(connector_handle, (ne_packhdr_t *)msg,  flag) ;
}
   */
/*
 * ������ӿڹ�������Ϣ���ݸ��û��������Ϣ,
 * һ����������������
 * @connect_handle �������Ϣ���
 * @msg ��Ϣ����
 * @callback_param �û����˲���
 */
NE_NET_API NEINT32 ne_translate_message(ne_handle  connector_handle, ne_packhdr_t *msg ) ;


/* 
 * ����������Ϣ���ݺ���
 * ������ӿڹ�������Ϣ���ݸ��û��������Ϣ,
 * һ����������������
 */
NE_NET_API NEINT32 ne_srv_translate_message(ne_handle listen_handle,ne_handle  connector_handle, ne_packhdr_t *msg ) ;

//ʹ��sessionid ģʽ
NE_NET_API NEINT32 ne_srv_translate_message1(ne_handle listen_handle,NEUINT16 session_id, ne_packhdr_t *msg ) ;

//Ȩ�޵ȼ�
NE_NET_API NEUINT32 ne_connect_level_get(ne_handle  connector_handle) ;

//Ȩ�޵ȼ�
NE_NET_API void ne_connect_level_set(ne_handle  connector_handle,NEUINT32 level);

#endif

