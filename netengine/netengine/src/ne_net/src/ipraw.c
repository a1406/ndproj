#include "ne_common/ne_os.h"
#include "ne_net/ne_netlib.h"

#include "ne_net/ne_sock.h"
#include "ne_net/ne_iphdr.h"

/*
 * 在windows xp sp2上已经不在支持IP_HDRINCL 发送tcp和UDP包了
 * 也就不能用他来发送syn包,以便检测主机端口是否打开.
 * 只能发送ICMP和IGMP协议包
 */
#ifdef WIN32

#if _MSC_VER < 1300 // 1200 == VC++ 6.0
#include <WS2TCPIP.H>
#else 
#define IP_HDRINCL                 2 // Header is included with data.
//#include <WinSock2.h>
//#include <windows.h>
//#include <WS2TCPIP.H>
//#include <ws2ipdef.h>
#endif

#else 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netinet/ip.h> 
#include <netinet/tcp.h> 
#include <netdb.h> 
#endif


enum {
	ICMP_ECHO = 8 ,ICMP_ECHOREPLY = 0, ICMP_MIN = 8
};
nesocket_t open_raw( NEINT32 protocol)
{
	NEINT32 opt = 1 ;
	nesocket_t fd = socket(AF_INET,SOCK_RAW, protocol) ;
	if(-1==fd) {
		ne_showerror() ;
		return -1 ;
	}

	/*设置接收和发送的超时*/
#ifdef WIN32
	opt=1000;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (NEINT8*)&opt, sizeof(opt))==-1){
		ne_showerror() ;
		ne_socket_close(fd) ;
		return -1 ;
	} 
#else 
	{

		struct timeval timeout ;
			
		timeout.tv_sec = 1 ;			// one second 
		timeout.tv_usec = 0 ;		
		
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout,sizeof(timeout)) ;
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&timeout,sizeof(timeout)) ;
	}
#endif 

	if(-1==ne_socket_nonblock(fd, 1) ) {
		ne_showerror() ;
		ne_socket_close(fd) ;
		return -1 ;
	}
	
	if(protocol=!IPPROTO_ICMP){
		if(-1==setsockopt(fd, IPPROTO_IP, IP_HDRINCL, (NEINT8*)&opt, sizeof(opt))) {
			ne_showerror() ;
			ne_socket_close(fd) ;
			return -1 ;
		}
	}
	return fd ;
}

//send syn pocket 
NEINT32 send_tcp_syn(nesocket_t fd, SOCKADDR_IN *src, SOCKADDR_IN *dest)
{
	pseudo_tcp *ps_tc ;
	struct ip_packet {
		ip_hdr ip ;
		tcp_hdr  tcp ;
	}t_packet = {0}  ;
	
	t_packet.tcp.source = src->sin_port ;
	t_packet.tcp.dest = dest->sin_port ;
	t_packet.tcp.seq =(u_32) rand() ;
	t_packet.tcp.doff = sizeof(tcp_hdr) >> 2;
	t_packet.tcp.syn = 1 ;						//set syn bit 
	t_packet.tcp.window = htons(512) ;

	ps_tc =(pseudo_tcp*) ((NEINT8*)(&t_packet) + (NEINT32)(sizeof(t_packet) - sizeof(pseudo_tcp))) ;
	ps_tc->dest_addr =(u_32) dest->sin_addr.s_addr ;
	ps_tc->src_addr =(u_32) src->sin_addr.s_addr ;
	//ps_tc->reserved = 0 ;
	ps_tc->protocol = IPPROTO_TCP ;
	
	ps_tc->tcp.check = ne_checksum((NEUINT16*)ps_tc, sizeof(pseudo_tcp)) ;
	
	memset((void*)&t_packet, 0, sizeof(ip_hdr)) ;

	t_packet.ip.ip_hl = sizeof(ip_hdr)>>2 ;
	t_packet.ip.ip_v = NE_IPVERSION ;
	t_packet.ip.ip_id = 1;
	t_packet.ip.ip_tos = 0;
	t_packet.ip.ip_len = sizeof(t_packet);
	t_packet.ip.ip_ttl = 64 ;
	t_packet.ip.ip_p = IPPROTO_TCP;
	t_packet.ip.ip_src = (u_32) src->sin_addr.s_addr ;
	t_packet.ip.ip_dst = (u_32) dest->sin_addr.s_addr ; 

	t_packet.ip.ip_sum = ne_checksum((NEUINT16*)&t_packet, sizeof(ip_hdr)) ;

	
	return sendto(fd,(NEINT8*)&t_packet,sizeof(t_packet),0,(SOCKADDR*)dest,sizeof(*dest)) ;
}

