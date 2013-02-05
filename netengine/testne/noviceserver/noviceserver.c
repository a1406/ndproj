#include "ne_app/ne_app.h"
#include "ne_common/ne_rbtree.h"
#include "ne_quadtree/quadtree.h"
//#include "ne_listensrv.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
//#include "OEM/map.h"
#include "demomsg.h"
#include "ne_log4c.h"
#include "noviceserver.h"

//#define LISTEN_PORT  (12346)
//#define MAX_CLIENT   (32)
/*
 * manage many types of gate server,
 */ 
#define MAX_CLEAN_USER  (100)
#define MAX_MONSTERS    (10000)
static log4c_category_t* mycat = NULL;	
static ne_handle g_listen_handle;
static NEUINT32 monster_index;
//todo need lock
static ne_mutex map_lock;
static quadtree_t *mapobj_tree = NULL;   //find user by position

static ne_rwlock name_map_lock;
static rb_node_t *username_map = NULL;   //find user by rolename
static ne_rwlock id_map_lock;
static rb_node_t *userid_map = NULL;     //find user by id

static ne_rwlock monster_lock;
static NEUINT16 monster_count;
static monster monsters[MAX_MONSTERS];
/*
static MAP maps[20][16];
static NEINT32 init_map(char *filename)
{
	FILE *fp=fopen(filename,"r+");
	fread(maps,sizeof(MAP),320,fp);
	fclose(fp);
	return (0);
}
*/
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
static int lua_addmonster(lua_State *l)
{
	int ret = -1;
	monster mon;
	LOG_DEBUG(mycat, "%s", __FUNC__);					

	if (!lua_isnumber(l, -1))
		goto done;
	mon.base.type = lua_tonumber(l, -1);

	if (!lua_isnumber(l, -2))
		goto done;
	mon.born_x = lua_tonumber(l, -2);

	if (!lua_isnumber(l, -3))
		goto done;
	mon.born_y = lua_tonumber(l, -3);

	mon.observered = NETRUE;
		//todo check valid
	mon.base.pos_x = mon.born_x;
	mon.base.pos_y = mon.born_y;
	quadbox_t box = {mon.base.pos_x, mon.base.pos_y, mon.base.pos_x, mon.base.pos_y};
		//todo HP should load from db
	mon.base.maxHP = mon.base.curHP = 100;
	if (mon.base.type & 0xf)
		mon.base.type = MONSTER_1;

	mon.base.id = monster_index++;

	ne_mutex_lock(&map_lock);
	ne_wrlock(&monster_lock);
	memcpy(&monsters[monster_count], &mon, sizeof(mon));
	if (quadtree_insert(mapobj_tree, &monsters[monster_count].quad_lst, &box) == NULL) {
		LOG_ERROR(mycat, "%s %d: quadtree_insert fail, err[%s]", __FUNC__, __LINE__, ne_last_error());
		ne_rwunlock(&monster_lock);
		ne_mutex_unlock(&map_lock);				
		return (-1);				
	}
	++monster_count;
	ne_rwunlock(&monster_lock);	
	ne_mutex_unlock(&map_lock);
	ret = 0;
done:	
	lua_pushnumber(l, ret);
	return (1);	
}

static int lua_init(char *filename)
{
	L = lua_open();
    luaopen_base(L);
//    luaopen_io(L);
    luaopen_string(L);
    luaopen_math(L);

	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) {
		error(L, "cannot run configuration file: %s", lua_tostring(L, -1));
		return (-1);
	}
	lua_pushcfunction(L, reloadconfig);
	lua_setglobal(L, "reload");
	lua_pushcfunction(L, lua_addnpc);
	lua_setglobal(L, "addnpc");
	lua_pushcfunction(L, lua_addmonster);
	lua_setglobal(L, "addmonster");	
	
	return (0);
}

