#include "ne_cliapp/ne_cliapp.h"
extern NEINT32 loging_message_handle(ne_handle nethandle,ne_usermsgbuf_t *recv_msg, ne_handle h_listen) ;



NEINT32 test_message_handle(ne_handle nethandle,ne_usermsgbuf_t *recv_msg, ne_handle h_listen) 
{
	NEINT8 tmp ;
	NEINT32 len = NE_USERMSG_LEN(recv_msg) - NE_USERMSG_HDRLEN;
	NEINT8 *addr = NE_USERMSG_DATA(recv_msg) ;
	tmp = addr[len] ;
	addr[len] = 0 ;
	neprintf("[TEST MSG maxid=%d minid=%d] %s\n", NE_USERMSG_MAXID(recv_msg),NE_USERMSG_MINID(recv_msg), addr) ;
	addr[len] = tmp ;
	return 0 ;

}

void install_msg(ne_handle connector)
{
	NEINT32 i;
	for (i=0; i<SSM_MSG_NUM; i++)
	{
		//install_climsg_entry(loging_message_handle, MAXID_START_SESSION, i, 0);
		ne_msgentry_install(connector,(ne_usermsg_func)loging_message_handle, MAXID_START_SESSION, i, 0);
	}
	ne_msgentry_install(connector,(ne_usermsg_func)test_message_handle, MAXID_SYS, SYM_TEST, 0);
}

NEINT32 test_broadcast_handle(ne_handle nethandle,ne_usermsgbuf_t *recv_msg, ne_handle h_listen) 
{
	NEINT8 tmp ;
	NEINT32 len = NE_USERMSG_LEN(recv_msg) - NE_USERMSG_HDRLEN;
	NEINT8 *addr = NE_USERMSG_DATA(recv_msg) ;
	tmp = addr[len] ;
	addr[len] = 0 ;
//	neprintf("[BROADCAST MSG maxid=%d minid=%d] %s\n", NE_USERMSG_MAXID(recv_msg),NE_USERMSG_MINID(recv_msg), addr) ;
	addr[len] = tmp ;
	return 0 ;

}
