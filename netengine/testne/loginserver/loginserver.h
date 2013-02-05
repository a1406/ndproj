#ifndef _LOGIN_SERVER_H__
#define _LOGIN_SERVER_H__

//#define LISTEN_PORT  (12345)
//#define MAX_CLIENT   (8096)

#define SERVER_TYPE_MAP  0x01000000

#define MAP_SERVER_NOVICE 0x00010000

/*
typedef struct logic_server_t
{
	unsigned int server_id;
	struct ne_tcp_node conn_node;
	struct logic_server_t *north;
	struct logic_server_t *south;
	struct logic_server_t *west;
	struct logic_server_t *east;	
} logic_server;
*/	
#define MAX_ADDR_LEN 128

typedef struct _map_server_t
{
	NEUINT32 server_id;
	NEINT8 addr[MAX_ADDR_LEN];
	NEUINT16 port;
	ne_handle conn_node;
	struct list_head quad_lst;
	quadbox_t box;
} map_server;

typedef struct ls_user_t
{
	NEINT32 id;
	map_server *server;
	struct list_head quad_lst;
//	struct ne_client_map *session;
	NEUINT16 sessionid;
	NEFLOAT pos_x;
	NEFLOAT pos_y;
} ls_user_t;

#endif