//向指定的地点发送IP包,并且把源地址设定位src
NEINT32 send_raw_udp(nesocket_t fd, NEINT8 *data, NEINT32 len, SOCKADDR_IN *src, SOCKADDR_IN *dest)
{
	psuedo_udp *ps_udp ;
	struct packet {
		ip_hdr ip ;
		udp_hdr  udp ;
		NEINT8 data[8192] ;
	}u_packet  ;

	memset((void*)&u_packet, 0, sizeof(ip_hdr)+sizeof(udp_hdr));

	if(len<=0 || len > 8192)
		return -1 ;
	u_packet.udp.dest_portno = dest->sin_port ;
	u_packet.udp.src_port = src->sin_port ;
	u_packet.udp.length = sizeof(udp_hdr) + len ;
	memcpy(u_packet.data, data, len) ;

	ps_udp =(psuedo_udp*) ((NEINT8*)(&u_packet) + (NEINT32)(sizeof(u_packet) - sizeof(psuedo_udp))) ;

	ps_udp->dest_addr =(u_32) dest->sin_addr.s_addr ;
	ps_udp->src_addr =(u_32) src->sin_addr.s_addr ;
	//ps_udp->reserved = 0 ;
	ps_udp->protocol = IPPROTO_UDP ;
	
	ps_udp->udp.checksum = ne_checksum((NEUINT16*)ps_udp, sizeof(psuedo_udp)) ;
	memset((void*)&u_packet, 0, sizeof(ip_hdr)) ;

	u_packet.ip.ip_hl = sizeof(ip_hdr)>>2 ;
	u_packet.ip.ip_v = NE_IPVERSION ;
	u_packet.ip.ip_id = 0;		//unique identifier : set to 0
	u_packet.ip.ip_tos = 0;
	u_packet.ip.ip_len = sizeof(ip_hdr) + u_packet.udp.length;
	u_packet.ip.ip_ttl = 64 ;
	u_packet.ip.ip_p = IPPROTO_UDP;
	u_packet.ip.ip_src = (u_32) src->sin_addr.s_addr ;
	u_packet.ip.ip_dst = (u_32) dest->sin_addr.s_addr ; 

	u_packet.ip.ip_sum = ne_checksum((NEUINT16*)&u_packet, sizeof(ip_hdr)) ;

	
	return sendto(fd,(NEINT8*)&u_packet,u_packet.ip.ip_len,0,(SOCKADDR*)dest,sizeof(*dest)) ;
	//return send(fd,(NEINT8*)&u_packet,u_packet.ip.ip_len,0);
}

NEINT32 send_icmp(nesocket_t fd, NEINT8 *data, NEINT32 len, SOCKADDR_IN *dest,NEINT32 seq_no)
{
	struct packet {
		icmp_hdr icmp ;
		NEINT8 data[8192] ;
	}u_packet  ;

	memset((void*)&u_packet, 0, sizeof(icmp_hdr));

	u_packet.icmp.icmp_code = 0 ;
	u_packet.icmp.icmp_type = ICMP_ECHO ;
	u_packet.icmp.icmp_id = (u_16)ne_processid() ;
	u_packet.icmp.icmp_sequence = seq_no ;
	u_packet.icmp.icmp_checksum = 0 ;
	u_packet.icmp.icmp_timestamp = ne_time() ;

	if(data && len >0  && len<8192) {
		memcpy(u_packet.data, data, len) ;
	}

	len +=  sizeof(icmp_hdr) ;
	u_packet.icmp.icmp_checksum = ne_checksum((NEUINT16*)&u_packet, len) ;
	
	return sendto(fd,(NEINT8*)&u_packet, len,0,(SOCKADDR*)dest,sizeof(*dest)) ;
}

//从原始套接字接受数据
NEINT32 recv_ippacket(nesocket_t fd,parse_ip func)
{
	NEINT32 readlen , sock_len;
	SOCKADDR_IN from ;
	NEINT8 buffer[8192] ;

	sock_len = sizeof(from) ;
	readlen = recvfrom(fd, buffer, 8192,0,(SOCKADDR*)&from, &sock_len);
	if(readlen>=sizeof(ip_hdr)) {
		func(buffer, readlen,&from);
	}
	return readlen ;
}
