#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "ne_log4c.h"
#include "ne_cliapp/ne_cliapp.h"
#include "ne_quadtree/quadtree.h"
#include "ne_common/ne_rbtree.h"
#include "demomsg.h"

typedef struct
{
	NEFLOAT x;
	NEFLOAT y;
}  POINT;

//////////////////////////////////////////////////////////////////////////
//定义英雄结构 存档及战斗用信息
typedef struct HERO 
{
	int Life; //生命
	int attack; //攻击力
	int recover; //防御力
	POINT position; //英雄坐标
	int Money; //金钱
	int Experience; //经验
	//////////////////////////////////////////////////////////////////////////
	//此处待更改，编号
	int addedattack; // 附加战斗力--武器   
	int addrecover; // 附加防御力
	char rolename[NORMAL_SHORT_STR_LEN];
	act_t act;
}*LPHERO;

#define MAX_OTHER_USER (100)
typedef struct Player
{
	POINT position;
	act_t act;
	char rolename[NORMAL_SHORT_STR_LEN];	
} *LPlayer;

//////////////////////////////////////////////////////////////////////////
//定义敌人结构体
typedef struct ENEMY
{
	int Life; //生命
	int attack; //攻击
	int recover; //防御力
	int Money; //金钱
	int Experience; //经验
}*LPENEMY;

//struct HERO hr;
struct Player players[MAX_OTHER_USER];
static log4c_category_t* mycat = NULL;	
ne_handle main_handle = NULL;

static ne_rwlock handle_map_lock;
static rb_node_t *handle_map = NULL;

typedef struct lua_player
{
	struct HERO hr;
	ne_handle handle;
} lua_player;

static void *netThread(void *param)
{
	int ret;
	lua_player *player = (lua_player *)param;
//	ne_usermsgbuf_t sendmsg;
//	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	assert(player);
	for (;;)
	{

		ret = ne_connector_update(player->handle, 10*1000);
		
//		ret = ne_connector_waitmsg(player->handle, (ne_packetbuf_t *)&sendmsg, 10*1000);

//		if(ret > 0) {
//			ne_translate_message(player->handle, (ne_packhdr_t*) &sendmsg) ;
			//run_cliemsg(connect_handle, &msg_buf) ;
			//msg_handle(connect_handle, msg_buf.msgid,msg_buf.param,
			//		msg_buf._data, msg_buf.data_len);
//		}
		if (ret < 0 || (ret == 0 && (struct ne_tcp_node *)player->handle->myerrno != NEERR_SUCCESS)) {
			neprintf(_NET("closed by remote ret = 0\n")) ;
			break ;
		}
		else {
//			neprintf(_NET("wait time out ret = %d\npress any key to continue\n"), ret) ;
//			getch() ;
			
		}

	}
	return (0);
}

int user_scene_req(ne_handle handle);
static void *sceneThread(void *param)
{
	lua_player *player = (lua_player *)param;
	assert(player);
	for (;;)
	{
		if (player->handle)
			user_scene_req(player->handle);
		sleep(5);
	}
}

static int init_player(lua_player *player);

static lua_State *L = NULL;
static int reloadconfig(lua_State *l)
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

static int lua_addnpc(lua_State *l)
{
	int ret = -1;
	const char *name;
	act_t act;
	lua_player *player;	
	LOG_DEBUG(mycat, "%s", __FUNC__);

	if (!lua_isstring(L, 1))
		goto done;

	name = lua_tostring(L, 1);

	LOG_DEBUG(mycat, "%s: add %s", __FUNC__, name);

	player = (lua_player *)malloc(sizeof(lua_player));
	if (!player) {
		LOG_ERROR(mycat, "%s: add %s fail", __FUNC__, name);
		goto done;
	}

	memset(player, 0, sizeof(lua_player));

	if(0 != init_player(player)) {
		LOG_ERROR(mycat, "%s %d: init player fail", __FUNC__, __LINE__);
		goto done;
	}
	
	ne_wrlock(&handle_map_lock);
	handle_map = rb_insert((key_t)player->handle, (data_t)player, handle_map);			
	ne_rwunlock(&handle_map_lock);
	
	login_server(player->handle, name, "nopassword");
	ret = 0;
done:	
	lua_pushnumber(L, ret);
	return (1);
}

