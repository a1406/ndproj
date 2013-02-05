#ifndef  _CLIENT_MAP_H_
#define _CLIENT_MAP_H_


#include "ne_net/ne_netlib.h"

#define NE_NAME_SIZE 40


//使用自定义的发送函数
//typedef NEINT32 (*snedmsg_ext_entry)(void *owner, ne_packhdr_t *msg_buf, NEINT32 flag) ;	//define extend send function
//client 状态
/*
enum eClientStatus{
	ECS_NONE = 0 ,			//空闲
	ECS_ACCEPTED ,			//accept
	ECS_READY,				//login 
	ESC_CLOSED				//connect closed
} ;
*/
//ne_client_map 结果需要根据各种不同的服务器消息结构来定制
/*client connect map on server*/

struct ne_client_map
{
	struct ne_tcp_node connect_node ;		//socket connect node 
	struct list_head map_list ;
};

#ifdef WIN32 

#if _MSC_VER < 1300 // 1200 == VC++ 6.0
#else 
#include "winsock2.h"

#endif

struct ne_client_map_iocp ;

struct NE_OVERLAPPED_PLUS  
{
	WSAOVERLAPPED overlapped ;	
	struct ne_client_map_iocp* client_addr; //Pointer to client
};

struct ne_client_map_iocp
{
	struct ne_client_map  __client_map ;		//common client map ,MUST in first 	
	size_t			total_send;				//write buf total
	size_t			send_len;				// had been send length
	NEINT32				__wait_buffers ;		//waiting sending buffer number
	netime_t		schedule_close_time;				//set close time 
	struct NE_OVERLAPPED_PLUS  ov_read;
	struct NE_OVERLAPPED_PLUS  ov_write ;
	struct list_head wait_send_list;		// wait send queue
	WSABUF			wsa_readbuf;			// Temporary buffer used for reading
	WSABUF			wsa_writebuf;			// Temporary buffer used for writing
};

#endif

enum ePlayerState {
	EPS_NONE = 0 ,
	EPS_CONNECT,
	EPS_LOGIN
};
//header of playerdata
typedef struct player_header{
	ne_netui_handle		h_session ;		//handle of connection session
	NEUINT32			id ;
	NEINT32					status ;		//ref ePlayerState
}player_header_t ;

/*

#define CLIENT_ID(cli_info)		(cli_info)->id
#define CLIENT_NAME(cli_info)	(cli_info)->status
#define CLIENT_STATUS(cli_info)	(cli_info)->name
#define CLIENT_LIST(cli_info)	(&(cli_info)->map_list)
*/

//定义client map handle
//typedef void *ne_cli_handle;
typedef struct netui_info *ne_climap_handle ;		//定义客户端连接镜像(客户端连接到服务器时需要建立这样一个镜像)

//从clienthandle中得到相关属性
//NE_SRV_API struct ne_client_info* ne_get_client_info(ne_climap_handle cli_handle) ; 

NE_SRV_API struct list_head *get_self_list(ne_climap_handle cli_handle);	//得到client自己的链表节点

NE_SRV_API  void ne_tcpcm_init(struct ne_client_map *client_map,ne_handle h_listen) ;

NE_SRV_API size_t ne_getclient_hdr_size(NEINT32 iomod) ;

static __INLINE__ void *ne_session_getdata(ne_netui_handle session) 
{
	return (void*) (((NEINT8*)session) + session->length );
}
#endif
