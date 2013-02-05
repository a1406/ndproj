#ifndef _NOVICE_SERVER_H__
#define _NOVICE_SERVER_H__

//todo AI
typedef struct monster
{
	struct list_head quad_lst;	
	monster_base_t base;
	NEBOOL   observered;
	NEFLOAT  born_x, born_y;
} monster;







#endif  //_NOVICE_SERVER_H__