static unsigned short get_listenport()
{
	unsigned short listen_port = 0;
	lua_getglobal(L, "listen_port");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "listen_port should be a number");				
		return (0);
	}
	listen_port = lua_tonumber(L, -1);
	return listen_port;
}
static unsigned short get_maxclient()
{
	int max_client = 0;
	lua_getglobal(L, "max_client");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "max_client should be a number");		
		return (0);
	}
	max_client = lua_tonumber(L, -1);
	return max_client;
}
static NEUINT32 get_mapid()
{
	NEUINT32 mapid;
	lua_getglobal(L, "map_id");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "map_id should be a number");
		return (0);
	}
	mapid = lua_tonumber(L, -1);
	return mapid;
}
static quadbox_t get_mapbox()
{
	quadbox_t ret = {0,0,0,0};
	lua_getglobal(L, "map_xmin");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "map_xmin should be a number");
	} else {
		ret._xmin = lua_tonumber(L, -1);
	}
	lua_getglobal(L, "map_xmax");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "map_xmax should be a number");
	} else {
		ret._xmax = lua_tonumber(L, -1);
	}
	lua_getglobal(L, "map_ymin");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "map_ymin should be a number");
	} else {
		ret._ymin = lua_tonumber(L, -1);
	}
	lua_getglobal(L, "map_ymax");
    if (!lua_isnumber(L, -1)) {
		LOG_ERROR(mycat, "map_ymax should be a number");
	} else {
		ret._ymax = lua_tonumber(L, -1);
	}
	return (ret);
}

static NEINT32 init_mapobj_tree()
{
	quadbox_t box = get_mapbox();
	mapobj_tree = quadtree_create(box, QTREE_DEPTH_MIN, QBOX_OVERLAP_MIN);
	ne_mutex_init(&map_lock);
	return (0);
}

static NEINT32 reg_map_server(NEUINT16 sessionid, ne_handle listener)
{
	ne_usermsgbuf_t buf;
	NEINT32 ret = -1;
	reg_mapserver_t *data = (reg_mapserver_t *)buf.data;
	
	data->id = get_mapid();
	data->box = get_mapbox();

	NE_USERMSG_MAXID(&buf) = MAXID_SERVER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_SERVER_REG;
	NE_USERMSG_LEN(&buf) = NE_USERMSG_HDRLEN + sizeof(reg_mapserver_t);
	NE_USERMSG_PARAM(&buf) = 0;
	NE_USERMSG_ENCRYPT(&buf) = 0;
	ret = NE_SENDEX(sessionid, (ne_usermsghdr_t *)&buf, ESF_URGENCY, listener);
	if (ret == NE_USERMSG_LEN(&buf)) {
		ret = 0;
	}
	return (ret);
}

#define MAX_GATE_SERVER (10)
static NEUINT16 gate_server_id[MAX_GATE_SERVER];

static NEINT32 test_accept(NEUINT16 sessionid, SOCKADDR_IN *addr, ne_handle listener)
{
	NEINT32 i;
    printf("%s %d: sessonid[%hx] addr[%s] listener[]\n", __FUNCTION__, __LINE__, sessionid, (char *)(inet_ntoa(addr->sin_addr)));
	for (i = 0; i < MAX_GATE_SERVER; ++i) {
		if (gate_server_id[i] == (NEUINT16)-1) {
			gate_server_id[i] = sessionid;
			break;
		}
	}

	if (i >= MAX_GATE_SERVER) {
		LOG_ERROR(mycat, "gate server too much");
		return (-1);
	}

	reg_map_server(sessionid, listener);
    return (0);
}

static void test_deaccept(NEUINT16 sessionid, ne_handle h_listen)
{
	struct listen_contex *lc;
	struct cm_manager *mamager;
	struct ne_client_map *session;

	NEINT32 i;
	printf("%s %d: sessonid[%hx] listener[]\n", __FUNCTION__, __LINE__, sessionid);
	for (i = 0; i < MAX_GATE_SERVER; ++i) {
		if (gate_server_id[i] == sessionid) {
			gate_server_id[i] = -1;
			break;
		}
	}
	if (i >= MAX_GATE_SERVER) {
		LOG_ERROR(mycat, "can not find sessionid when deacdept");
	}

	lc = (struct listen_contex *) h_listen ;
	mamager = ne_listensrv_get_cmmamager(h_listen);
	ne_assert(mamager) ;
	session = (struct ne_client_map *)mamager->lock(mamager, sessionid) ;
	if(session) {
		tcp_client_close_fin(session, 1);
		mamager->unlock(mamager, sessionid) ;
	} else {
		LOG_ERROR(mycat, "%s %d: can not find session by sessionid[%d]", __FUNC__, __LINE__, sessionid);			
	}
}

