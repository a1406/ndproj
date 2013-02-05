#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"

#include "ne_srvcore/ne_srvlib.h"

#ifdef WIN32
#include "../win_iocp/ne_iocp.h"
#endif 


void ne_tcpcm_init(struct ne_client_map *client_map, ne_handle h_listen)
{	
	ne_tcpnode_init(&(client_map->connect_node)) ;
//	client_map->id = 0 ;
//	client_map->status = ECS_NONE ;
//	client_map->extern_send_entry = NULL;
	INIT_LIST_HEAD(&(client_map->map_list)) ;
//	client_map->name[0] = 0 ;
//	client_map->mapserver_index = -1;
	client_map->connect_node.length = sizeof(struct ne_client_map) ;
	client_map->connect_node.close_entry = (ne_close_callback ) tcp_client_close ;
}

/* get client header size*/
size_t ne_getclient_hdr_size(NEINT32 iomod)
{
	size_t ret ;
	switch(iomod)
	{
	//case NE_LISTEN_UDT_DATAGRAM:
	case NE_LISTEN_UDT_STREAM:
		ret = sizeof(struct ne_udtcli_map) ;
		break ;
	case NE_LISTEN_OS_EXT:
#if defined(WIN32)
		ret = sizeof(struct ne_client_map_iocp) ;
		break ;
#endif
	case 	NE_LISTEN_COMMON :
	default :
		ret = sizeof(struct ne_client_map) ;
		break ; ;
	}
	return ret ;
}
