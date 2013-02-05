#include "ne_net/ne_netlib.h"
#include "ne_common/ne_common.h"
//send data from socket
NEINT32 ne_socket_tcp_write(nesocket_t fd, void *write_buf, size_t len)
{
	ne_assert(fd) ;
	ne_assert(len>0) ;
	return send(fd, write_buf, len, 0) ;
}
//reand tcp data from socket fd 
NEINT32 ne_socket_tcp_read(nesocket_t fd, void *buf, size_t buflen)
{
	ne_assert(fd) ;
	ne_assert(buflen>0) ;
	return recv(fd, buf, buflen, 0);
}

/* read date from net 
 * input : udp fd , date buffer address and buffer size 
 * output : from_addr
 * return value: read datalen 
 */
NEINT32 ne_socket_udp_read(nesocket_t fd ,NEINT8 *buf, size_t size, SOCKADDR_IN* from_addr)
{
	NEINT32 sock_len, read_len ;
	SOCKADDR_IN *paddr, addr ;
	
	ne_assert(buf && size > 0) ;
	
	sock_len = sizeof(addr) ;
	if(from_addr) {
		paddr = from_addr ;
	}
	else {
		memset(&addr, 0, sizeof(addr)) ;
		paddr = &addr ;
	}
	
	read_len = recvfrom(fd, buf, size, 0, (LPSOCKADDR)paddr, &sock_len )  ;
#ifdef NE_DEBUG
	if(-1==read_len)
		ne_showerror();
#endif
//	NE_logdebug("read from %s:%d len=%d\n" 
//		AND ne_sock2ip(paddr) AND  ntohs(paddr->sin_port) AND read_len);
	return read_len  ;
}

/* write data to net 
 * return value : write data len 
 */
NEINT32 ne_socket_udp_write(nesocket_t fd, NEINT8* data, size_t data_len ,SOCKADDR_IN* to_addr)
{
	NEINT32 ret = 0 ;
	ne_assert(data && data_len > 0 && to_addr) ;
	ret = sendto(fd, data, data_len,0,(LPSOCKADDR)to_addr, sizeof(*to_addr)) ;
#ifdef NE_DEBUG
	if(-1==ret) {
		ne_showerror() ;
	}
#endif 
	return ret ;
}

//close a net socket of tcp or udp 
void ne_socket_close(nesocket_t s)
{
	if(s==0 || (nesocket_t)-1==s) {
		return ;
	}
#ifdef WIN32
	shutdown(s, SO_DONTLINGER);
	closesocket(s) ;
#else 
	close(s) ;
#endif
}

/*get host address from host name
 * input host name in string (NEINT8*) and port
 * output socket address
 */
NEINT32 get_sockaddr_in(NEINT8 *host_name, NEINT16 port, SOCKADDR_IN* sock_addr)
{
	HOSTENT *host ;
	
	ne_assert(sock_addr && host_name) ;
	if(!sock_addr ) {
		return -1 ;
	}
	
	host = gethostbyname((const NEINT8 *)(host_name ));
	if(!host){
		return -1 ;
	}
	
	memset(sock_addr, 0, sizeof(sock_addr)) ;
	
	sock_addr->sin_family = AF_INET ;
	sock_addr->sin_port = htons(port) ;
	
	sock_addr->sin_addr = *((struct in_addr*)(host->h_addr)) ;
	return 0 ;
}


/* create a socket and bind 
 * @port listened port
 * @type = SOCK_DGRAM create udp port
 * @type = SOCK_STREAM create udp tcp 
 * @out_addr out put address of the socket
 * return -1 error else return socket
 */
nesocket_t ne_socket_openport(NEINT32 port, NEINT32 type, SOCKADDR_IN *sock_add,NEINT32 listen_nums)
{
	nesocket_t listen_fd  ;					/* listen and connect fd */
	SOCKADDR_IN serv_addr ={0} ;		/* socket address */
	NEINT32 ret ,re_usr_addr = 1 ;
	
	ne_assert(type==SOCK_DGRAM || type==SOCK_STREAM) ;
	/* zero all socket slot */
	listen_fd = socket(AF_INET, type, 0) ;
	if(-1 == listen_fd){
		return -1;
	}
	/* set ip address */
	serv_addr.sin_family = AF_INET ;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
	serv_addr.sin_port = htons((NEUINT16)port);
	
	if(type==SOCK_STREAM)
		setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&re_usr_addr, sizeof(re_usr_addr)) ;
	ret = bind (listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ;
	
	if(ret <0){
		return -1 ;
	}
	if(type==SOCK_STREAM)
		listen(listen_fd, listen_nums);
	
	if(sock_add) {
		memcpy(sock_add, &serv_addr, sizeof(serv_addr)) ;
	}
	return listen_fd ;
}