static NEINT32 clean_user(unsigned long id)
{
	user_info_t *user_info;
	rb_node_t *usernode;

	ne_rdlock(&id_map_lock);
	usernode = rb_search(id, userid_map);
	if (!usernode || !usernode->data) {
		ne_rwunlock(&id_map_lock);
		LOG_ERROR(mycat, "%s %d: can not find usernode[%d]", __FUNC__, __LINE__, id);					
		return (-1);
	}
	user_info = usernode->data;
	ne_rwunlock(&id_map_lock);

	ne_wrlock(&id_map_lock);
	userid_map = rb_erase(id, userid_map);
	ne_rwunlock(&id_map_lock);

	ne_wrlock(&name_map_lock);	
	username_map = rb_erase_v2(user_info->base.rolename, username_map);
	ne_rwunlock(&name_map_lock);
	
	ne_mutex_lock(&map_lock);
	list_del(&user_info->quad_lst);
	ne_mutex_unlock(&map_lock);

	LOG_DEBUG(mycat, "%s %d: user[%s] id[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, user_info->base.rolename, user_info->id, user_info->base.pos_x, user_info->base.pos_y);
	
	free(user_info);
	return (0);
}

//todo clean not active user
static NEINT32 tryto_clean_user()
{
	NEUINT32 clean_id[MAX_CLEAN_USER];
	return (0);
}

static NEINT32 test_usermsg_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle listener)
{
	printf("%s %d: sessonid[%hx] listener[]\n", __FUNCTION__, __LINE__, sessionid);
}

static NEINT32 server_reg_func(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle h_listen)
{
	LOG_DEBUG(mycat, "%s %d: ",	__FUNC__, __LINE__);
	return (0);
}

static NEINT32 user_leave_map(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle listener)
{
	NEUINT32 id;
	ne_assert(NE_USERMSG_MAXID(msg) == MAXID_SERVER_SERVER);
	ne_assert(NE_USERMSG_MINID(msg) == MSG_USER_LEAVE_MAP);
	id = NE_USERMSG_PARAM(msg);
	clean_user(id);
	return (0);
}

//todo 
static NEINT32 moveto_newmap(NEINT32 id_newmap, user_info_t *user_info, NEUINT16 sessionid, ne_handle listener)
{
	ne_usermsgbuf_t buf;
	NEINT32 ret = -1;
	moveto_newmap_t *data = (moveto_newmap_t *)buf.data;

	data->id = id_newmap;
	memcpy(&data->user_info, user_info, sizeof(user_info_t));
	NE_USERMSG_MAXID(&buf) = MAXID_SERVER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_LEAVE_MAP;
	NE_USERMSG_LEN(&buf) = NE_USERMSG_HDRLEN + sizeof(moveto_newmap_t);
	NE_USERMSG_PARAM(&buf) = user_info->id;	
	NE_USERMSG_ENCRYPT(&buf) = 0;
	ret = NE_SENDEX(sessionid, (ne_usermsghdr_t *)&buf, ESF_URGENCY, listener);
	if (ret == NE_USERMSG_LEN(&buf)) {
			//maybe we should wait gateserver return and then clean_user
			//clean_user(id);
		return (0);
	}
	LOG_ERROR(mycat, "%s %d: sendex ret %d", __FUNC__, __LINE__, ret);
	return (-1);
}

static NEINT32 send_to_all_gateserver(ne_usermsghdr_t * buf, ne_handle listener)
{
	NEINT32 ret;
	NEINT32 i;

	for (i = 0; i < MAX_GATE_SERVER; ++i) {
		if (gate_server_id[i] == (NEUINT16)-1)
			continue;
		ret = NE_SENDEX(gate_server_id[i], buf, ESF_URGENCY, listener);
	}
	return (0);
}

