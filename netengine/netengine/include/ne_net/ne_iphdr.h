#ifndef _NE_IPHDR_H_
#define _NE_IPHDR_H_

#include "ne_net/byte_order.h"


#define	NE_IPVERSION	4               /* IP version number */
#define	NE_IP_MAXPACKET	65535			/* maximum packet size */

enum {

    NE_IPPROTO_IP = 0,	   /* Dummy protocol for TCP.  */
    NE_IPPROTO_ICMP = 1,	   /* Internet Control Message Protocol.  */
    NE_IPPROTO_IGMP = 2,	   /* Internet Group Management Protocol. */
    NE_IPPROTO_TCP = 6,	   /* Transmission Control Protocol.  */
    NE_IPPROTO_UDP = 17	   /* User Datagram Protocol.  */
} ;


#pragma pack(push)
#pragma pack(1)
typedef struct _iphdr
{
#  if NE_BYTE_ORDER == NE_L_ENDIAN
	u_8 ip_hl:4;			/* header length */
	u_8 ip_v:4;				/* version */
#elif NE_BYTE_ORDER == NE_B_ENDIAN
	u_8 ip_v:4;				/* version */
	u_8 ip_hl:4;			/* header length */
#endif
	u_8 ip_tos;				/* type of service */
	u_16 ip_len;			/* total length */
	u_16 ip_id;				/* identification */
	u_16 ip_off;			/* fragment offset field */
#define	IP_RF 0x8000			/* reserved fragment flag */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
	u_8 ip_ttl;					/* time to live */
	u_8 ip_p;					/* protocol */
	u_16 ip_sum;				/* checksum */
	u_32 ip_src, ip_dst;		/* source and dest address */
}ip_hdr;
//tcp头

typedef struct ne_tcphdr
 {
    u_16 source;
    u_16 dest;
    u_32 seq;
    u_32 ack_seq;
#  if NE_BYTE_ORDER == NE_L_ENDIAN
    u_16 res1:4;
    u_16 doff:4;
    u_16 fin:1;
    u_16 syn:1;
    u_16 rst:1;
    u_16 psh:1;
    u_16 ack:1;
    u_16 urg:1;
    u_16 res2:2;
#  elif NE_BYTE_ORDER == NE_B_ENDIAN
    u_16 doff:4;
    u_16 res1:4;
    u_16 res2:2;
    u_16 urg:1;
    u_16 ack:1;
    u_16 psh:1;
    u_16 rst:1;
    u_16 syn:1;
    u_16 fin:1;
#  else
#   error "unknow CPU arch"
#  endif
    u_16 window;
    u_16 check;
    u_16 urg_ptr;
}tcp_hdr;

//udp头
typedef struct _udp_hdr
{
    u_16 src_port;       // Source port no.
    u_16 dest_portno;       // Dest. port no.
    u_16 length;       // Udp packet length
    u_16 checksum;     // Udp checksum (optional)
} udp_hdr;


// ICMP header
typedef struct _icmp_hdr
{
    u_8   icmp_type;
    u_8   icmp_code;
    u_16  icmp_checksum;
    u_16  icmp_id;
    u_16  icmp_sequence;
    u_32   icmp_timestamp;
} icmp_hdr;

//tcp伪头部,用来计算CHECK SUM
typedef struct _psuedo_udp_hdr
{
	u_32 src_addr;
	u_32 dest_addr;
	u_8 reserved;
	u_8 protocol;
	u_16 udp_length;
	udp_hdr udp ;
}psuedo_udp;

//tcp伪头部,用来计算CHECK SUM
typedef struct pseudo_tcp_header
{
	u_32 src_addr;
	u_32 dest_addr;
	u_8 reserved;
	u_8 protocol;
	u_16 tcp_length;
	tcp_hdr tcp;
} pseudo_tcp;

#pragma pack(pop)

#endif
