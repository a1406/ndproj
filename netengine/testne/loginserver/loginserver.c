#include "ne_app/ne_app.h"
#include "ne_srvcore/client_map.h"
#include "ne_common/ne_rbtree.h"
#include "ne_quadtree/quadtree.h"
#include "loginserver.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "ne_log4c.h"
#include "demomsg.h"
#include <stddef.h>

/* todo
 * manage many types of server,
 * manage many users, use rb tree.
 */
static NEINT32 mapserver_count;
static map_server *mapserver;

static ne_rwlock id_map_lock;
static rb_node_t *userid_map = NULL;

static ne_rwlock sessionid_map_lock;
static rb_node_t *sessionid_map = NULL;

static ne_rwlock pos_map_lock;
static quadtree_t *pos_map;

static ne_rwlock mapserver_map_lock;
static quadtree_t *mapserver_map;

static log4c_category_t* mycat = NULL;	
static ne_handle listen_handle = NULL;
static NEINT32 usermsg_logout_func(ls_user_t *lsuser, ne_usermsgbuf_t *msg, ne_handle h_listen);

static lua_State *L = NULL;
static int reloadconfig(lua_State *l)
{
	LOG_DEBUG(mycat, "%s", __FUNC__);
	return (0);
}
static int lua_addnpc(lua_State *l)
{
	LOG_DEBUG(mycat, "%s", __FUNC__);
	return (0);
}

static void dump_stack(lua_State *l)
{
	int i, top, type;	
	top = lua_gettop(L);
	printf("==== dump stack =====\n");
	for (i = 1; i <= top; ++i) {
		type = lua_type(L, i);
		printf("%s\n", lua_typename(L, type));
	}
	printf("==== dump end =====\n");	
}

static int lua_init(char *filename)
{
	L = lua_open();
    luaopen_base(L);
	luaopen_io(L);
    luaopen_string(L);
    luaopen_math(L);
    luaopen_table(L);	

	lua_pop(L, 7);
	
	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) {
		error(L, "cannot run configuration file: %s", lua_tostring(L, -1));
		return (-1);
	}
	
	lua_pushcfunction(L, reloadconfig);
	lua_setglobal(L, "reload");
	lua_pushcfunction(L, lua_addnpc);
	lua_setglobal(L, "addnpc");
	
	return (0);
}

static unsigned short get_listenport()
{
	unsigned short listen_port = 0;
	lua_getglobal(L, "listen_port");
    if (!lua_isnumber(L, -1)) {
		LOG_DEBUG(mycat, "listen_port should be a number");
		return (0);
	}
	listen_port = lua_tonumber(L, -1);
	lua_pop(L, 1);	
	return listen_port;
}
static unsigned short get_maxclient()
{
	int max_client = 0;
	lua_getglobal(L, "max_client");
    if (!lua_isnumber(L, -1)) {
		LOG_DEBUG(mycat, "max_client should be a number");
		return (0);
	}
	max_client = lua_tonumber(L, -1);
	lua_pop(L, 1);	
	return max_client;
}
static NEINT32 get_startid()
{
	NEINT32 start_id = 0;
	lua_getglobal(L, "userid_start");
    if (!lua_isnumber(L, -1)) {
		LOG_DEBUG(mycat, "max_client should be a number");
		return (0);
	}
	start_id = lua_tonumber(L, -1);
	lua_pop(L, 1);	
	return start_id;
}

static NEUINT32 get_maplength()
{
	int length;
	lua_getglobal(L, "map_length");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "map_length should be a number");
		return (0);
	}
	length = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return length;
}

static NEINT32 get_mapserverconfig()
{
	int i;

	lua_getglobal(L, "mapserver_count");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "mapserver_count should be a number");
		return (-1);
	}
	mapserver_count = lua_tonumber(L, -1);
	lua_pop(L, 1);

	mapserver = (map_server *)malloc(sizeof(map_server) * mapserver_count);
	if (!mapserver) {
		LOG_ERROR(mycat, "%s: malloc fail", __FUNC__);
		return (-1);
	}

	lua_getglobal(L, "mapserver_config");
	if (!lua_istable(L, -1)) {
		LOG_ERROR(mycat, "can not get mapserver_config");		
		return (-1);
	}
	
	for (i = 0; i < mapserver_count; ++i) {
		lua_pushnumber(L, i+1);
		lua_gettable(L, -2);		
		if (!lua_istable(L, -1)) {
			LOG_ERROR(mycat, "%s %d: [%d]can not get mapserver_config", __FUNC__, __LINE__, i);					
		}
		lua_pushnumber(L, 1);
		lua_gettable(L, -2);
		if (!lua_isstring(L, -1)) {
			LOG_ERROR(mycat, "%s %d: [%d]can not get mapserver_config", __FUNC__, __LINE__, i);					
		}
			//todo use strncpy instead strcpy
		strcpy(mapserver[i].addr, lua_tostring(L, -1));
		lua_pop(L, 1);

		lua_pushnumber(L, 2);
		lua_gettable(L, -2);
		if (!lua_isnumber(L, -1)) {
			LOG_ERROR(mycat, "%s %d: [%d]can not get mapserver_config", __FUNC__, __LINE__, i);					
		}
		mapserver[i].port = lua_tonumber(L, -1);
		lua_pop(L, 2);
	}
	lua_pop(L, 1);
	
	return (0);
}

static NEINT32 sendmsg_to_mapserver(map_server *server, ne_usermsghdr_t *msg, NEINT32 flag)
{
	NEINT32 ret;

	if (!server)
		return (-1);
	NE_USERMSG_MAXID(msg) = MAXID_SERVER_SERVER;

	if(!ne_connector_valid((ne_netui_handle)server->conn_node)) {
		LOG_ERROR(mycat, "%s %d: mapserver_index[%d] not connect",
			__FUNC__, __LINE__, server->server_id);
		return (-1);
	}
	ret = ne_sessionmsg_sendex(server->conn_node, msg, flag);			
	return ret;
}