void getnpc(data_t t, void *param)
{
	lua_player *player;
	lua_State *l = (lua_State *)param;
	assert(l);
	LOG_DEBUG(mycat, "%s", __FUNC__);
	
	if (!t) {
		LOG_INFO(mycat, "%s %d: t == NULL", __FUNC__, __LINE__);
		return;
	}
	player = (lua_player *)t;

	lua_pushvalue(l, -1);
	
	lua_newtable(l);
	lua_pushstring(l, "name");
	lua_pushstring(l, player->hr.rolename);
	lua_settable(l, -3);

	lua_pushstring(l, "handle");
	lua_pushnumber(l, (unsigned int)player->handle);
	lua_settable(l, -3);	

	lua_pushstring(l, "x");
	lua_pushnumber(l, player->hr.position.x);
	lua_settable(l, -3);

	lua_pushstring(l, "y");
	lua_pushnumber(l, player->hr.position.y);	
	lua_settable(l, -3);

	lua_pcall(l, 1, 0, 0);
}

static int lua_getnpc(lua_State *l)
{
	int ret = -1;
	LOG_DEBUG(mycat, "%s", __FUNC__);

	if (lua_type(l, 1) != LUA_TFUNCTION) {
		LOG_ERROR(mycat, "%s %d: param fail", __FUNC__, __LINE__);
		goto done;
	}

	ne_rdlock(&handle_map_lock);	
	rb_travel(handle_map, getnpc, l);
	ne_rwunlock(&handle_map_lock);
	lua_pop(l, 1);
	ret = 0;
done:
	lua_pushnumber(l, ret);
	return (1);
}

