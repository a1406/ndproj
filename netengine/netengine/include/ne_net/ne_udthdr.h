#ifndef _NE_UDTHDR_H_
#define _NE_UDTHDR_H_

#include "ne_net/byte_order.h"

#define NEUDT_VERSION 			1
#define NEUDT_FRAGMENT_LEN		536  //reference rfc 1122
#define NEUDT_BUFFER_SIZE		1024 //�������

//����UDT��������
enum eUdtMsgType{
	NEUDT_DATA=0,
	NEUDT_SYN,
	NEUDT_ALIVE,
	NEUDT_ACK,
	NEUDT_FIN,
	NEUDT_RESET,
};

#define NOT_ACK_SPACE		//��ʹ��ack_seq��Ϊ�������ݵĻ���

enum eneudt_protocol{
	PROTOCOL_OTHER = 0,		//UNKNOW POROTOCOL
	PROTOCOL_UDT,			//udt protocol(reliable udp data transfer protocol)
	PROTOCOL_UDP			//common udp data(unreliable udp protocol)
};

#pragma pack(push)
#pragma pack(1)

//udtЭ�鱨ͷ
//32bits
struct neudt_header
{
	
	
#  if NE_BYTE_ORDER == NE_L_ENDIAN
	//0~15 bit
	u_16	resevered:2;		//����
	u_16	stuff:1;			//��Ϊ�˼��ܶ����(udt�в�ʹ��
	u_16	crypt:1;			//�Ƿ����(udt�в�ʹ��
	u_16	version:4;			//�汾��Ϣ(û��ʹ��,��Ҫ��Ϊ�˱����������)
	u_16	ack:1;				//�Ƿ�����ظ�
	u_16	udt_type:4;			//��������
	u_16	protocol:3;			//Э������
#else 
	//0~15 bit
	u_16	protocol:3;			//Э������
	u_16	udt_type:4;			//��������
	u_16	ack:1;				//�Ƿ�����ظ�
	u_16	version:4;			//�汾��Ϣ(û��ʹ��,��Ҫ��Ϊ�˱����������)
	u_16	crypt:1;			//�Ƿ����(udt�в�ʹ��
	u_16	stuff:1;			//��Ϊ�˼��ܶ����(udt�в�ʹ��
	u_16	resevered:2;		//����
#endif

	//16~31 bits
	u_16	checksum;
};

#pragma warning (disable: 4200)
//16 bytes
//udt���ͷ��
struct neudt_pocket
{
	struct neudt_header header;
	u_16	session_id ;			//(port of udt protocol, session id)
	u_16	window_len;			//slide window lenght	
	u_32	sequence ;			//sender sequence ����ϵ�к�
	//32bits * 3
	u_32	ack_seq;			//ȷ�Ͻ���ϵ��
	u_8		data[0];
};

//����ʹ�õ���Ҫ���ڱ�Ľṹ�ж�����neudt_pocket ֻ�ܷ��ڽ�β,����������ڽṹ��ǰ��ռ����
struct _neudt_pocket_header		//������һ�� ���ǲ����� u_8		data[0];
{
	struct neudt_header header;
	u_16	session_id ;			//(port of udt protocol, session id)
	u_16	window_len;			//slide window lenght	
	u_32	sequence ;			//sender sequence ����ϵ�к�
	//32bits * 3
	u_32	ack_seq;			//ȷ�Ͻ���ϵ��
};

//udp���ݰ�
struct neudp_packet
{
	struct neudt_header header;		//32bits
	u_16	session_id ;			//(port of udt protocol, session id)
	//32bits + 16bits
	u_8		data[0] ;
};
//���Ҳ��ռ���õ�
struct _neudp_packet_header
{
	struct neudt_header header;		//32bits
	u_16	session_id ;			//(port of udt protocol, session id)

};

/*����udt header��*/
union _uudt_header
{
	struct neudt_header hdr;
	NEINT32 _val ;
};
#pragma pack(pop)
#define UDP_PACKET_HEAD_SIZE sizeof(struct neudp_packet)

