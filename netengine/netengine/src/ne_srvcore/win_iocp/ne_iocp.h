#ifndef _NE_IOCP_H_
#define _NE_IOCP_H_

#ifdef WIN32

//#define USER_THREAD_POLL		//�Ƿ�ʹ���߳�ģʽ

#define IOCP_DELAY_CLOSE_TIME		(10*1000)	//�ӳ�10��ر�
#include "ne_srvcore/client_map.h"
#include "ne_srvcore/ne_listensrv.h"
//#include "ne_srvcore/ne_srvlib.h"
typedef BOOL (WINAPI *BindIoCPCallback)(
	HANDLE FileHandle,                         // handle to file
	LPOVERLAPPED_COMPLETION_ROUTINE Function,  // callback
	ULONG Flags                                // reserved
);

struct send_buffer_node
{
	struct list_head list ;
	//struct ndnet_msg msg_buf ;
	ne_packhdr_t msg_buf;
};

NEINT32 ne_iocp_node_init(struct ne_client_map_iocp *iocp_map,ne_handle h_listen) ;
NEINT32 ne_init_iocp_client_map(struct ne_client_map_iocp *iocp_map,NEINT32 listen_fd);

#define _SENE_LIST(iocp_map) &((iocp_map)->wait_send_list)

static __INLINE__ NEUINT16 iocp_session_id(struct ne_client_map_iocp *iocp_map)
{
	return iocp_map->__client_map.connect_node.session_id  ;
}

static __INLINE__ NEINT32 check_send_list_empty(struct ne_client_map_iocp *iocp_map)
{
	return iocp_map->wait_send_list.next == &iocp_map->wait_send_list ;
}
static __INLINE__ NEINT32 iocp_unnotified_length(struct ne_client_map_iocp *iocp_map)
{
	return (iocp_map->total_send - iocp_map->send_len) ;
}
static __INLINE__ ne_netbuf_t * iocp_send_buf(struct ne_client_map_iocp *cli_map)
{
	return &(cli_map->__client_map.connect_node.send_buffer) ;
}

static __INLINE__ ne_netbuf_t * iocp_recv_buf(struct ne_client_map_iocp *cli_map)
{
	return &(cli_map->__client_map.connect_node.recv_buffer) ;
}

#define IOCP_MAP_FD(node) (node)->__client_map.connect_node.fd
void CALLBACK iocp_callback(DWORD dwErrorCode, DWORD dwByteCount,LPOVERLAPPED lpOverlapped) ;

//iocp_write /iocp_read ��socket������read write
NE_SRV_API NEINT32 iocp_write(struct ne_client_map_iocp *iocp_map) ;
NE_SRV_API NEINT32 iocp_read(struct ne_client_map_iocp *iocp_map) ;
NE_SRV_API NEINT32 iocp_close_client(struct ne_client_map_iocp *iocp_map, NEINT32 force) ;

//�� ne_tcp_node �����ϵķ�����Ҫ��ʹ��WSASend ������send
NE_SRV_API NEINT32 _iocp_write2sock(struct ne_tcp_node *node,void *data , size_t len);
//����һ����������Ϣ
NE_SRV_API NEINT32 _iocp_write2sock_wait(struct ne_tcp_node *node,void *data , size_t len) ;

//��client_map�ڵ��Ϸ���
NE_SRV_API NEINT32 ne_iocp_sendmsg(struct ne_client_map_iocp *iocp_map,ne_packhdr_t *msg_buf, NEINT32 flag) ;

NEINT32 check_repre_accept() ;	//�õ���ǰ��Ҫpreaccept�ĸ���
//the following function used is iocp model
NEINT32 pre_accept(struct ne_srv_node *srv_node) ;
NEINT32 iocp_accept(struct ne_client_map_iocp *node);
NEINT32 iocp_parse_msgex(struct ne_client_map_iocp *iocp_map,NENET_MSGENTRY msg_entry );
NEINT32 data_income(struct ne_client_map_iocp *pclient, DWORD dwRecvBytes );

NEINT32 update_iocp_cm(struct listen_contex *srv_root) ;
#endif
#endif
