#ifndef _NE_SOCK_H_
#define _NE_SOCK_H_

//#define DONOT_CONVERT_BIGEND	//不要进行大小尾数转换
#ifdef WIN32
#ifndef _WINDOWS_
#include <WINSOCK2.H>
#endif 
	//typedef SOCKET				nesocket_t;				//socket id 
	typedef NEINT32 				nesocket_t;
	typedef NEINT32						socklen_t ;
	#define	EWOULDBLOCK				WSAEWOULDBLOCK
	#define ne_socket_last_error		WSAGetLastError			
#else 
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	typedef NEINT32 				nesocket_t;
	typedef struct sockaddr_in  SOCKADDR_IN ;
	typedef struct hostent HOSTENT,*LPHOSTENT;
	typedef struct sockaddr SOCKADDR, *LPSOCKADDR ;
	#define  ioctlsocket  ioctl 
	#define ne_socket_last_error()	errno
#endif //win32

//关闭一个连接
NE_NET_API void ne_socket_close(nesocket_t s) ;
//send data from socket 
NE_NET_API NEINT32 ne_socket_tcp_write(nesocket_t fd, void *write_buf, size_t len);
//reand tcp data from socket fd 
NE_NET_API NEINT32 ne_socket_tcp_read(nesocket_t fd, void *buf, size_t buflen) ;

/* read date from net 
 * input : udp fd , date buffer address and buffer size 
 * output : from_addr
 * return value: read datalen 
 */
NE_NET_API NEINT32 ne_socket_udp_read(nesocket_t fd ,NEINT8 *buf, size_t size, SOCKADDR_IN* from_addr);
/* write data to net 
 * return value : write data len 
 */
NE_NET_API NEINT32 ne_socket_udp_write(nesocket_t fd, NEINT8* data, size_t data_len ,SOCKADDR_IN* to_addr);

/*get host address from host name*/
NE_NET_API NEINT32 get_sockaddr_in(NEINT8 *host_name, NEINT16 port, SOCKADDR_IN* sock_addr);

/* create a server socket and bind 
 * @port listened port
 * @type = SOCK_DGRAM create udp port
 * @type = SOCK_STREAM create udp tcp 
 * @out_addr out put address of the socket
 * return -1 error else return socket
 */
NE_NET_API nesocket_t ne_socket_openport(NEINT32 port, NEINT32 type, SOCKADDR_IN *sock_add,NEINT32 listen_nums);
/* connect to server
 * @out put address of remote host
 * return -1 on error else return socket fd
 */
NE_NET_API nesocket_t ne_socket_tcp_connect(NEINT8 *host_name, NEINT16 port,SOCKADDR_IN *out_addr);
NE_NET_API nesocket_t ne_socket_udp_connect(NEINT8 *host_name, NEINT16 port,SOCKADDR_IN *out_addr);
NE_NET_API NEUINT16 ne_checksum(NEUINT16 *buf,size_t length) ;
/*
 * wait socket writablity.
 * return -1 error 
 * return 0 timeout 
 * return 1 socket fd is writablity
 */
NE_NET_API NEINT32 ne_socket_wait_writablity(nesocket_t fd,NEINT32 timeval) ;
NE_NET_API NEINT32 ne_socket_wait_read(nesocket_t fd,NEINT32 timeval);

//compare two address of internet 
//return 0 equate
static __INLINE__ NEINT32 ne_sockadd_in_cmp(SOCKADDR_IN *src_addr, SOCKADDR_IN *desc_addr)
{
	return !((src_addr->sin_port==desc_addr->sin_port) &&
		((u_long)src_addr->sin_addr.s_addr==(u_long)desc_addr->sin_addr.s_addr));
}

NE_NET_API NEINT32 ne_socket_nonblock(nesocket_t fd, NEINT32 cmd) ; //set socket nonblock or blocl

NE_NET_API NEINT8 *ne_inet_ntoa (NEUINT32 in) ;

/*raw socket*/

typedef void (*parse_ip) (NEINT8 *buf, NEINT32 len, SOCKADDR_IN *from) ;
//open raw socket and set IPHDRINCL
NE_NET_API nesocket_t open_raw( NEINT32 protocol);
//send syn pocket 
NE_NET_API NEINT32 send_tcp_syn(nesocket_t fd, SOCKADDR_IN *src, SOCKADDR_IN *dest);
//向指定的地点发送IP包,并且把源地址设定位src
NE_NET_API NEINT32 send_raw_udp(nesocket_t fd, NEINT8 *data, NEINT32 len, SOCKADDR_IN *src, SOCKADDR_IN *dest);
//从原始套接字接受数据
NE_NET_API NEINT32 recv_ippacket(nesocket_t fd,parse_ip func);

NE_NET_API NEINT32 send_icmp(nesocket_t fd, NEINT8 *data, NEINT32 len, SOCKADDR_IN *dest,NEINT32 seq_no) ;
#endif