#ifdef SINGLE_THREAD_MOD
static NEINT32 test_accept(NEUINT16 sessionid, SOCKADDR_IN *addr, ne_handle listener)
{
	LOG_DEBUG(mycat, "%s %d: sessionid[%hx]", __FUNC__, __LINE__, sessionid);
	return (0);
}
static void test_disconnect(NEUINT16 sessionid, ne_handle h_listen)
{
	ls_user_t *lsuser;
	rb_node_t *rb_data;
	ne_usermsgbuf_t buf;
	struct listen_contex *lc;
	struct cm_manager *mamager;
	struct ne_client_map *session;
//	buf.msg_hdr.packet_hdr = NE_PACKHDR_INIT;
	ne_usermsghdr_init(&buf.msg_hdr);	
	lc = (struct listen_contex *) h_listen ;
	mamager = ne_listensrv_get_cmmamager(h_listen);
	ne_assert(mamager) ;
	session = (struct ne_client_map *)mamager->lock(mamager, sessionid) ;
	if(session) {
		LOG_DEBUG(mycat, "%s %d: sessionid[%hx]", __FUNC__, __LINE__, sessionid);
		ne_wrlock(&sessionid_map_lock);
		rb_data = rb_search(sessionid, sessionid_map);
		if (!rb_data || !rb_data->data) {
			ne_rwunlock(&sessionid_map_lock);		
			LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid", __FUNC__, __LINE__);
			return;
		}
		lsuser = (ls_user_t *)rb_data->data;
		sessionid_map = rb_erase(sessionid, sessionid_map);
		ne_rwunlock(&sessionid_map_lock);
		
		ne_wrlock(&id_map_lock);
		userid_map = rb_erase(lsuser->id, userid_map);
		ne_rwunlock(&id_map_lock);

		ne_wrlock(&pos_map_lock);
		list_del(&lsuser->quad_lst);
		ne_rwunlock(&pos_map_lock);
	
		usermsg_logout_func(lsuser, &buf, h_listen);
		free(lsuser);

		tcp_client_close_fin(session, 1);
		mamager->unlock(mamager, sessionid) ;
	} else {
		LOG_ERROR(mycat, "%s %d: can not find session by sessionid[%d]", __FUNC__, __LINE__, sessionid);			
	}
}
#else
static NEINT32 test_accept(void *data, SOCKADDR_IN *addr, ne_handle listener)
{
	struct ne_client_map* session = (struct ne_client_map *)data;

	ne_connect_level_set((ne_handle)session, EPL_LOGIN);
	
	LOG_DEBUG(mycat, "%s %d: sessionid[%hx]", __FUNC__, __LINE__,
		session->connect_node.session_id);
	
    return (0);
}
static NEINT32 pre_deaccept(struct ne_client_map *session, ne_handle listner)
{
	return (0);
}
//这里不能锁，已经锁住的
static NEINT32 test_deaccept(void *data, ne_handle listener)
{
	ls_user_t *lsuser;
	rb_node_t *rb_data;
	NEUINT16 sessionid;
	struct ne_client_map* session = (struct ne_client_map *)data;	
	ne_usermsgbuf_t buf;
//	buf.msg_hdr.packet_hdr = NE_PACKHDR_INIT;
	ne_usermsghdr_init(&buf.msg_hdr);	
	sessionid = session->connect_node.session_id;
	
	LOG_DEBUG(mycat, "%s %d: sessionid[%hx]", __FUNC__, __LINE__, sessionid);
	ne_wrlock(&sessionid_map_lock);
	rb_data = rb_search(sessionid, sessionid_map);
	if (!rb_data) {
		ne_rwunlock(&sessionid_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid", __FUNC__, __LINE__);
		return 0;
	}
	lsuser = (ls_user_t *)rb_data->data;
	if (!lsuser) {
		ne_rwunlock(&sessionid_map_lock);		
		return (0);
	}
	sessionid_map = rb_erase(sessionid, sessionid_map);
	ne_rwunlock(&sessionid_map_lock);
		
	ne_wrlock(&id_map_lock);
	userid_map = rb_erase(lsuser->id, userid_map);
	ne_rwunlock(&id_map_lock);

	ne_wrlock(&pos_map_lock);
	list_del(&lsuser->quad_lst);
	ne_rwunlock(&pos_map_lock);
	
	usermsg_logout_func(lsuser, &buf, listener);
	free(lsuser);
	return (0);
}
#endif

static NEINT32 test_usermsg_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle listener)
{
	LOG_DEBUG(mycat, "%s %d: sessionid[%hx]", __FUNC__, __LINE__, sessionid);	
}

static int send_leave_map(ls_user_t *ls_user, NEUINT8 sendto_user)
{
	NEINT32 ret = -1;	
	ne_usermsgbuf_t buf;
	ne_usermsghdr_init(&buf.msg_hdr);		
	if (!listen_handle) {
		LOG_ERROR(mycat, "%s %d: do not have listen_handle", __FUNC__, __LINE__);				
		return (-1);
	}
	NE_USERMSG_PARAM(&buf) = ls_user->id;
	NE_USERMSG_MAXID(&buf) = MAXID_SERVER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_LEAVE_MAP;
	ret = sendmsg_to_mapserver(ls_user->server, (struct ne_usermsghdr_t *)&buf, ESF_URGENCY);
	return (0);
}

