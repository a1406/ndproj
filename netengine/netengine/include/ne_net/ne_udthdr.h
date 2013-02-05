#ifndef _NE_UDTHDR_H_
#define _NE_UDTHDR_H_

#include "ne_net/byte_order.h"

#define NEUDT_VERSION 			1
#define NEUDT_FRAGMENT_LEN		536  //reference rfc 1122
#define NEUDT_BUFFER_SIZE		1024 //封包缓冲

//定义UDT报文类型
enum eUdtMsgType{
	NEUDT_DATA=0,
	NEUDT_SYN,
	NEUDT_ALIVE,
	NEUDT_ACK,
	NEUDT_FIN,
	NEUDT_RESET,
};

#define NOT_ACK_SPACE		//不使用ack_seq作为发送数据的缓冲

enum eneudt_protocol{
	PROTOCOL_OTHER = 0,		//UNKNOW POROTOCOL
	PROTOCOL_UDT,			//udt protocol(reliable udp data transfer protocol)
	PROTOCOL_UDP			//common udp data(unreliable udp protocol)
};

#pragma pack(push)
#pragma pack(1)

//udt协议报头
//32bits
struct neudt_header
{
	
	
#  if NE_BYTE_ORDER == NE_L_ENDIAN
	//0~15 bit
	u_16	resevered:2;		//保留
	u_16	stuff:1;			//否为了加密而填充(udt中不使用
	u_16	crypt:1;			//是否加密(udt中不使用
	u_16	version:4;			//版本信息(没有使用,主要是为了保留对齐而以)
	u_16	ack:1;				//是否包含回复
	u_16	udt_type:4;			//报文类型
	u_16	protocol:3;			//协议类型
#else 
	//0~15 bit
	u_16	protocol:3;			//协议类型
	u_16	udt_type:4;			//报文类型
	u_16	ack:1;				//是否包含回复
	u_16	version:4;			//版本信息(没有使用,主要是为了保留对齐而以)
	u_16	crypt:1;			//是否加密(udt中不使用
	u_16	stuff:1;			//否为了加密而填充(udt中不使用
	u_16	resevered:2;		//保留
#endif

	//16~31 bits
	u_16	checksum;
};

#pragma warning (disable: 4200)
//16 bytes
//udt封包头部
struct neudt_pocket
{
	struct neudt_header header;
	u_16	session_id ;			//(port of udt protocol, session id)
	u_16	window_len;			//slide window lenght	
	u_32	sequence ;			//sender sequence 发送系列号
	//32bits * 3
	u_32	ack_seq;			//确认接收系列
	u_8		data[0];
};

//辅助使用的主要是在别的结构中定义了neudt_pocket 只能放在结尾,这个可以用在结构体前面占坑用
struct _neudt_pocket_header		//和上面一样 但是不包含 u_8		data[0];
{
	struct neudt_header header;
	u_16	session_id ;			//(port of udt protocol, session id)
	u_16	window_len;			//slide window lenght	
	u_32	sequence ;			//sender sequence 发送系列号
	//32bits * 3
	u_32	ack_seq;			//确认接收系列
};

//udp数据包
struct neudp_packet
{
	struct neudt_header header;		//32bits
	u_16	session_id ;			//(port of udt protocol, session id)
	//32bits + 16bits
	u_8		data[0] ;
};
//这个也是占坑用的
struct _neudp_packet_header
{
	struct neudt_header header;		//32bits
	u_16	session_id ;			//(port of udt protocol, session id)

};

/*计算udt header用*/
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

//从封包中提取数据
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