static __INLINE__ void _init_udt_header(struct neudt_header *hdr)
{
	ne_assert(4==sizeof(*hdr));
	((union _uudt_header*) hdr)->_val = 0;
}
//copy udt header 
static __INLINE__ void copy_udt_hdr(struct neudt_header *src, struct neudt_header* desc) 
{
	((union _uudt_header*) desc)->_val = ((union _uudt_header*) src)->_val ;
}


//#define copy_udt_hdr(src, desc) ((union _uudt_header*) (desc))->_val = ((union _uudt_header*) (src))->_val

//get pocket data addr
static __INLINE__ NEINT8* pocket_data(struct neudt_pocket *pocket)
{
	if(pocket->header.ack) {
		return (NEINT8*)(pocket->data );
	}else {
		return(NEINT8*) &(pocket->ack_seq);
	}
}

static __INLINE__ void set_pocket_ack(struct neudt_pocket *pocket,u_32 ack)
{
	pocket->header.ack = 1 ;
	pocket->ack_seq = ack ;
}


//get pocket header size
static __INLINE__ NEINT32 net_header_size(struct neudt_pocket *pocket)
{
	if(pocket->header.ack) {
		return sizeof(struct neudt_pocket);
	}else {
		return sizeof(struct neudt_pocket)-sizeof(u_32);
	}
}

//�ӷ������ȡ����
static __INLINE__ void *get_datagram_data(void *packet)
{
	union u1{
		struct neudt_header _header;
		struct neudt_pocket _udt;
		struct neudp_packet _udp;
	}*pocket_addr = (union u1*)packet ;

	if(PROTOCOL_UDT==pocket_addr->_header.protocol) {
		return pocket_data(&(pocket_addr->_udt)) ;
	}
	else if(PROTOCOL_UDP==pocket_addr->_header.protocol) {
		return pocket_addr->_udp.data ;
	}
	else 
		return packet;
	
}

#pragma pack(push)
#pragma pack(1)

typedef union pocket_buffer
{
	struct neudt_pocket pocket ;
	u_8		_buffer[NEUDT_BUFFER_SIZE] ;
}udt_pocketbuf ;
#pragma pack(pop)

/*
static __INLINE__ void init_common_pocket(struct neudt_pocket *pocket)
{
	u_32 *p =(u_32 *) pocket ;
	*p++ = 0 ;*p++=0; *p++=0;
	pocket->header.protocol=PROTOCOL_UDP ;
}
*/
static __INLINE__ void init_udt_pocket(struct neudt_pocket *pocket)
{
	u_32 *p =(u_32 *) pocket ;
	*p++ = 0 ;*p++=0; *p++=0; 
	pocket->header.protocol=PROTOCOL_UDT ;
}
#define POCKET_SESSIONID(pocket) (pocket)->session_id
#define POCKET_CHECKSUM(pocket) (pocket)->header.checksum
#define POCKET_PROTOCOL(pocket) (pocket)->header.protocol
#define POCKET_TYPE(pocket)		(pocket)->header.udt_type
#define POCKET_ACK(pocket)		(pocket)->header.ack

//SET  pocket type
#define SET_SYN(pocket) POCKET_TYPE(pocket)=NEUDT_SYN 
#define SET_ALIVE(pocket) POCKET_TYPE(pocket)=NEUDT_ALIVE
#define SET_FIN(pocket) POCKET_TYPE(pocket)=NEUDT_FIN
#define SET_RESET(pocket) POCKET_TYPE(pocket)=NEUDT_RESET
#define SET_ACK(pocket) POCKET_TYPE(pocket)=NEUDT_ACK

#if NE_BYTE_ORDER == NE_B_ENDIAN
static __INLINE__ void _udt_host2net(struct neudt_pocket *pock)
{
	pock->header.checksum = ne_btols(pock->header.checksum) ;
	pock->session_id = ne_btols(pock->session_id);
	pock->window_len = ne_btols(pock->window_len);
	pock->sequence = ne_btoll(pock->sequence);
	if(pock->header.ack) {
		pock->ack_seq = ne_btoll(pock->ack_seq);
	}
}
#define udt_host2net(pocket)  _udt_host2net(pocket)
#define udt_net2host(pocket)  _udt_host2net(pocket)

#else 
#define udt_host2net(pocket)  //nothing to be done
#define udt_net2host(pocket)  //nothing to be done

#endif

#endif 