static NEINT32 user_change_map(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle listener)
{
	ne_usermsgbuf_t buf;
	user_action_t *ack = (user_action_t *)buf.data;
	brd_torange_info_t *broadcast_data = (brd_torange_info_t *)(buf.data + sizeof(user_action_t));
	login_ack_t *data = (login_ack_t *)msg->data;					
	user_info_t *user = NULL;
//	ne_usermsgbuf_t buf = NE_USERMSG_INITILIZER;
	NEINT32 ret = -1;
		//long key;

	assert(NE_USERMSG_MAXID(msg) == MAXID_SERVER_SERVER);
	assert(NE_USERMSG_MINID(msg) == MSG_USER_CHG_MAP);
	assert(NE_USERMSG_DATALEN(msg) == sizeof(login_ack_t));

	user = malloc(sizeof(user_info_t));
	if (!user) {
		LOG_ERROR(mycat, "%s %d: malloc fail, err[%s]", __FUNC__, __LINE__, ne_last_error());
		return (-1);
	}
	memcpy(user, &data->user_info, sizeof(user_info_t));
	quadbox_t box = {user->base.pos_x, user->base.pos_y, user->base.pos_x, user->base.pos_y};

	LOG_DEBUG(mycat, "%s %d: user[%s] changemap id[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, user->base.rolename, user->id, user->base.pos_x, user->base.pos_y);
	
	ne_mutex_lock(&map_lock);
	if (quadtree_insert(mapobj_tree, &user->quad_lst, &box) == NULL) {
		LOG_ERROR(mycat, "%s %d: quadtree_insert fail, err[%s]", __FUNC__, __LINE__, ne_last_error());
		free(user);
		ne_mutex_unlock(&map_lock);				
		return (-1);				
	}
	user->quad_head = user->quad_lst.prev;
	ne_mutex_unlock(&map_lock);

		//key = calc_hashnr(user->rolename, strlen(user->rolename));

	ne_wrlock(&name_map_lock);	
	username_map = rb_insert_v2(sizeof(user->base.rolename), user->base.rolename, username_map);
	ne_rwunlock(&name_map_lock);
	
	ne_wrlock(&id_map_lock);
	userid_map = rb_insert(user->id, user, userid_map);			
	ne_rwunlock(&id_map_lock);
	
	NE_USERMSG_MAXID(msg) = MAXID_SERVER_SERVER ;
	NE_USERMSG_MINID(msg) = MSG_USER_CHG_MAP_RET ;
	NE_USERMSG_PARAM(msg) = user->id;	
	NE_USERMSG_LEN(msg) = NE_USERMSG_HDRLEN;
	ret = NE_SENDEX(sessionid, (ne_usermsghdr_t *)msg, ESF_URGENCY, listener);

	broadcast_data->box._xmin = user->base.pos_x - SCENE_LENGTH;
	broadcast_data->box._ymin = user->base.pos_y - SCENE_LENGTH;
	broadcast_data->box._xmax = user->base.pos_x + SCENE_LENGTH;
	broadcast_data->box._ymax = user->base.pos_y + SCENE_LENGTH;

	broadcast_data->real_maxid = MAXID_USER_SERVER;
	broadcast_data->real_minid = MSG_USER_ACTION_ACK;
	NE_USERMSG_MAXID(&buf) = MAXID_SERVER_SERVER;
	NE_USERMSG_MINID(&buf) = MSG_BROADCAST_TORANGE;
	NE_USERMSG_PARAM(&buf) = sizeof(user_action_t);
	NE_USERMSG_LEN(&buf) = NE_USERMSG_HDRLEN + sizeof(user_action_t)
		+ sizeof(brd_torange_info_t);
	NE_USERMSG_ENCRYPT(&buf) = 0;

	memcpy(ack->rolename, user->base.rolename, NORMAL_SHORT_STR_LEN);
	ack->action.act = ACT_STANDBY;
	ack->param = 0;
	ack->x = user->base.pos_x;
	ack->y = user->base.pos_y;
	send_to_all_gateserver((ne_usermsghdr_t *)&buf, listener);

	return (0);
}

static NEINT32 user_scene_req(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle listener)
{
	NEUINT32 id;
	user_info_t *user_info = NULL;
	monster *mon = NULL;
	rb_node_t *usernode;
	struct list_head *retlist[MAX_QUAD_SEARCH];
	struct list_head *pos;
	ne_usermsgbuf_t buf, buf_monster;// = NE_USERMSG_INITILIZER;
	user_scene_ack_t *ack = (user_scene_ack_t *)buf.data;
	monster_scene_ack_t *ack_monster = (monster_scene_ack_t *)buf_monster.data;
	user_scene_req_t *req = (user_scene_req_t *)msg->data;
	NEINT32 ret;
	NEINT32 index;
	NEINT32 i;
	
	assert(NE_USERMSG_MAXID(msg) == MAXID_SERVER_SERVER);
	assert(NE_USERMSG_MINID(msg) == MSG_USER_SCENE_REQ);

	id = NE_USERMSG_PARAM(msg);	
	ne_rdlock(&id_map_lock);
	usernode = rb_search(id, userid_map);
	if (!usernode || !usernode->data) {
		ne_rwunlock(&id_map_lock);	
		LOG_ERROR(mycat, "%s %d: can not find usernode[%d]", __FUNC__, __LINE__, id);					
		return (-1);
	}
	user_info = usernode->data;
	ne_rwunlock(&id_map_lock);	

	user_info->rollcall = 0;
	quadbox_t box = {user_info->base.pos_x - 100,
					 user_info->base.pos_y - 100,
					 user_info->base.pos_x + 100,
					 user_info->base.pos_y + 100};			
	index = 0;
	ne_mutex_lock(&map_lock);
	ne_rdlock(&monster_lock);
	quadtree_search(mapobj_tree, &box, retlist, &index, MAX_QUAD_SEARCH);
	ack->num = 0;
	for (i = 0; i < index; ++i) {
		list_for_each(pos, retlist[i]) {
			user_info = list_entry(pos, user_info_t, quad_lst);
			if (user_info->base.type & 0xf) {
				memcpy(&ack->player[ack->num], &user_info->base, sizeof(player_base_t)); 
				++ack->num;
			} else {
				mon = list_entry(pos, monster, quad_lst);
				memcpy(&ack_monster->monster[ack->num], &mon->base, sizeof(monster_base_t));
				mon->observered = NETRUE;
				++ack_monster->num;				
			}
		}
	}
	ne_rwunlock(&monster_lock);
	ne_mutex_unlock(&map_lock);	

	LOG_DEBUG(mycat, "%s %d: user[%s] scene req num[%d]",
		__FUNC__, __LINE__, user_info->base.rolename, ack->num);
	
	NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_SCENE_ACK ;
	NE_USERMSG_LEN(&buf) = NE_USERMSG_HDRLEN + (sizeof(user_scene_ack_t) + ack->num * sizeof(player_base_t));
	NE_USERMSG_PARAM(&buf) = id;	
	NE_USERMSG_ENCRYPT(&buf) = 0;
	ret = NE_SENDEX(sessionid, (ne_usermsghdr_t *)&buf, ESF_URGENCY, listener);

	NE_USERMSG_MAXID(&buf_monster) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf_monster) = MSG_MONSTER_SCENE_ACK ;
	NE_USERMSG_LEN(&buf_monster) = NE_USERMSG_HDRLEN + (sizeof(monster_scene_ack_t) + ack->num * sizeof(monster_base_t));
	NE_USERMSG_PARAM(&buf) = id;	
	NE_USERMSG_ENCRYPT(&buf) = 0;
	ret = NE_SENDEX(sessionid, (ne_usermsghdr_t *)&buf, ESF_URGENCY, listener);
	return (0);
}
static NEINT32 user_action_req(NEUINT16 sessionid, ne_usermsgbuf_t *msg , ne_handle listener)
{
	user_action_t *data = (user_action_t *)msg->data;
	ne_usermsgbuf_t buf;
	user_action_t *ack = (user_action_t *)buf.data;
	brd_torange_info_t *broadcast_data = (brd_torange_info_t *)(buf.data + sizeof(user_action_t));
	NEINT32 ret = -1;
	NEUINT32 id;
	user_info_t *user_info = NULL;
	rb_node_t *usernode;
//	NEINT32 usernum;
//	struct list_head *retlist[MAX_QUAD_SEARCH];
//	struct list_head *pos;
//	NEINT32 index;
//	NEINT32 i;
	
	assert(NE_USERMSG_MAXID(msg) == MAXID_SERVER_SERVER);
	assert(NE_USERMSG_MINID(msg) == MSG_USER_ACTION_REQ);
	assert(NE_USERMSG_DATALEN(msg) == sizeof(user_action_t));
	;//todo check data valid

		//get id from param
	id = NE_USERMSG_PARAM(msg);
		//find user_info by id
	ne_rdlock(&id_map_lock);
	usernode = rb_search(id, userid_map);
	if (!usernode || !usernode->data) {
		ne_rwunlock(&id_map_lock);	
		LOG_ERROR(mycat, "%s %d: can not find usernode[%d]", __FUNC__, __LINE__, id);							
		goto notfound;
	}
	user_info = usernode->data;
	ne_rwunlock(&id_map_lock);	

	LOG_DEBUG(mycat, "%s %d: user[%s] action act[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, user_info->base.rolename, (int)data->action.act, data->x, data->y);
	
	user_info->rollcall = 0;
	if (strncmp(user_info->base.rolename, data->rolename, sizeof(data->rolename)) != 0) {
		LOG_ERROR(mycat, "%s %d: rolename not compare", __FUNC__, __LINE__);							
		goto notfound;
	}

		//list_del from quadtree
	ne_mutex_lock(&map_lock);	
	list_del(&user_info->quad_lst);
	ne_mutex_unlock(&map_lock);	

		//quadinsert into quadtree
	user_info->base.pos_x = data->x;
	user_info->base.pos_y = data->y;
	memcpy(&user_info->act, &data->action, sizeof(user_info->act));
	quadbox_t box = {user_info->base.pos_x,
					 user_info->base.pos_y,
					 user_info->base.pos_x,
					 user_info->base.pos_y};			
	ne_mutex_lock(&map_lock);
	if (quadtree_insert(mapobj_tree, &user_info->quad_lst, &box) == NULL) {
		ne_mutex_unlock(&map_lock);
		LOG_ERROR(mycat, "%s %d: quadtree_insert fail, err[%s]", __FUNC__, __LINE__, ne_last_error());
		goto notfound;
	}
	user_info->quad_head = user_info->quad_lst.prev;			
	ne_mutex_unlock(&map_lock);
	
	broadcast_data->box._xmin = user_info->base.pos_x - SCENE_LENGTH;
	broadcast_data->box._ymin = user_info->base.pos_y - SCENE_LENGTH;
	broadcast_data->box._xmax = user_info->base.pos_x + SCENE_LENGTH;
	broadcast_data->box._ymax = user_info->base.pos_y + SCENE_LENGTH;
	

//	index = 0;
//	ne_mutex_lock(&map_lock);
//	quadtree_search(mapobj_tree, &box, retlist, &index, MAX_QUAD_SEARCH);
//	usernum = 0;

//	for (i = 0; i < index; ++i) {	
//		list_for_each(pos, retlist[i]) {
//			user_info = list_entry(pos, user_info_t, quad_lst);
//			if (user_info->id == id)
//				continue;
//			broadcast_data->userid[usernum] = user_info->id;
//			++usernum;
//		}
//	}
//	ne_mutex_unlock(&map_lock);

//	broadcast_data->usernum = usernum;
	broadcast_data->real_maxid = MAXID_USER_SERVER;
	broadcast_data->real_minid = MSG_USER_ACTION_ACK;
	NE_USERMSG_MAXID(&buf) = MAXID_SERVER_SERVER;
	NE_USERMSG_MINID(&buf) = MSG_BROADCAST_TORANGE;
	NE_USERMSG_PARAM(&buf) = sizeof(user_action_t);
	NE_USERMSG_LEN(&buf) = NE_USERMSG_HDRLEN + sizeof(user_action_t)
		+ sizeof(brd_torange_info_t);
	NE_USERMSG_ENCRYPT(&buf) = 0;
	memcpy(ack, data, sizeof(*ack));

	send_to_all_gateserver((ne_usermsghdr_t *)&buf, listener);
notfound:
	return (0);
}

static void show_userinfo(data_t data, void *param)
{
	user_info_t *user = (user_info_t *)data;
	neprintf(_NET("id[%d] rolename[%s] rollcall[%d] pos[%lf:%lf]\n"),
		user->id, user->base.rolename, user->rollcall, user->base.pos_x, user->base.pos_y);
}
static void show_userinfo_v2(data_t data, void *param)
{
	user_info_t *user;
	user = container_of(data, user_info_t, base.rolename);
	show_userinfo(user, param);
//	neprintf(_NET("id[%d] rolename[%s] rollcall[%d] pos[%lf:%lf]\n"),
//		user->id, user->base.rolename, user->rollcall, user->base.pos_x, user->base.pos_y);
}


static void show_quad_userinfo(struct list_head *head, void *param)
{
	struct list_head *pos;
	user_info_t *user = NULL;	
	list_for_each(pos, head) {
		user = list_entry(pos, user_info_t, quad_lst);
		if (user->base.type & 0xf) {
			neprintf(_NET("id[%d] rolename[%s] rollcall[%d] pos[%lf:%lf]\n"),
				user->id, user->base.rolename, user->rollcall, user->base.pos_x, user->base.pos_y);
		}
	}
}

static NEINT32 count_weight(monster *mon, user_info_t *user)
{
	return (0);
}

static void tryto_attack(monster *mon, user_info_t *user)
{
	ne_usermsgbuf_t buf;
	monster_action_t *ack = (monster_action_t *)buf.data;	
	brd_torange_info_t *broadcast_data = (brd_torange_info_t *)(buf.data + sizeof(user_action_t));	
	quadbox_t box;
	if (mon->base.pos_x != user->base.pos_x) {
		if (user->base.pos_x > mon->base.pos_x)
			++mon->base.pos_x;
		else
			--mon->base.pos_x;
	}
	if (mon->base.pos_y != user->base.pos_y) {
		if (user->base.pos_y > mon->base.pos_y)
			++mon->base.pos_y;
		else
			--mon->base.pos_y;
	}
	if (mon->base.pos_x == user->base.pos_x && mon->base.pos_y == user->base.pos_y) {
		;//todo attack;
	}
	box._xmin = box._xmax = mon->base.pos_x;
	box._ymin = box._ymax = mon->base.pos_y;	
	list_del(&mon->quad_lst);
	if (quadtree_insert(mapobj_tree, &mon->quad_lst, &box) == NULL) {
		LOG_ERROR(mycat, "%s %d: quadtree_insert fail, err[%s]", __FUNC__, __LINE__, ne_last_error());
		return;
	}
	broadcast_data->real_maxid = MAXID_USER_SERVER;
	broadcast_data->real_minid = MSG_USER_ACTION_ACK;
	NE_USERMSG_MAXID(&buf) = MAXID_SERVER_SERVER;
	NE_USERMSG_MINID(&buf) = MSG_BROADCAST_TORANGE;
	NE_USERMSG_PARAM(&buf) = sizeof(user_action_t);
	NE_USERMSG_LEN(&buf) = NE_USERMSG_HDRLEN + sizeof(user_action_t)
		+ sizeof(brd_torange_info_t);
	NE_USERMSG_ENCRYPT(&buf) = 0;
	ack->id = mon->base.id;
	ack->action.act = ACT_STANDBY;
	ack->x = mon->base.pos_x;
	ack->y = mon->base.pos_y;	
	
	send_to_all_gateserver((ne_usermsghdr_t *)&buf, g_listen_handle);
}

/* try to attack on player */
static void monster_ai(monster *mon)
{
	NEINT32 index, i, target_weight, new_weight;
	struct list_head *retlist[MAX_QUAD_SEARCH];
	struct list_head *pos;
	user_info_t *target = NULL;
	user_info_t *user_info = NULL;	
	assert(mon);
	if (!mon->observered)
		return;

	quadbox_t box = {mon->base.pos_x - SCENE_LENGTH,
					 mon->base.pos_y - SCENE_LENGTH,
					 mon->base.pos_x + SCENE_LENGTH,
					 mon->base.pos_y + SCENE_LENGTH};
	
	index = 0;
	ne_mutex_lock(&map_lock);
	quadtree_search(mapobj_tree, &box, retlist, &index, MAX_QUAD_SEARCH);
	for (i = 0; i < index; ++i) {
		list_for_each(pos, retlist[i]) {
			user_info = list_entry(pos, user_info_t, quad_lst);
			if (user_info->base.type & 0xf) {
				new_weight = count_weight(mon, user_info);
				if (!target || new_weight > target_weight) {
					target = user_info;
					target_weight = new_weight;
				}
			}
		}
	}
	if (index == 0)
		mon->observered = NEFALSE;
	if (target)
		tryto_attack(mon, target);
	ne_mutex_unlock(&map_lock);
}

NEINT32 ontimer(netimer_t timer_id, void *param)
{
	int i;
	LOG_DEBUG(mycat, "%s", __FUNC__) ;
	ne_rdlock(&monster_lock);
	for (i = 0; i < monster_count; ++i) {
		monster_ai(&monsters[i]);
	}
	ne_rwunlock(&monster_lock);		
	return (0);
}

int main(int argc, char *argv[])
{
    char ch;
    int ret;
	int i;

	monster_count = 0;
	memset(&monsters, 0, sizeof(monsters));
	
	for (i = 0; i < MAX_GATE_SERVER; ++i)
		gate_server_id[i] = -1;
	
	log4c_init();
	mycat = log4c_category_get("noviceserver");	

//    const log4c_location_info_t locinfo = LOG4C_LOCATION_INFO_INITIALIZER(NULL);	
//	log4c_category_log(mycat, LOG4C_PRIORITY_DEBUG, "Debugging app 1 - loop %d", i);
//	log4c_category_log_locinfo(mycat, &locinfo, LOG4C_PRIORITY_DEBUG, "Debugging app 1 - loop %d", i);
	LOG_DEBUG(mycat, "Debugging app 1 - loop %d", 10);
	
    ret = init_module();

	lua_init("config.lua");
//	init_map("1.map");
	init_mapobj_tree();
	ne_rwlock_init(&name_map_lock);
	ne_rwlock_init(&id_map_lock);
	ne_rwlock_init(&monster_lock);

	ret = init_module();
//    ret = create_rsa_key();

	g_listen_handle = ne_object_create("listen-tcp") ;

	ne_set_connection_timeout(g_listen_handle, 60 * 60);
	
	ne_listensrv_set_single_thread(g_listen_handle) ;
	ne_thsrv_timer(((struct listen_contex *)g_listen_handle)->mqc.thid, ontimer, NULL, 1 * 1000, ETT_LOOP);
    ret = ne_listensrv_session_info(g_listen_handle, get_maxclient(), 0);

	NE_SET_ONCONNECT_ENTRY(g_listen_handle, test_accept, NULL, test_deaccept) ;
	ne_srv_msgtable_create(g_listen_handle, MAXID_SYS + 1, 0);
	NE_INSTALL_HANDLER(g_listen_handle, test_usermsg_func, MAXID_SYS, SYM_ECHO, 0);

	NE_INSTALL_HANDLER(g_listen_handle, user_change_map, MAXID_SERVER_SERVER, MSG_USER_CHG_MAP, 0);
	NE_INSTALL_HANDLER(g_listen_handle, user_leave_map, MAXID_SERVER_SERVER, MSG_USER_LEAVE_MAP, 0);		
//	NE_INSTALL_HANDLER(listen_handle, user_move_req, MAXID_USER_SERVER, MSG_USER_MOVE_REQ, 0);
	NE_INSTALL_HANDLER(g_listen_handle, user_action_req, MAXID_SERVER_SERVER, MSG_USER_ACTION_REQ, 0);
	NE_INSTALL_HANDLER(g_listen_handle, user_scene_req, MAXID_SERVER_SERVER, MSG_USER_SCENE_REQ, 0);

	NE_INSTALL_HANDLER(g_listen_handle, server_reg_func, MAXID_SERVER_SERVER, MSG_SERVER_REG, 0);
	
	ret = ne_listensrv_open(get_listenport(), g_listen_handle);

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
					rb_travel(userid_map, show_userinfo, NULL);
					ne_rwunlock(&id_map_lock);					
					neprintf(_NET("==================\n"));
					
					ne_rdlock(&name_map_lock);
					rb_travel(username_map, show_userinfo_v2, NULL);
					ne_rwunlock(&name_map_lock);
					neprintf(_NET("==================\n"));
					if (mapobj_tree) {
						ne_mutex_lock(&map_lock);
						quad_travel(mapobj_tree->_root, show_quad_userinfo, NULL);
						ne_mutex_unlock(&map_lock);
					}
					break;
				default:
					break;
			}
			
		}
		else {
			ne_sleep(1000) ;
		}
	}

//	ret = ne_listensrv_close(listen_handle, 0);	
	ne_srv_msgtable_destroy(g_listen_handle);

	ne_object_destroy(g_listen_handle, 0);

//	destroy_rsa_key();
	destroy_module();

	if (log4c_fini()){
		printf("log4c_fini() failed");
	}
	
    return 0;
}