static int lua_npcmove(lua_State *l)
{
	int ret = -1;
	unsigned int handle;
	rb_node_t *rb_data;	
	lua_player *player;
	NEFLOAT x, y;
	act_t act;	

	if (!lua_isnumber(L, 1))
		goto done;	
	if (!lua_isnumber(L, 2))
		goto done;
	if (!lua_isnumber(L, 3))
		goto done;	

	handle = lua_tonumber(L, 1);	
	x = lua_tonumber(L, 2);
	y = lua_tonumber(L, 3);	

	ne_rdlock(&handle_map_lock);
	rb_data = rb_search((key_t)handle, handle_map);
	if (!rb_data || !rb_data->data) {
		ne_rwunlock(&handle_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from handle", __FUNC__, __LINE__);
		goto done;
	}
	player = (lua_player *)rb_data->data;
	ne_rwunlock(&handle_map_lock);
	
	LOG_DEBUG(mycat, "%s: move %s to x[%lf] y[%lf]", __FUNC__, player->hr.rolename, x, y);

	act.act = ACT_MOV_UP;	
	user_action_req(player->handle, act, player->hr.rolename, x, y);
	ret = 0;
done:
	lua_pushnumber(l, ret);
	return (1);
}

static NEBOOL stopmove = NETRUE;
static NEINT32 lua_stopmove(lua_State *l)
{
	if (stopmove)
		lua_pushstring(l, "stop");
	else
		lua_pushstring(l, "run");
	stopmove = NETRUE;
	return (1);
}

static int Psleep(lua_State *L)			/** sleep(seconds) */
{
	unsigned int seconds = luaL_checkint(L, 1);
	lua_pushnumber(L, sleep(seconds));
	return 1;
}

static int lua_init()
{
	L = lua_open();
    luaopen_base(L);
//    luaopen_io(L);
    luaopen_string(L);
    luaopen_math(L);

	lua_pushcfunction(L, reloadconfig);
	lua_setglobal(L, "reload");
	lua_pushcfunction(L, lua_addnpc);
	lua_setglobal(L, "addnpc");
	lua_pushcfunction(L, lua_getnpc);
	lua_setglobal(L, "getnpc");
	lua_pushcfunction(L, lua_npcmove);
	lua_setglobal(L, "movenpc");
	lua_pushcfunction(L, lua_stopmove);
	lua_setglobal(L, "stopmove");
	lua_pushcfunction(L, Psleep);
	lua_setglobal(L, "sleep");			
	return (0);
}

int ClearAllPlayers()
{
	int i;
	for (i = 0; i < MAX_OTHER_USER; ++i) {
		players[i].rolename[0] = '\0';
	}
	return (0);
}

int login_server(ne_handle handle, char *username, char *password)
{
	ne_usermsgbuf_t buf;
	ne_usermsghdr_init(&buf.msg_hdr);	
	NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_LOGIN_REQ ;

	login_req_t *data = (login_req_t *)buf.data;
	strncpy(data->username, username, sizeof(data->username));
	data->username[sizeof(data->username) - 1] = '\0';

	strncpy(data->password, password, sizeof(data->password));
	data->password[sizeof(data->password) - 1] = '\0';

	NE_USERMSG_LEN(&buf) += sizeof(login_req_t);

	NE_USERMSG_PARAM(&buf) = ne_time() ;
	ne_connector_send(handle, (ne_packhdr_t *)&buf, ESF_URGENCY/*ESF_NORMAL*/ | ESF_ENCRYPT);
	return (0);
}
int user_action_req(ne_handle handle, act_t act, char *name, NEFLOAT x, NEFLOAT y)
{
	ne_usermsgbuf_t buf;
//	buf.msg_hdr.packet_hdr = NE_PACKHDR_INIT;						
	ne_usermsghdr_init(&buf.msg_hdr);	
	NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_ACTION_REQ;

    user_action_t *data = (user_action_t *)buf.data;
	memcpy(data->rolename, name, sizeof(data->rolename));
	data->x = x;
	data->y = y;
	memcpy(&data->action, &act, sizeof(act_t));
	NE_USERMSG_LEN(&buf) += sizeof(user_action_t);
	NE_USERMSG_PARAM(&buf) = ne_time() ;
	ne_connector_send(handle, (ne_packhdr_t *)&buf, ESF_URGENCY | ESF_ENCRYPT);
	return (0);
}

int user_scene_req(ne_handle handle)
{
	ne_usermsgbuf_t buf;
//	buf.msg_hdr.packet_hdr = NE_PACKHDR_INIT;
	ne_usermsghdr_init(&buf.msg_hdr);	
	NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_SCENE_REQ;

	NE_USERMSG_PARAM(&buf) = ne_time() ;
	ne_connector_send(handle, (ne_packhdr_t *)&buf, ESF_URGENCY | ESF_ENCRYPT);
	return (0);
}
	
static int msg_entry(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	assert(h_listen == NULL);
	return (0);
}

static int login_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	rb_node_t *rb_data;	
	lua_player *player;
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_USER_LOGIN_ACK)
		return (-1);
	login_ack_t *data = (login_ack_t *)msg->data;

	LOG_DEBUG(mycat, "%s %d: result[%d] name[%s] id[%d]",
		__FUNC__, __LINE__, data->result, data->user_info.base.rolename, data->user_info.id);
	
	if (data->result != 0)
	{
		LOG_INFO(mycat, "%s %d: result is %d", __FUNC__, __LINE__, data->result);		
		return (0);
	}
	if (NE_USERMSG_DATALEN(msg) != sizeof(login_ack_t))
	{
		LOG_INFO(mycat, "%s %d: length is %d", __FUNC__, __LINE__, NE_USERMSG_DATALEN(msg));
		return (0);			
	}

	ne_rdlock(&handle_map_lock);
	rb_data = rb_search((key_t)connect_handle, handle_map);
	if (!rb_data || !rb_data->data) {
		ne_rwunlock(&handle_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from handle", __FUNC__, __LINE__);
		return 0;
	}
	player = (lua_player *)rb_data->data;
	ne_rwunlock(&handle_map_lock);
	player->hr.Life = data->user_info.hp;
	player->hr.attack = data->user_info.attack;
	player->hr.recover = data->user_info.recover;
	player->hr.Money = data->user_info.money;
	player->hr.Experience = data->user_info.experience;
	player->hr.position.x = data->user_info.base.pos_x;
	player->hr.position.y = data->user_info.base.pos_y;	
	strncpy(player->hr.rolename, (char *)data->user_info.base.rolename, sizeof(player->hr.rolename));
	player->hr.rolename[sizeof(player->hr.rolename) - 1] = '\0';

	return (0);
}

static int monster_action_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_MONSTER_ACTION_ACK)
		return (-1);
	monster_action_t *ack = (monster_action_t *)msg->data;

	LOG_DEBUG(mycat, "%s %d: id[%lu] action[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, ack->id, ack->action.act, ack->x, ack->y);
	
	return (0);
}