/* connect to server
 * @out put address of remote host
 * return -1 on error else return socket fd
 */
nesocket_t ne_socket_tcp_connect(NEINT8 *host_name, NEINT16 port,SOCKADDR_IN *out_addr)
{
	nesocket_t conn_sock ;
	SOCKADDR_IN their_addr = {0} ;
	if(-1==get_sockaddr_in(host_name, port, &their_addr) ) {
		return -1 ;
	}
	conn_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	if(-1==conn_sock)
		return -1;

	if(-1==connect(conn_sock,(LPSOCKADDR)&their_addr, sizeof(struct sockaddr)) ) {
		close(conn_sock);
		return -1 ;
	}

	if(out_addr)
		memcpy((void*)out_addr, (void*)&their_addr, sizeof(their_addr)) ;
	
	return conn_sock;
}

/*
 * udp connect do not real connect 
 * only create a socket !
 */
nesocket_t ne_socket_udp_connect(NEINT8 *host_name, NEINT16 port,SOCKADDR_IN *out_addr)
{	
	nesocket_t conn_sock ;

	if(out_addr) 
		get_sockaddr_in(host_name, port, out_addr)  ;	
	conn_sock = socket(AF_INET, SOCK_DGRAM, 0);

	/*
	 * to realizing p2p do not use system call 'bind' and 'connect'
	 */
	return conn_sock ;
}

//从ip地址NEINT32 到字符串形式
NEINT8 *ne_inet_ntoa (NEUINT32 in)
{
	static NEINT8 buffer[20];
	NEUINT8 *bytes = (NEUINT8 *) &in;
	snprintf (buffer, 20, "%d.%d.%d.%d",
	      bytes[0], bytes[1], bytes[2], bytes[3]);
	return buffer ;
}

/*
 * 等待socket可写
 * return value : 0 time out , -1 error ,else writablity
 */
NEINT32 ne_socket_wait_writablity(nesocket_t fd,NEINT32 timeval)
{
	NEINT32 ret;
	fd_set rfds;
	struct timeval tmvel ;
	
	FD_ZERO(&rfds) ;
	FD_SET(fd,&rfds) ;
	
	if(-1==timeval){
		ret = select (fd+1, NULL, &rfds,  NULL, NULL) ;
	}
	else {
		tmvel.tv_sec = timeval/1000 ; 
		tmvel.tv_usec = (timeval%1000) * 1000;
		ret = select (fd+1, NULL, &rfds,  NULL, &tmvel) ;
	}

	if(ret <=0 ) {
		return ret ;
	}

	return FD_ISSET(fd, &rfds) ;		
}

/*
 * 等待socket可读
 * return value : 0 time out , -1 error ,else readable
 */
NEINT32 ne_socket_wait_read(nesocket_t fd,NEINT32 timeval)
{
	NEINT32 ret;
	fd_set rfds;
	struct timeval tmvel ;
	
	FD_ZERO(&rfds) ;
	FD_SET(fd,&rfds) ;
	
	if(-1==timeval){
		ret = select (fd+1, NULL, &rfds,  NULL, NULL) ;
	}
	else {
		tmvel.tv_sec = timeval/1000 ; 
		tmvel.tv_usec = (timeval%1000) * 1000;
		ret = select (fd+1, &rfds,  NULL, NULL, &tmvel) ;
	}

	if(ret <=0 ) {
		return ret ;
	}

	return FD_ISSET(fd, &rfds) ;	
};

NEINT32 ne_socket_nonblock(nesocket_t fd, NEINT32 cmd)
{
	NEINT32 val = cmd;
	return ioctlsocket(fd,FIONBIO ,&val) ;
}

//校验和计算
NEUINT16 ne_checksum(NEUINT16 *buf,size_t length)
{
	register size_t len=length>>1;
	register NEUINT32 sum = 0;

	for(sum=0;len>0;len--){
		sum += *buf++;
	}
	if(length&1){
		//sum+= (*buf&0xff00);		//这里出错了,这个程序是大尾数的,哈哈浪费了我很多时间啊
		sum += *(NEUINT8*)buf ;		//这个好一点,不过大小通吃
	}
	sum=(sum>>16)+(sum&0xffff);
	sum+=(sum>>16);

	return (NEUINT16)(~sum);
}