static int send_change_map(ls_user_t *ls_user, user_info_t *user_info, NEUINT8 sendto_user)
{
	NEINT32 ret = -1;
	NEUINT16 sessionid;
	ne_usermsgbuf_t buf;
	login_ack_t *data = (login_ack_t *)buf.data;
//	buf.msg_hdr.packet_hdr = NE_PACKHDR_INIT;
	ne_usermsghdr_init(&buf.msg_hdr);	
	if (!listen_handle) {
		LOG_ERROR(mycat, "%s %d: do not have listen_handle", __FUNC__, __LINE__);				
		return (-1);
	}

	sessionid = ls_user->sessionid;
		
	data->result = 0;
	memcpy(&data->user_info, user_info, sizeof(user_info_t));

	NE_USERMSG_LEN(&buf) += sizeof(login_ack_t);			
	NE_USERMSG_PARAM(&buf) = ne_time();
	NE_USERMSG_MAXID(&buf) = MAXID_SERVER_SERVER;
	NE_USERMSG_MINID(&buf) = MSG_USER_CHG_MAP;
	NE_USERMSG_ENCRYPT(&buf) = 0;
			
	ret = sendmsg_to_mapserver(ls_user->server,	(struct ne_usermsghdr_t *)&buf, 0);

	if (sendto_user == 1) {
		NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
		NE_USERMSG_MINID(&buf) = MSG_USER_LOGIN_ACK ;
		ret = NE_SENDEX(sessionid, (struct ne_usermsghdr_t *)&buf,
			ESF_ENCRYPT, listen_handle);
	}
	return (0);
}

static NEINT32 usermsg_logout_func(ls_user_t *lsuser, ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	send_leave_map(lsuser, 0);
	return (0);
}

static void change_map(data_t data, void *param)
{
	ls_user_t *ls_user;
	ls_user = (ls_user_t *)data;
//todo
//	send_change_map(ls_user, 0);
}

static NEINT32 alluser_change_map(NEINT32 mapserver_id)
{
	ne_rdlock(&id_map_lock);
	rb_travel(userid_map, change_map, (void *)mapserver_id);
	ne_rwunlock(&id_map_lock);
	return (0);
}
#ifdef SINGLE_THREAD_MOD
static NEINT32 usermsg_userscene_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	NEINT32 ret = -1;
	ls_user_t *ls_user;
	rb_node_t *rb_data;
	quadbox_t box;
	struct list_head *quad_ret[10];	
	int i, n;
	map_server *server;

	ne_rdlock(&sessionid_map_lock);
	rb_data = rb_search(sessionid, sessionid_map);
	if (!rb_data) {
		ne_rwunlock(&sessionid_map_lock);		
//		LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid[%d]",
//			__FUNC__, __LINE__, sessionid);
		return (-1);
	}
	ls_user = (ls_user_t *)rb_data->data;
	ne_rwunlock(&sessionid_map_lock);

//	LOG_DEBUG(mycat, "%s %d: user scene req id[%d] x[%lf] y[%lf]",
//		__FUNC__, __LINE__, ls_user->id, ls_user->pos_x, ls_user->pos_y);
	
	box._xmin = ls_user->pos_x;
	box._ymin = ls_user->pos_y;
	box._xmax += 10.0;
	box._ymax += 10.0;

	n = 0;
	ne_rdlock(&mapserver_map_lock);
	quadtree_search(mapserver_map, &box, quad_ret, &n, 10);
	ne_rwunlock(&mapserver_map_lock);
	for (i = 0; i < n; ++i) {
		server = (map_server *)((char *)(quad_ret[i]->next) - offsetof(map_server, quad_lst));
		if(!ne_connector_valid((ne_netui_handle)server->conn_node)) {
			LOG_ERROR(mycat, "%s %d: mapserver_index[%lu] not connect",
				__FUNC__, __LINE__, server->server_id);
			return (-1);
		}
		NE_USERMSG_PARAM(msg) = ls_user->id;		
		NE_USERMSG_MAXID(msg) = MAXID_SERVER_SERVER;
		ne_sessionmsg_sendex(server->conn_node, (ne_usermsghdr_t *)msg, 0);			
	}
	ret = 0;
	return (ret);
}

static NEINT32 usermsg_useraction_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	NEINT32 ret = -1;
	user_action_t *req;
	ls_user_t *ls_user;
	rb_node_t *rb_data;
	quadbox_t box;
	struct list_head *quad_ret[1];	
	user_info_t user_info;
	int n;
	map_server *server;

	if (NE_USERMSG_DATALEN(msg) != sizeof(user_action_t)) {
		LOG_WARN(mycat, "%s %d: data len wrong, ignore it", __FUNC__, __LINE__);					
		return (-1);
	}

	req = (user_action_t *)msg->data;
//todo  check action and x y;

	ne_rdlock(&sessionid_map_lock);
	rb_data = rb_search(sessionid, sessionid_map);
	if (!rb_data || !rb_data->data) {
		ne_rwunlock(&sessionid_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid[%d]",
			__FUNC__, __LINE__, sessionid);
		return (-1);
	}
	ls_user = (ls_user_t *)rb_data->data;
	ne_rwunlock(&sessionid_map_lock);

	LOG_DEBUG(mycat, "%s %d: user action id[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, ls_user->id, req->x, req->y);

		//todo modify maptree
	ls_user->pos_x = req->x;
	ls_user->pos_y = req->y;	

	box._xmin = req->x;
	box._xmax = req->x;
	box._ymin = req->y;
	box._ymax = req->y;
	n = 0;
	ne_rdlock(&mapserver_map_lock);
	quadtree_search(mapserver_map, &box, quad_ret, &n, 1);
	assert(n == 1);
	server = (map_server *)((char *)(quad_ret[0]->next) - offsetof(map_server, quad_lst));
	ne_rwunlock(&mapserver_map_lock);

	ne_wrlock(&pos_map_lock);
	list_del(&ls_user->quad_lst);
	box._xmin = ls_user->pos_x;
	box._ymin = ls_user->pos_y;
	box._xmax = ls_user->pos_x;
	box._ymax = ls_user->pos_y;
	if (quadtree_insert(pos_map, &ls_user->quad_lst, &box) == NULL) {
		LOG_ERROR(mycat, "%s %d: add to quadtree fail", __FUNC__, __LINE__);
	}
	ne_rwunlock(&pos_map_lock);
	
	if (server != ls_user->server) {
		//send leave map
		send_leave_map(ls_user, 0);
		//send change map
		memcpy(user_info.base.rolename, req->rolename, sizeof(user_info.base.rolename));
		user_info.level = 1;
		user_info.id = sessionid;
		user_info.hp = 100;
		user_info.mp = 0;
		user_info.attack = 20;
		user_info.recover = 0;
		user_info.base.pos_x = req->x;
		user_info.base.pos_y = req->y;
		user_info.money = 0;
		user_info.experience = 0;
		user_info.addedattack = 0;
		user_info.addedrecover = 0;
		user_info.rollcall = 0;
		ls_user->server = server;
		send_change_map(ls_user, &user_info, 0);
	} else {
		NE_USERMSG_PARAM(msg) = ls_user->id;
		NE_USERMSG_MAXID(msg) = MAXID_SERVER_SERVER;
		NE_USERMSG_ENCRYPT(msg) = 0;
		ret = sendmsg_to_mapserver(ls_user->server, (struct ne_usermsghdr_t *)msg, 0);
	}
	return (0);
}
static NEINT32 usermsg_usermove_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	NEINT32 ret = -1;
	user_move_req_t *req;
	ls_user_t *ls_user;
	rb_node_t *rb_data;
	quadbox_t box;
	struct list_head *quad_ret[1];	
	int n;
	map_server *server;

	if (NE_USERMSG_DATALEN(msg) != sizeof(user_move_req_t)) {
		LOG_WARN(mycat, "%s %d: data len wrong, ignore it", __FUNC__, __LINE__);					
		return (-1);
	}

	req = (user_move_req_t *)msg->data;