static int action_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	rb_node_t *rb_data;	
	lua_player *player;
	int i;
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_USER_ACTION_ACK)
		return (-1);
	user_action_t *ack = (user_action_t *)msg->data;

	LOG_DEBUG(mycat, "%s %d: name[%s] action[%d] x[%lf] y[%lf]",
		__FUNC__, __LINE__, ack->rolename, ack->action.act, ack->x, ack->y);

	ne_rdlock(&handle_map_lock);
	rb_data = rb_search((key_t)connect_handle, handle_map);
	if (!rb_data || !rb_data->data) {
		ne_rwunlock(&handle_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from handle", __FUNC__, __LINE__);
		return 0;
	}
	player = (lua_player *)rb_data->data;
	ne_rwunlock(&handle_map_lock);
	memcpy(&player->hr.act, &ack->action, sizeof(player->hr.act));
	player->hr.position.x = ack->x;
	player->hr.position.y = ack->y;
	return (0);
	
#if 0
	if (strcmp((char *)ack->rolename, (char *)hr.rolename) == 0) {
		memcpy(&hr.act, &ack->action, sizeof(hr.act));
		hr.position.x = ack->x;
		hr.position.y = ack->y;
		return (0);
	} else {
		for (i = 0; i < MAX_OTHER_USER; ++i) {
			if (players[i].rolename[0] == '\0') {
					//todo insert a new player
				strncpy(players[i].rolename, (const char *)ack->rolename, sizeof(players[i].rolename));
				players[i].rolename[sizeof(players[i].rolename) - 1] = '\0';
				memcpy(&players[i].act, &ack->action, sizeof(players[i].act));
				players[i].position.x = ack->x;
				players[i].position.y = ack->y;
				break;
			}
			if (strcmp(players[i].rolename, (const char *)ack->rolename) == 0) {
					//todo update the player's info
				players[i].position.x = ack->x;
				players[i].position.y = ack->y;
				memcpy(&players[i].act, &ack->action, sizeof(players[i].act));
				break;
			}
		}
	}
#endif
//todo
//	if (g_wnd)
//		InvalidateRect(g_wnd, NULL, TRUE);
	return (0);
}

static int changemap_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	int id;
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_USER_CHG_MAP_RET)
		return (-1);
	id = NE_USERMSG_PARAM(msg);
	return (0);
}

static int monster_scene_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	int i;
	int index;
	monster_scene_ack_t *t;
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_MONSTER_SCENE_ACK)
		return (-1);
	t = (monster_scene_ack_t *)msg->data;
	for (i = 0, index = 0; i < t->num; ++i)
	{
		LOG_DEBUG(mycat, "%s %d: HP[%d] x[%lf] y[%lf]", 
			__FUNC__, __LINE__, t->monster[i].curHP, t->monster[i].pos_x, t->monster[i].pos_y);
		++index;
	}
	return (0);
}
static int scene_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	int i;
	int index;
	user_scene_ack_t *t;
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_USER_SCENE_ACK)
		return (-1);
	t = (user_scene_ack_t *)msg->data;
//	ClearAllPlayers();
		//todo
	for (i = 0, index = 0; i < t->num; ++i)
	{
//		if (strcmp((const char *)t->player[i].rolename, hr.rolename) == 0)
//			continue;

		LOG_DEBUG(mycat, "%s %d: name[%s] x[%lf] y[%lf]", 
			__FUNC__, __LINE__, t->player[i].rolename, t->player[i].pos_x, t->player[i].pos_y);

//		players[index].position.x = t->player[i].x;
//		players[index].position.y = t->player[i].y;
//		memcpy(players[index].rolename, t->player[i].rolename, NORMAL_SHORT_STR_LEN);
//		memcpy(&players[index].act, &t->player[i].action, sizeof(act_t));
		++index;
	}
	return (0);
	
}


void cmd_login()
{
	char username[256];
	char password[256];
	printf("username = ? password = ?   ");
	scanf("%s %s", username, password);
	login_server(main_handle, username, password);				
}

