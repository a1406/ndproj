/*
#include "StdAfx.h"
#include <windows.h>
#include <windowsx.h>
#include "resource.h"
*/
#ifndef OEM_MAP_H
#define OEM_MAP_H

typedef struct  
{
	int Expect; //地图材质
	BOOL Block; //阻塞
	int Events; /*触发事件 事件表:   \r\n 
				(材质1)10 小史莱姆 11红史莱姆 12大史莱姆 13史莱姆王 14小蝙蝠 15大蝙蝠 16红蝙蝠 17吸血鬼 \r\n
	                         18骷髅 19骷髅战士 20黄金骷髅 21骷髅将军 22泥怪 23青泥怪 24红泥怪  25兽人 26兽人战士 \r\n
							 27蓝袍巫师 28红袍巫师 29幻影战士 \r\n
							   未完成

				*/
	int Global; //场景偏移,作为多关卡 1战斗场景  5关卡偏移 
}MAP,*LPMAP;

#endif
