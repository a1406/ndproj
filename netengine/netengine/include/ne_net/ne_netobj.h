#ifndef _NE_NETOBJ_H_
#define _NE_NETOBJ_H_

#define  NE_NETOBJ_BASE \
	NEUINT32	length ;\
	NEUINT32	nodetype;\
	NEUINT32	myerrno;\
	ne_close_callback close_entry ;\
	NEUINT32	level ;\
	size_t		send_len ;\
	size_t		recv_len ;\
	netime_t	start_time ;\
	netime_t	last_recv ;	\
	NEUINT16	session_id;	\
	NEUINT16	close_reason;\
	write_packet_entry	write_entry;	\
	void *user_msg_entry ;				\
	void *srv_root;						\
	void *user_data ;					\
	ne_cryptkey	crypt_key ;				\
	struct sockaddr_in 	remote_addr 	

#define NE_NETBUF_SIZE (1024*16)

//¶¨ÒåÍøÂç»º³å
typedef struct ne_netbuf 
{	
	NEUINT32 buf_capacity ;
	NEINT8 *__start, *__end ;	
	NEINT8 buf[NE_NETBUF_SIZE] ;
}ne_netbuf_t;

#endif