//todo  check action and x y;

	box._xmin = req->x - SCENE_LENGTH;
	box._xmax = req->x + SCENE_LENGTH;
	box._ymin = req->y - SCENE_LENGTH;
	box._ymax = req->y + SCENE_LENGTH;
	ne_rdlock(&sessionid_map_lock);
	rb_data = rb_search(sessionid, sessionid_map);
	if (!rb_data || !rb_data->data) {
		ne_rwunlock(&sessionid_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid[%d]",
			__FUNC__, __LINE__, sessionid);
		return (-1);
	}
	ls_user = (ls_user_t *)rb_data->data;
	ne_rwunlock(&sessionid_map_lock);

	LOG_DEBUG(mycat, "%s %d: user move id[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, ls_user->id, req->x, req->y);
	
	ls_user->pos_x = req->x;
	ls_user->pos_y = req->y;	
	
	n = 0;
	ne_rdlock(&mapserver_map_lock);
	quadtree_search(mapserver_map, &box, quad_ret, &n, 1);
	assert(n == 1);
	server = (map_server *)((char *)(quad_ret[0]->next) - offsetof(map_server, quad_lst));
	ne_rwunlock(&mapserver_map_lock);

		//todo change map
	NE_USERMSG_PARAM(msg) = ls_user->id;
	NE_USERMSG_MAXID(msg) = MAXID_SERVER_SERVER;
	NE_USERMSG_ENCRYPT(msg) = 0;
	ret = sendmsg_to_mapserver(ls_user->server, (struct ne_usermsghdr_t *)msg, 0);
	return (0);
}

static NEINT32 usermsg_login_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	static NEINT32 start_id = -1;
	NEINT32 ret = -1;
	user_info_t user_info;
	struct ne_client_map *session;
	ls_user_t *ls_user;
	struct listen_contex *lc;
	struct cm_manager *mamager;
	login_req_t *login_data;
	struct list_head *quad_ret[1];	
	int i, n;
	map_server *server;
	
	quadbox_t pos_box = {0.0, 0.0, PLAYER_LENGTH, PLAYER_LENGTH};

	if (start_id < 0)
		start_id = get_startid();

	if (NE_USERMSG_DATALEN(msg) != sizeof(login_req_t)) {
		LOG_WARN(mycat, "%s %d: data len wrong, ignore it", __FUNC__, __LINE__);					
		return (-1);
	}
	
	ls_user = malloc(sizeof(ls_user_t));
	if (!ls_user) {
		LOG_ERROR(mycat, "%s %d: malloc fail", __FUNC__, __LINE__);					
		return (-1);
	}

	lc = (struct listen_contex *) h_listen ;
	mamager = ne_listensrv_get_cmmamager(h_listen);
	
	ne_assert(mamager) ;
	session = (struct ne_client_map *)mamager->lock(mamager, sessionid) ;
	if(session) {
		if(ne_connector_valid((ne_netui_handle) session) && sessionid == session->connect_node.session_id) {
			login_data = (login_req_t *)msg->data;
			memcpy(user_info.base.rolename, login_data->username, sizeof(user_info.base.rolename));
			user_info.level = 1;
			user_info.id = start_id;
			user_info.hp = 100;
			user_info.mp = 0;
			user_info.attack = 20;
			user_info.recover = 0;
			user_info.base.pos_x = 0.0;
			user_info.base.pos_y = 0.0;
			user_info.money = 0;
			user_info.experience = 0;
			user_info.addedattack = 0;
			user_info.addedrecover = 0;
			user_info.rollcall = 0;			
			user_info.base.type = USER_BOY;
			
			ls_user->id = start_id;

			n = 0;
			ne_rdlock(&mapserver_map_lock);
			quadtree_search(mapserver_map, &pos_box, quad_ret, &n, 1);
			ne_rwunlock(&mapserver_map_lock);
			if (n != 1)
				ls_user->server = NULL;
			else {
				server = (map_server *)((char *)(quad_ret[0]->next) - offsetof(map_server, quad_lst));
				if(!ne_connector_valid((ne_netui_handle)server->conn_node)) {
					LOG_ERROR(mycat, "%s %d: mapserver_index[%lu] not connect",
						__FUNC__, __LINE__, server->server_id);
					return (-1);
				}
				ls_user->server = server;
			}
			ls_user->sessionid = sessionid;
			ls_user->pos_x = user_info.base.pos_x;
			ls_user->pos_y = user_info.base.pos_y;
			ne_wrlock(&id_map_lock);
			userid_map = rb_insert(ls_user->id,	(data_t)ls_user, userid_map);			
			ne_rwunlock(&id_map_lock);
	
			ne_wrlock(&sessionid_map_lock);
			sessionid_map = rb_insert(sessionid, (data_t)ls_user, sessionid_map);			
			ne_rwunlock(&sessionid_map_lock);

			ne_wrlock(&pos_map_lock);
			if (quadtree_insert(pos_map, &ls_user->quad_lst, &pos_box) == NULL) {
				LOG_ERROR(mycat, "%s %d: add to quadtree fail", __FUNC__, __LINE__);
			}
			ne_rwunlock(&pos_map_lock);
			++start_id;
			
			LOG_DEBUG(mycat, "%s %d: user login id[%d] serverid[%d] sessionid[%hx]",
				__FUNC__, __LINE__, ls_user->id,
				ls_user->server ? ls_user->server->server_id : -1,
				ls_user->sessionid);
		} else {
			LOG_ERROR(mycat, "%s %d: session is not valid sessionid[%d]", __FUNC__, __LINE__, sessionid);			
		}
		mamager->unlock(mamager, sessionid) ;
		send_change_map(ls_user, &user_info, 1);
	} else {
		LOG_ERROR(mycat, "%s %d: can not find session by sessionid[%d]", __FUNC__, __LINE__, sessionid);			
	}
	return (0);
}

