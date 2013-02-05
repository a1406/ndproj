/* 启动udt listen,并且对相应的clientmap 管理做出调整*/
#ifndef _NE_UDTSRV_H_ 
#define _NE_UDTSRV_H_

//#include "ne_srvcore/ne_srvlib.h"
#include "ne_srvcore/client_map.h"
//#include "ne_net/ne_udt.h"
#include "ne_srvcore/ne_listensrv.h"

/*client connect map is server*/
struct ne_udtcli_map
{
	ne_udt_node connect_node ;		//socket connect node 
//	NEINT32 status ;		//ref eClientStatus
	struct list_head map_list ;
};

NEINT32 udt_datagram(ne_udt_node *conn_socket, void *data, size_t len,void *param)  ;
#endif