void cmd_move()
{
	rb_node_t *rb_data;	
	lua_player *player;
	act_t act;
	NEFLOAT x, y;
	printf("direct[up:1 left:2 down:3 right:4] = ? x = ? y = ?   \n");
	scanf("%d %lf %lf", (int *)&act.act, &x, &y);

	ne_rdlock(&handle_map_lock);
	rb_data = rb_search((key_t)main_handle, handle_map);
	if (!rb_data || !rb_data->data) {
		ne_rwunlock(&handle_map_lock);		
		LOG_ERROR(mycat, "%s %d: can not find rb_data from handle", __FUNC__, __LINE__);
		return;
	}
	player = (lua_player *)rb_data->data;
	ne_rwunlock(&handle_map_lock);

	switch (act.act)
	{
		case ACT_MOV_UP:
		case ACT_MOV_LEFT:
		case ACT_MOV_DOWN:
		case ACT_MOV_RIGHT:
			break;
		default:
			act.act = ACT_MOV_UP;
	}
	user_action_req(player->handle, act, player->hr.rolename, x, y);
}

void cmd_getscene()
{
	user_scene_req(main_handle);
}

void* runlua(void* param)
{
	char *filename = (char *)param;
	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) {
		LOG_ERROR(mycat, "cannot run configuration file: %s", lua_tostring(L, -1));
	}
	return NULL;
}

void cmd_runlua()
{
	static char filename[1024];
	printf("lua file name = ?");
	scanf("%s", filename);
	ne_createthread(runlua, filename, NULL, NET_PRIORITY_LOW);
	return;
}

void usage()
{
	printf("q: exit\n");
	printf("l: login to server\n");
	printf("m: move\n");
	printf("s: get scene\n");
	printf("r: run lua script\n");
	printf("t: stop lua move\n");	
}

static int init_player(lua_player *player)
{
	assert(player);
	player->handle = create_connector() ;
	if (!player->handle)
	{
		LOG_ERROR(mycat, "create connector fail");
		return (-1);
	}
	if(0 != start_encrypt(player->handle) ) {
		LOG_ERROR(mycat, "start_encrypt fail");
		return (-1);
	}
	ne_msgentry_install(player->handle, msg_entry, MAXID_SYS,SYM_BROADCAST,0);
	ne_msgentry_install(player->handle, login_ack, MAXID_USER_SERVER, MSG_USER_LOGIN_ACK, 0);
	ne_msgentry_install(player->handle, action_ack, MAXID_USER_SERVER, MSG_USER_ACTION_ACK, 0);
	ne_msgentry_install(player->handle, monster_action_ack, MAXID_USER_SERVER, MSG_MONSTER_ACTION_ACK, 0);
	ne_msgentry_install(player->handle, scene_ack, MAXID_USER_SERVER, MSG_USER_SCENE_ACK, 0);
	ne_msgentry_install(player->handle, monster_scene_ack, MAXID_USER_SERVER, MSG_MONSTER_SCENE_ACK, 0);
	
//	ne_msgentry_install(player->handle, changemap_ack, MAXID_USER_SERVER, MSG_USER_CHG_MAP_RET, 0);		

	ne_createthread(netThread, player, NULL, 0);
//	ne_createthread(sceneThread, NULL, NULL, 0);
	return (0);
}

int main(int argc, char *argv[])
{
	lua_player *player;
	char c;
	log4c_init();
	mycat = log4c_category_get("cmdclient");	
	LOG_DEBUG(mycat, "Debugging app 1 - loop %d", 10);

	if (ne_cliapp_init(argc, (NEINT8 **)argv) != 0) {
		LOG_ERROR(mycat, "ne_cliapp_init fail");
		return (-1);
	}
	ne_rwlock_init(&handle_map_lock);

	player = (lua_player *)malloc(sizeof(lua_player));
	if (!player) {
		LOG_ERROR(mycat, "%s: add main player fail", __FUNC__);
		return (-1);
	}
	memset(player, 0, sizeof(lua_player));
	if(0 != init_player(player)) {
		LOG_ERROR(mycat, "%s %d: init player fail", __FUNC__, __LINE__);
		return (-1);
	}
	
	ne_wrlock(&handle_map_lock);
	handle_map = rb_insert((key_t)player->handle, (data_t)player, handle_map);			
	ne_rwunlock(&handle_map_lock);
	
	main_handle = player->handle;

	lua_init();
	
	while (c = getchar()) {
		switch (c)
		{
			case 'q':
				exit(0);
				break;
			case '\n':
				break;
			case 'l':
				cmd_login();
				break;
			case 'm':
				cmd_move();
				break;
			case 's':
				cmd_getscene();
				break;
			case 'r':
				cmd_runlua();
				break;
			case 't':
				stopmove = NEFALSE;
				break;
			default:
				usage();
				break;
		}
	}
    return 0;
}