static NEINT32 usermsg_normal_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	NEINT32 ret = -1;
//	struct ne_client_map *session ;
	ls_user_t *ls_user;
	rb_node_t *rb_data;
//	struct listen_contex *lc;
//	struct cm_manager *mamager;

	ne_rdlock(&sessionid_map_lock);
	rb_data = rb_search(sessionid, sessionid_map);
	if (!rb_data) {
		ne_rwunlock(&sessionid_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid[%d]",
			__FUNC__, __LINE__, sessionid);
		return (-1);
	}
	ls_user = (ls_user_t *)rb_data->data;
	ne_rwunlock(&sessionid_map_lock);

	LOG_DEBUG(mycat, "%s %d: user normal id[%d] maxid[%hx] minid[%hx]",
		__FUNC__, __LINE__, ls_user->id, NE_USERMSG_MAXID(msg), NE_USERMSG_MINID(msg));
	
	NE_USERMSG_PARAM(msg) = ls_user->id;
	NE_USERMSG_MAXID(msg) = MAXID_SERVER_SERVER;
	NE_USERMSG_ENCRYPT(msg) = 0;
	ret = sendmsg_to_mapserver(ls_user->server,	(struct ne_usermsghdr_t *)msg, 0);
	
	return (0);
}
#else
static NEINT32 usermsg_login_func(struct ne_client_map *session, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	static NEINT32 index = 0;
	NEINT32 ret = -1;
	user_info_t user_info;
	ls_user_t *ls_user;
	NEUINT16 sessionid;
	
	ls_user = malloc(sizeof(ls_user_t));
	if (!ls_user) {
		LOG_ERROR(mycat, "%s %d: malloc fail", __FUNC__, __LINE__);					
		return (-1);
	}

	if(session) {
		if(ne_connector_valid((ne_netui_handle) session)) {
			sessionid = session->connect_node.session_id;
			sprintf(user_info.base.rolename, "testuser%d", index++);
			user_info.level = 1;
			user_info.id = index;
			user_info.hp = 100;
			user_info.mp = 0;
			user_info.attack = 20;
			user_info.recover = 0;
			user_info.base.pos_x = 0;
			user_info.base.pos_y = 0;
			user_info.money = 0;
			user_info.experience = 0;
			user_info.addedattack = 0;
			user_info.addedrecover = 0;
			user_info.rollcall = 0;			

			ls_user->id = index;
			ls_user->mapserver_index = 0;
			ls_user->session = session;

			ne_wrlock(&id_map_lock);
			userid_map = rb_insert(ls_user->id,	(data_t)ls_user, userid_map);
			ne_wrunlock(&id_map_lock);
				
			ne_wrlock(&sessionid_map_lock);
			sessionid_map = rb_insert(sessionid, (data_t)ls_user, sessionid_map);			
			ne_wrunlock(&sessionid_map_lock);

			LOG_DEBUG(mycat, "%s %d: user login id[%d] serverid[%d] session[%p] sessionid[%hx]",
				__FUNC__, __LINE__, ls_user->id,
				ls_user->mapserver_index,
				ls_user->session,
				ls_user->session->connect_node.session_id);
		} else {
			LOG_ERROR(mycat, "%s %d: session is not valid sessionid[%d]", __FUNC__, __LINE__, sessionid);			
		}
	} else {
		LOG_ERROR(mycat, "%s %d: can not find session by sessionid[%d]", __FUNC__, __LINE__, sessionid);			
	}
	send_change_map(ls_user, &user_info, 1);
	return (0);
}

static NEINT32 usermsg_normal_func(struct ne_client_map *session, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	NEINT32 ret = -1;
	ls_user_t *ls_user;
	rb_node_t *rb_data;

	if (!session)
		return (0);
	
	NEUINT16 sessionid = session->connect_node.session_id;

	ne_rdlock(&sessionid_map_lock)
	rb_data = rb_search(sessionid, sessionid_map);
	if (!rb_data) {
		ne_rwunlock(&sessionid_map_lock)			
		LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid[%d]",
			__FUNC__, __LINE__, sessionid);
		return (-1);
	}
	ls_user = (ls_user_t *)rb_data->data;
	ne_rwunlock(&sessionid_map_lock)	

	if (session != ls_user->session) {
		LOG_ERROR(mycat, "%s %d: session is not valid sessionid[%d]",
			__FUNC__, __LINE__, sessionid);			
	}

	if(ne_connector_valid((ne_netui_handle) session)
		&& sessionid == session->connect_node.session_id
		&& ls_user->mapserver_index >= 0
		&& ls_user->mapserver_index < mapserver_count) {
		NE_USERMSG_PARAM(msg) = ls_user->id;
		NE_USERMSG_MAXID(msg) = MAXID_SERVER_SERVER;
		NE_USERMSG_ENCRYPT(msg) = 0;
		ret = sendmsg_to_mapserver(ls_user->mapserver_index,
			(struct ne_usermsghdr_t *)msg, 0);
	} else {
		LOG_ERROR(mycat, "%s %d: session is not valid sessionid[%d]", __FUNC__, __LINE__, sessionid);			
	}
	return (0);
}
#endif
static NEINT32 server_reg_func(ne_handle handle, ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	int i;
	reg_mapserver_t *data = (reg_mapserver_t *)msg->data;

	for (i = 0; i < mapserver_count; ++i) {
		if (mapserver[i].conn_node == handle) {
			mapserver[i].box = data->box;
			mapserver[i].server_id = data->id;
			ne_wrlock(&mapserver_map_lock);
			if (quadtree_insert(mapserver_map, &mapserver[i].quad_lst, &data->box) == NULL) {
				LOG_ERROR(mycat, "%s %d: add to quadtree fail", __FUNC__, __LINE__);
			}
			ne_rwunlock(&mapserver_map_lock);

			LOG_DEBUG(mycat, "%s %d: server add mapserver id[%d] xmin[%lf] xmax[%lf] ymin[%lf] ymax[%lf]",
				__FUNC__, __LINE__, data->id, data->box._xmin, data->box._xmax, data->box._ymin, data->box._ymax);
			
			break;
		}
	}
	return (0);
}

