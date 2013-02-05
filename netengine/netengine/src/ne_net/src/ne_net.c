#include "ne_net/ne_netlib.h"
//#include "ne_net/ne_sock.h"

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib") 
//#ifdef NE_DEBUG
//#pragma comment(lib, "ne_common_dbg.lib") 
//#else 
//#pragma comment(lib, "ne_common.lib") 
//#endif
#endif

NEINT32 register_connector(void) ;
NEINT32 ne_net_init(void)
{
#ifdef WIN32
	{
		WORD wVersionRequested = MAKEWORD(2,2);
		
		WSADATA wsaData;

 		HRESULT nRet = WSAStartup(wVersionRequested, &wsaData);

		if (0!=nRet || wsaData.wVersion != wVersionRequested)
		{	
			WSACleanup();
			return -1;
		}
	}
#else 
#endif
	
	register_connector() ;
	return 0 ;
}

void ne_net_destroy(void)
{
#ifdef WIN32
	WSACleanup();
#else 
#endif
}
/*
void tcp_connector_init(struct ne_tcp_node *node)
{
	
}

void udt_connector_init(ne_udt_node *node)
{
	node->nodetype = NE_UDT ;
}
*/

//×¢²átcp udtÁ¬½ÓÆ÷
NEINT32 register_connector(void)
{
	NEINT32 ret ;
	struct ne_handle_reginfo reginfo ;
	
	//tcp connector register 
	reginfo.object_size = sizeof(struct ne_tcp_node ) ;
	reginfo.init_entry =(ne_init_func ) ne_tcpnode_init ;
	reginfo.close_entry = (ne_close_callback )_connector_destroy  ;
	strcpy(reginfo.name, "tcp-connector" ) ;
	
	ret = ne_object_register(&reginfo) ;
	if(-1==ret) {
		ne_logerror("register tcp-connector error") ;
		return -1 ;
	}
	
	//udt connector register 
	reginfo.object_size = sizeof(ne_udt_node) ;
	reginfo.init_entry = (ne_init_func ) ne_udtnode_init ;
	reginfo.close_entry = (ne_close_callback )_connector_destroy  ;
	strcpy(reginfo.name, "udt-connector" ) ;

	ret = ne_object_register(&reginfo) ;
	if(-1==ret) {
		ne_logerror("register udt-connector error") ;
		return -1 ;
	}
	return ret ;
}
