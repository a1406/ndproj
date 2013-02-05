#ifndef _DEMOMSG_H_
#define _DEMOMSG_H_

#pragma pack(4) 

#define NORMAL_SHORT_STR_LEN 32
#define MAX_QUAD_SEARCH (100)

const NEFLOAT SCENE_LENGTH = 10;
const NEFLOAT PLAYER_LENGTH = 2;

//MAXID_USER_SERVER
enum
{
	MSG_USER_LOGIN_REQ = 0x1,
	MSG_USER_LOGIN_ACK,
	MSG_USER_MOVE_REQ,
	MSG_USER_MOVE_ACK,
	MSG_USER_ACTION_REQ,
	MSG_USER_ACTION_ACK,
	MSG_MONSTER_ACTION_ACK,	
	MSG_OTHERUSER_MOVE,
	MSG_OTHERUSER_ACTION,
	MSG_USER_SCENE_REQ,
	MSG_USER_SCENE_ACK,
	MSG_MONSTER_SCENE_ACK,	
};

//MAXID_SERVER_SERVER
enum
{
	MSG_USER_CHG_MAP = 0x1,
	MSG_USER_CHG_MAP_RET,
	MSG_USER_LEAVE_MAP,
	MSG_USER_LEAVE_MAP_RET,
	MSG_BROADCAST_TOUSER,
	MSG_BROADCAST_TORANGE,	
	MSG_SERVER_REG,
};

typedef struct login_req_t
{
	NEUINT8 username[NORMAL_SHORT_STR_LEN];
	NEUINT8 password[NORMAL_SHORT_STR_LEN];
} login_req_t;

typedef struct reg_mapserver_t
{
	NEUINT32 id;
	quadbox_t box;
} reg_mapserver_t;

typedef enum USER_ACTION
{
	ACT_STANDBY = 1,
	ACT_MOV_UP,
	ACT_MOV_LEFT,
	ACT_MOV_DOWN,
	ACT_MOV_RIGHT,
	ACT_ATTACK_UP,
	ACT_ATTACK_LEFT,
	ACT_ATTACK_DOWN,
	ACT_ATTACK_RIGHT,	
} USER_ACTION;

typedef struct act_t
{
	USER_ACTION act;
	NEINT32 index;
} act_t;

#define USER_BOY  (0x0001)
#define USER_GIRL  (0x0002)
#define MONSTER_1  (0x0010)
#define MONSTER_2  (0x0020)
typedef struct player_base_t
{
	NEUINT16 type;
	NEINT8 rolename[NORMAL_SHORT_STR_LEN];
	NEFLOAT pos_x, pos_y;
} player_base_t;
typedef struct user_info_t
{
	struct list_head quad_lst;
	player_base_t base;
	NEUINT32 id;
	NEINT32 level;
	NEINT32 hp;
	NEINT32 mp;
	NEINT32 attack;
	NEINT32 recover;
	act_t act;
	NEINT32 money;
	NEINT32 experience;
	NEINT32 addedattack;
	NEINT32 addedrecover;
	NEINT32 rollcall;
	struct list_head *quad_head;  // not used yet
} user_info_t;


typedef struct login_ack_t
{
	NEINT32 result;
	user_info_t user_info;
} login_ack_t;


typedef struct moveto_newmap_t
{
	NEINT32 id;
	user_info_t user_info;	
} moveto_newmap_t;

typedef struct user_move_req_t
{
	NEINT32 action;
	NEFLOAT x;
	NEFLOAT y;
} user_move_req_t;

typedef struct user_move_ack_t
{
	NEINT32 result;
	NEINT32 action;
	NEFLOAT x;
	NEFLOAT y;
} user_move_ack_t;

typedef struct user_action_t
{
	NEINT8 rolename[NORMAL_SHORT_STR_LEN];
	act_t action;
	NEUINT32 param;
	NEFLOAT x;
	NEFLOAT y;
} user_action_t;

typedef struct monster_action_t
{
	NEUINT32 id;
	act_t action;
	NEUINT32 param;
	NEFLOAT x;
	NEFLOAT y;
} monster_action_t;

typedef struct user_scene_req_t
{
	quadbox_t box;
} user_scene_req_t;

typedef struct user_scene_ack_t
{
	NEUINT32 num;
	player_base_t player[0];
} user_scene_ack_t;
typedef struct monster_base_t
{
	NEUINT16 type;
	NEUINT32 id;
	NEUINT32 maxHP;
	NEUINT32 curHP;
	NEFLOAT  pos_x, pos_y;	
} monster_base_t;
typedef struct monster_scene_ack_t
{
	NEUINT32 num;
	monster_base_t monster[0];	
} monster_scene_ack_t;

typedef struct broadcast_info_t
{
	NEINT32 real_maxid;
	NEINT32 real_minid;
	NEINT32 usernum;
	NEINT32 userid[0];
} broadcast_info_t;

typedef struct brd_torange_info_t
{
	NEINT32 real_maxid;
	NEINT32 real_minid;
	quadbox_t box;
} brd_torange_info_t;

#pragma pack()

#endif