static NEINT32 server_moveto_newmap(ne_handle handle, ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	rb_node_t *rb_data;
	ls_user_t *ls_user;
	NEUINT32 userid = NE_USERMSG_PARAM(msg);
	moveto_newmap_t *data = (moveto_newmap_t *)msg->data;
	
	if (NE_USERMSG_LEN(msg) != NE_USERMSG_HDRLEN + sizeof(moveto_newmap_t)) {
		LOG_ERROR(mycat, "%s %d: msg len wrong", __FUNC__, __LINE__);		
		return (-1);
	}

	if (userid != data->user_info.id) {
		LOG_ERROR(mycat, "%s %d: userid wrong", __FUNC__, __LINE__);				
		return (-1);
	}
	
	ne_rdlock(&id_map_lock);	
	rb_data = rb_search(userid, userid_map);
	if (!rb_data) {
		ne_rwunlock(&id_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid", __FUNC__, __LINE__);
		return (-1);
	}
	ls_user = (ls_user_t *)rb_data->data;
	ne_rwunlock(&id_map_lock);

	LOG_DEBUG(mycat, "%s %d: server moveto newmap id[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, ls_user->id, ls_user->pos_x, ls_user->pos_y);
	
	if (!listen_handle) {
		LOG_ERROR(mycat, "%s %d: do not have listen_handle", __FUNC__, __LINE__);				
		return (-1);
	}
	
//sendto cur mapserver MSG_USER_LEAVE_MAP
//	ls_user->mapserver_index = data->id;
//sendto new mapserver MSG_USER_CHG_MAP

	return (0);
}

static NEINT32 server_normal_ack(ne_handle handle, ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	NEINT32 ret;
	NEUINT16 sessionid;
	rb_node_t *rb_data;
	NEUINT32 userid = NE_USERMSG_PARAM(msg);
	ls_user_t *ls_user;
//	ne_usermsgbuf_t buf;

	ne_rdlock(&id_map_lock);
	rb_data = rb_search(userid, userid_map);
	if (!rb_data) {
		ne_rwunlock(&id_map_lock);		
		LOG_WARN(mycat, "%s %d: can not find rb_data from sessionid, maybe disconnected", __FUNC__, __LINE__);
		return (0);
	}
	ls_user = (ls_user_t *)rb_data->data;
	sessionid = ls_user->sessionid;
	ne_rwunlock(&id_map_lock);

	if (!listen_handle) {
		LOG_ERROR(mycat, "%s %d: do not have listen_handle", __FUNC__, __LINE__);				
		return (-1);
	}

	LOG_DEBUG(mycat, "%s %d: server normal ack id[%d] maxid[%hx] minid[%hx]",
		__FUNC__, __LINE__, ls_user->id, NE_USERMSG_MAXID(msg), NE_USERMSG_MINID(msg));
	
//	memcpy(&buf, msg, NE_USERMSG_LEN(msg));
	ret = NE_SENDEX(sessionid,
		(ne_usermsghdr_t *)msg, ESF_ENCRYPT, listen_handle);
	return (0);
}

static NEINT32 server_brd_torange_func(ne_handle handle, ne_usermsgbuf_t *msg, ne_handle notuse)
{
	struct list_head *retlist[MAX_QUAD_SEARCH];
	struct list_head *pos;
	ls_user_t *ls_user;
	NEINT32 index, i, ret;
//	ne_usermsgbuf_t buf;
	brd_torange_info_t *bd_data = (brd_torange_info_t *)(msg->data + NE_USERMSG_PARAM(msg));	
	if (!listen_handle) {
		LOG_ERROR(mycat, "%s %d: do not have listen_handle", __FUNC__, __LINE__);				
		return (-1);
	}
	
	NE_USERMSG_LEN(msg) -= (sizeof(brd_torange_info_t));
	NE_USERMSG_MAXID(msg) = bd_data->real_maxid;
	NE_USERMSG_MINID(msg) = bd_data->real_minid;

	index = 0;
	ne_rdlock(&pos_map_lock);	
	quadtree_search(pos_map, &(bd_data->box), retlist, &index, MAX_QUAD_SEARCH);

	LOG_DEBUG(mycat, "%s %d: server broadcast maxid[%hx] minid[%hx], %d players found",
		__FUNC__, __LINE__, bd_data->real_maxid, bd_data->real_minid, index);

	for (i = 0; i < index; ++i) {	
		list_for_each(pos, retlist[i]) {
//			memcpy(&buf, msg, NE_USERMSG_LEN(msg));  //because it will encrypt
			ls_user = list_entry(pos, ls_user_t, quad_lst);
			ret = NE_SENDEX(ls_user->sessionid,
				(ne_usermsghdr_t *)msg, ESF_ENCRYPT_LATER | ESF_WRITEBUF, listen_handle);
		}
	}
	ne_rwunlock(&pos_map_lock);
	return (0);
}

static NEINT32 server_brd_touser_func(ne_handle handle, ne_usermsgbuf_t *msg, ne_handle notuse)
{
	NEINT32 i, n, ret;
	NEUINT32 userid;
	rb_node_t *rb_data;
	ls_user_t *ls_user;
	broadcast_info_t *bd_data = (broadcast_info_t *)(msg->data + NE_USERMSG_PARAM(msg));
	NEUINT16 sessionid;
//	ne_usermsgbuf_t buf;
	
	if (!listen_handle) {
		LOG_ERROR(mycat, "%s %d: do not have listen_handle", __FUNC__, __LINE__);				
		return (-1);
	}
	
	NE_USERMSG_LEN(msg) -= (sizeof(broadcast_info_t) + sizeof(int) * bd_data->usernum);
	NE_USERMSG_MAXID(msg) = bd_data->real_maxid;
	NE_USERMSG_MINID(msg) = bd_data->real_minid;

	LOG_DEBUG(mycat, "%s %d: server broadcast maxid[%hx] minid[%hx]",
		__FUNC__, __LINE__, bd_data->real_maxid, bd_data->real_minid);
	
	for (i = 0; i < bd_data->usernum; ++i) {
//		memcpy(&buf, msg, NE_USERMSG_LEN(msg));  //because it will encrypt
		userid = bd_data->userid[i];
		ne_rdlock(&id_map_lock);
		rb_data = rb_search(userid, userid_map);
		if (!rb_data) {
			ne_rwunlock(&id_map_lock);			
			LOG_ERROR(mycat, "%s %d: can not find rb_data from sessionid", __FUNC__, __LINE__);
			continue;
		}
		ls_user = (ls_user_t *)rb_data->data;
		sessionid = ls_user->sessionid;
		ne_rwunlock(&id_map_lock);

		LOG_DEBUG(mycat, "%s %d: server broad to user id[%d]",
			__FUNC__, __LINE__, ls_user->id);

		ret = NE_SENDEX(sessionid,
			(ne_usermsghdr_t *)msg, ESF_ENCRYPT_LATER | ESF_WRITEBUF, listen_handle);
//			(ne_usermsghdr_t *)&buf, ESF_URGENCY | ESF_ENCRYPT, listen_handle);
	}

	return (0);
}

NEINT32 logic_msg_entry(ne_handle handle, ne_usermsgbuf_t *msg, ne_handle listener)
{
	NEINT8 *buf;

	LOG_DEBUG(mycat, "%s: received message %d ", __FUNC__, NE_USERMSG_MAXID(msg));
	
	if(NE_USERMSG_LEN(msg)>0) {
		buf = (NEINT8 *)msg + sizeof(*msg);
	}
	return 0 ;
}

void init_map_server(map_server *l)
{
//	memset(l, 0, sizeof(*l));
	INIT_LIST_HEAD(&l->quad_lst);
	
	l->conn_node = ne_object_create("tcp-connector");
	if (!l->conn_node) {
		LOG_ERROR(mycat, "%s %d: ne_object_create ret fail[%s]",
			__FUNC__, __LINE__, ne_last_error());
		return;
	}

	l->server_id = -1;
	if (ne_msgtable_create(l->conn_node, MSG_CLASS_NUM, MAXID_BASE) != 0) {
		LOG_ERROR(mycat, "%s %d: ne_msgtable_create ret fail[%s]",
			__FUNC__, __LINE__, ne_last_error());
		return;
	}

	ne_msgentry_install(l->conn_node, logic_msg_entry,
		MAXID_SYS, SYM_ECHO, 0) ;	
	ne_msgentry_install(l->conn_node, server_brd_touser_func,
		MAXID_SERVER_SERVER, MSG_BROADCAST_TOUSER, 0);
	ne_msgentry_install(l->conn_node, server_brd_torange_func,
		MAXID_SERVER_SERVER, MSG_BROADCAST_TORANGE,	0);	
	ne_msgentry_install(l->conn_node, server_normal_ack,
		MAXID_SERVER_SERVER, MSG_USER_CHG_MAP_RET, 0);		
	ne_msgentry_install(l->conn_node, server_normal_ack,
		MAXID_USER_SERVER, MSG_USER_SCENE_ACK, 0);
	ne_msgentry_install(l->conn_node, server_normal_ack,
		MAXID_USER_SERVER, MSG_MONSTER_SCENE_ACK, 0);		
	ne_msgentry_install(l->conn_node, server_moveto_newmap,
		MAXID_SERVER_SERVER, MSG_USER_LEAVE_MAP_RET, 0);
	ne_msgentry_install(l->conn_node, server_reg_func,
		MAXID_SERVER_SERVER, MSG_SERVER_REG, 0);
}


static void *mapserver_thread(void *param)
{
	NEINT32 ret;
//	ne_usermsgbuf_t sendmsg = NE_USERMSG_INITILIZER;
	map_server *mapserver = (map_server *)param;

	if (!mapserver) {
		LOG_ERROR(mycat, "%s %d: do not have mapserver", __FUNC__, __LINE__) ;
		return;
	}

	ne_handle mapserver_handle = NULL;
	for (;;)
	{
		if (!mapserver_handle) {
			if (ne_connector_openex(mapserver->conn_node, mapserver->addr, mapserver->port)) {
				neprintf(_NET("%s %d: connect error :%s!\n"), __FUNC__, __LINE__, ne_last_error()) ;
				LOG_ERROR(mycat, "%s %d: connect error :%s!",
					__FUNC__, __LINE__, ne_last_error()) ;
				ne_sleep(1 * 1000);
				continue;
			} else {
				mapserver_handle = mapserver->conn_node;
				LOG_DEBUG(mycat, "connect success!");
			}
		}

		ret = ne_connector_update(mapserver_handle, 10*1000);
/*		ret = ne_connector_waitmsg(mapserver_handle, (ne_packetbuf_t *)&sendmsg, 10*1000);

		if(ret > 0) {
			ne_translate_message(mapserver_handle, (ne_packhdr_t*) &sendmsg) ;
		}
		else if(-1==ret) {
*/
		if (ret < 0 || (ret == 0 && (struct ne_tcp_node *)mapserver_handle->myerrno != NEERR_SUCCESS)) {
			neprintf(_NET("closed by remote ret = 0\n")) ;
			LOG_DEBUG(mycat, "%s %d: closed by remote[%d]!",
				__FUNC__, __LINE__,
				(struct ne_tcp_node *)mapserver_handle->myerrno);
			mapserver_handle = NULL;
			ne_connector_reset(mapserver->conn_node);			
			continue ;
		}
		else {
//			neprintf(_NET("wait time out ret = %d\npress any key to continue\n"), ret) ;
		}
	}

	LOG_DEBUG(mycat, "%s %d: mapserver_thread end", __FUNC__, __LINE__) ;
	return (0);
}

static void show_ls_userinfo(data_t data, void *param)
{
	ls_user_t *ls_user = (ls_user_t *)data;
	neprintf(_NET("id[%d] serverid[%d] sessionid[%hx] x[%lf] y[%lf]\n"),
		ls_user->id,
		ls_user->server->server_id,
		ls_user->sessionid,
		ls_user->pos_x,
		ls_user->pos_y);
}

int main(int argc, char *argv[])
{
    char ch;
    int ret;
	int i;
	struct listen_contex *lc;
	quadbox_t mapbox;
	
	log4c_init();
	mycat = log4c_category_get("loginserver");	
	LOG_DEBUG(mycat, "Debugging app 1 - loop %d", 10);
#if 0
	start_server(argc, argv);
	wait_services() ;
	printf_dbg("leave wait server\n") ;
	end_server(0) ;
	printf_dbg("program exit from main\n Press ANY KEY to continue\n") ;
	getch();
#else	
	
//	start_server(argc, argv);

	ne_rwlock_init(&id_map_lock);	
	ne_rwlock_init(&sessionid_map_lock);
	ne_rwlock_init(&pos_map_lock);
	ne_rwlock_init(&mapserver_map_lock);	
	ret = init_module();

	lua_init("config.lua");

	i = get_maplength();
	mapbox._xmin = mapbox._ymin = 0;
	mapbox._xmax = mapbox._ymax = i;
	pos_map = quadtree_create(mapbox, 6, 0.1);

	mapserver_map = quadtree_create(mapbox, QTREE_DEPTH_MIN, QBOX_OVERLAP_MIN);
	
	ret = create_rsa_key();
	listen_handle = ne_object_create("listen-ext") ;
#ifdef NE_DEBUG
	ne_set_connection_timeout(listen_handle, 60 * 60);
#endif
	get_mapserverconfig();
	for (i = 0; i < mapserver_count; ++i) {
		init_map_server(&mapserver[i]);
		ne_createthread(mapserver_thread, &mapserver[i], NULL, 0);
	}

#ifdef SINGLE_THREAD_MOD	
	ne_listensrv_set_single_thread(listen_handle) ;
#endif
	ret = ne_listensrv_session_info(listen_handle, get_maxclient(), 0);

#ifdef SINGLE_THREAD_MOD
	NE_SET_ONCONNECT_ENTRY(listen_handle, test_accept, NULL, test_disconnect) ;
//	lc = (struct listen_contex *)listen_handle ;
//	lc->tcp.pre_out_callback = test_deaccept ;
#else
	ne_listensrv_set_entry(listen_handle, test_accept, test_deaccept, NULL);
#endif

	ne_srv_msgtable_create(listen_handle, MAXID_SYS + 1, 0);

	install_start_session(listen_handle);
	NE_INSTALL_HANDLER(listen_handle, srv_broadcast_handler,
		MAXID_SYS, SYM_BROADCAST, EPL_CONNECT) ;
	ne_net_set_crypt((ne_netcrypt)ne_TEAencrypt, (ne_netcrypt)ne_TEAdecrypt, sizeof(tea_v)) ;
	
	NE_INSTALL_HANDLER(listen_handle, test_usermsg_func, MAXID_SYS, SYM_ECHO, 0);
	NE_INSTALL_HANDLER(listen_handle, usermsg_login_func, MAXID_USER_SERVER, MSG_USER_LOGIN_REQ, 0);

//	NE_INSTALL_HANDLER(listen_handle, usermsg_usermove_func, MAXID_USER_SERVER, MSG_USER_MOVE_REQ, 0);
	NE_INSTALL_HANDLER(listen_handle, usermsg_useraction_func, MAXID_USER_SERVER, MSG_USER_ACTION_REQ, 0);
	NE_INSTALL_HANDLER(listen_handle, usermsg_userscene_func, MAXID_USER_SERVER, MSG_USER_SCENE_REQ, 0);

		//ne_singleth_msghandler
		//ne_do_netmsg
	
	ret = ne_listensrv_open(get_listenport(), listen_handle);

	while (1) {
		if(kbhit()) {
			ch = getch() ;
			if(NE_ESC==ch){
				printf_dbg("you are hit ESC, program eixt\n") ;
				break ;
			}
			switch (ch)
			{
				case 'a':
					ne_rdlock(&id_map_lock);
					rb_travel(userid_map, show_ls_userinfo, NULL);
					ne_rwunlock(&id_map_lock);

					neprintf(_NET("==================\n"));

					ne_rdlock(&sessionid_map_lock);
					rb_travel(sessionid_map, show_ls_userinfo, NULL);
					ne_rwunlock(&sessionid_map_lock);
					break;
				default:
					break;
			}
		}
		else {
			ne_sleep(1000) ;
		}
	}

	ne_srv_msgtable_destroy(listen_handle);

	ne_object_destroy(listen_handle, 0);
	destroy_rsa_key();
	destroy_module();
#endif
	if (log4c_fini()){
		printf("log4c_fini() failed");
	}
    return 0;
}


