
#if !defined(AFX_OEM_H__0EC4934C_6642_4DA5_AFE7_193E2CA3D89B__INCLUDED_)
#define AFX_OEM_H__0EC4934C_6642_4DA5_AFE7_193E2CA3D89B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "StdAfx.h"
#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include "ne_cliapp\ne_cliapp.h"


typedef struct NEPOINT
{
    NEFLOAT  x;
    NEFLOAT  y;
} NEPOINT;

//////////////////////////////////////////////////////////////////////////
//定义英雄结构 存档及战斗用信息
typedef struct HERO 
{
	int Life; //生命
	int attack; //攻击力
	int recover; //防御力
	NEPOINT position; //英雄坐标
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
	NEPOINT position;
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

extern int LoseLife;
extern HERO hr;
extern ENEMY enemy[100];
extern TCHAR lose[256];
extern Player players[MAX_OTHER_USER];

//////////////////////////////////////////////////////////////////////////


ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

//////////////////////////////////////////////////////////////////////////
// callback message WM_PAINT
void Main_OnPaint(HWND hwnd);
void Main_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
void HeroPaint(HDC hdc, HDC memDC);
void OtherUserPaint(HDC hdc, HDC memDC);
void LoadMap(HDC hdc,HDC memDC);
//void SenceMove(HWND hwnd,HDC hdc,HDC memDC,int Screen);
//void EnemyPaint(HWND hwnd,HDC hdc,HDC memDC);
//void BattleEnemyPaint(HDC hdc,HDC memDC);
//void PaintBattle(HWND hwnd,HDC hdc,HDC memDC);
//void PaintDead(HWND hwnd,HDC hdc,HDC memDC);
//void PaintBattleInfo(HWND hwnd,HDC hdc,HDC memDC);
//void PaintLevel(HWND hwnd,HDC hdc,HDC memDC);
//void PaintEnd(HWND hwnd,HDC hdc,HDC memDC);

int user_action_req(ne_handle handle, act_t act, int x, int y);
//////////////////////////////////////////////////////////////////////////
//battle
void InitHeroEnemyInfo(HWND hwnd);
int CheckLevelUP(HWND hwnd);
int Battle(HWND hwnd,int spring);

#endif // !defined(AFX_OEM_H__0EC4934C_6642_4DA5_AFE7_193E2CA3D89B__INCLUDED_)
