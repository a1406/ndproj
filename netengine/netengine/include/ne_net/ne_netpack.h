#ifndef _NE_NETPACK_H_
#define _NE_NETPACK_H_

#include "ne_net/byte_order.h"
#include "ne_common/ne_common.h"

#define NDNETMSG_VERSION		1

#define NE_PACKET_SIZE 8192

#pragma pack(push)
#pragma pack(1)

/*定义消息包头部*/
typedef struct packet_hdr
{
	NEUINT16	length ;				/*length of packet*/
	NEUINT8		version ;				/*version of net protocol*/
#if NE_BYTE_ORDER == NE_L_ENDIAN
	NEUINT8		encrypt:1;				/* 是否加密*/
	NEUINT8		stuff:1;				/* 是否为了加密而填充*/
	NEUINT8		stuff_len:6;			/* 填充长度*/
#else 
	NEUINT8		stuff_len:6;			/* 填充长度*/
	NEUINT8		stuff:1;				/* 是否为了加密而填充*/
	NEUINT8		encrypt:1;				/* 是否加密*/
#endif
	
} ne_packhdr_t;

//封包头长度
#define NE_PACKET_HDR_SIZE  sizeof(ne_packhdr_t)
#define NE_PACKET_DATA_SIZE (NE_PACKET_SIZE - NE_PACKET_HDR_SIZE)

//消息包
typedef struct ne_net_packet_buf
{
	ne_packhdr_t	hdr ;
	NEINT8			data[NE_PACKET_DATA_SIZE] ;
}ne_packetbuf_t;

#pragma pack(pop)

/* 定义消息发送类型 */
enum send_flag {
	ESF_NORMAL = 0 ,		//正常发送
	ESF_WRITEBUF =1,		//写入发送缓冲
	ESF_URGENCY = 2,		//紧急发送
	ESF_POST	= 4,		//不可靠的发送(可能回丢失)
	ESF_ENCRYPT = 8,			//加密(可以和其他位连用|)
	ESF_ENCRYPT_LATER = 16,   // must use with ESF_WRITEBUF
};

/*初始化头部*/
static __INLINE__ void ne_hdr_init(ne_packhdr_t *hdr)
{
	*((NEUINT32*)hdr) = 0 ;
	hdr->version = NDNETMSG_VERSION ;
}

/*convert header from host to net*/
static __INLINE__ void ne_hdr_hton(ne_packhdr_t *hdr)
{
	//nothing to be done !
	return ;
}

/*convert header from net to host*/
static __INLINE__ void ne_hdr_ntoh(ne_packhdr_t *hdr)
{
	//nothing to be done !
	return ;
}

/*convert header from net to host*/
#define ne_pack_len(hdr) (hdr)->length 

/*convert header from net to host*/
static __INLINE__ void ne_set_pack_len(ne_packhdr_t *hdr, NEUINT16 len)
{
	hdr->length = len;
}

// hdrdest = hdrsrc
#define NE_HDR_SET(hdrdest, hdrsrc) *((NEUINT32*)hdrdest) = *((NEUINT32*)hdrsrc)  

typedef NEINT32 (*NENET_MSGENTRY)(void *connect, ne_packhdr_t *msg_hdr, void *param) ;	//消息处理函数

//#define  msg_hton //nothing
//#define msg_ntoh //nothing
#define packet_hton(p)
#define packet_ntoh(p)
#endif
