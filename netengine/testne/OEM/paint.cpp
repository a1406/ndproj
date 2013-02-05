#include "stdafx.h"
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
#include "ne_common/ne_os.h"
#include "ne_quadtree/quadtree.h"
#include "demomsg.h"
#include "OEM.h"
#include <wingdi.h>
#include "map.h"

extern ne_handle g_connect_handle;


extern "C" ne_connector_update(ne_handle net_handle, netime_t timeout) ;
extern "C" netime_t ne_time(void) ;
extern "C" NEINT32 ne_msgentry_install(ne_handle  handle, ne_usermsg_func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) ;
extern "C" void ne_fclose_dbg(FILE *fp);
extern "C" NEINT32 tcpnode_parse_recv_msgex(struct ne_tcp_node *node,NENET_MSGENTRY msg_entry , void *param) ;

//POINT hero; //Ӣ������   //����
#define hero  hr.position

MAP maps[20][16];

int toolgate=1;
//int Mkeydown=0; // 0�� 1�� 2�� 3�� 4��
//int idown=0,iup=0,ileft=0,iright=0;

TCHAR money[256];
TCHAR lose[256];
//////////////////////////////////////////////////////////////////////////
//Ӣ��
#define HERODOWN 0,0,32,31  //Ӣ������	
#define HERODOWN1 32,0,32,31 //��1�߶�
#define HERODOWN2 96,0,32,31 //��2�߶�

#define HEROLEFT 0,32,31,31
#define HEROLEFT1 32,32,32,31
#define HEROLEFT2 96,32,32,31

#define HEROUP 0,94,32,30
#define HEROUP1 32,94,32,30
#define HEROUP2 96,94,32,30

#define HERORIGHT 0,64,31,30
#define HERORIGHT1 32,64,32,30
#define HERORIGHT2 96,64,32,30

//////////////////////////////////////////////////////////////////////////
//����
#define EXPECT1 0,0,32,32 //����1 ש
#define EXPECT2 192,32,32,32 //����2 �۽�
#define EXPECT3 160,32,32,32 //�ǿ�
#define EXPECT4 224,32,32,32 //����
#define EXPECT5 160,0,32,32 //ѩ��
#define EXPECT6 192,0,32,32 //ש
#define EXPECT7 0,64,32,32 //�ݵ�
#define EXPECT8 224,0,32,32 //��ɫש
#define EXPECT9 224,224,32,32 //ľ�ذ�
#define EXPECT10 160,64,62,62 //��



//////////////////////////////////////////////////////////////////////////
//���˲���1
/*�����¼� �¼���:   \r\n 
(����1)10 Сʷ��ķ 11��ʷ��ķ 12��ʷ��ķ 13ʷ��ķ�� 14С���� 15������ 16������ 17��Ѫ�� \r\n
18���� 19����սʿ 20�ƽ����� 21���ý��� 22��� 23����� 24�����  25���� 26����սʿ \r\n
27������ʦ 28������ʦ 29��Ӱսʿ \r\n
(����2)30����ҩˮ 31��+ 32��+ 33���� 34���� 35��¥�� 36��¥��
δ���

*/
#define ENEMY1_1 0,0,32,32 // Сʷ��ķ
#define ENEMY1_2 32,0,32,32 //��ʷ��ķ
#define ENEMY1_3 64,0,32,32 //��ʷ��ķ
#define ENEMY1_4 96,0,32,32 //ʷ��ķ��
#define ENEMY1_5 128,0,32,32 //С����
#define ENEMY1_6 160,0,32,32 //������
#define ENEMY1_7 192,0,32,32 //������
#define ENEMY1_8 224,0,32,32 //��Ѫ��
#define ENEMY1_9 256,0,32,32 //����
#define ENEMY1_10 0,32,32,32 //����սʿ
#define ENEMY1_11 32,32,32,32 //�ƽ�����
#define ENEMY1_12 64,32,32,32 //���ý���
#define ENEMY1_13 352,0,32,32 //���
#define ENEMY1_14 288,32,32,32 //�����
#define ENEMY1_15 320,32,32,32 //�����
#define ENEMY1_16 96,32,32,32 //����
#define ENEMY1_17 128,32,32,32 //����սʿ
#define ENEMY1_18 224,32,32,32 //������ʦ
#define ENEMY1_19 256,32,32,32 //������ʦ
#define ENEMY1_20 192,32,32,32 //��Ӱսʿ

#define ENEMY2_1 0,32,32,32 //����ҩˮ
#define ENEMY2_2 0,0,32,32 //����+
#define ENEMY2_3 32,0,32,32 //��+
#define ENEMY2_4 0,128,32,32 //����
#define ENEMY2_5 0,224,32,32 //����
#define ENEMY2_6 0,96,32,32 //��¥��
#define ENEMY2_7 32,96,32,32 //��¥��
//////////////////////////////////////////////////////////////////////////



RECT InvalideHero;



////////////////////////////////////////////////////////////////////////////////
//  Main_OnPaint
void Main_OnPaint(HWND hwnd)
{
	HINSTANCE hi=(HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);
	PAINTSTRUCT ps;
	HDC hdc;
	hdc = BeginPaint(hwnd, &ps);	
	HDC memDC=CreateCompatibleDC(hdc);
	//	HDC memDC2=CreateCompatibleDC(hdc);
	//if (maps[hero.x][hero.y].Events!=0)
	//{
	//	//����ƫ��
	//	SenceMove(hwnd,hdc,memDC,maps[hero.x][hero.y].Global);
	//}
	//else
	//{
	//////////////////////////////////////////////////////////////////////////
	//Map
	HBITMAP BitMap=LoadBitmap(hi,MAKEINTRESOURCE(IDB_WALL));
	SelectObject(memDC,BitMap);
	LoadMap(hdc,memDC);
	DeleteObject(BitMap);
	//////////////////////////////////////////////////////////////////////////
	//Hero
	HBITMAP Hero=LoadBitmap(hi,MAKEINTRESOURCE(IDB_HERO));
	SelectObject(memDC,Hero);
	HeroPaint(hdc, memDC);
	OtherUserPaint(hdc, memDC);
	DeleteObject(Hero);
	//////////////////////////////////////////////////////////////////////////
	//Enemy	
	//EnemyPaint(hwnd,hdc,memDC);

	//}


	//////////////////////////////////////////////////////////////////////////
	ReleaseDC(hwnd,memDC);
	EndPaint(hwnd, &ps);
}


//////////////////////////////////////////////////////////////////////////

static void PeoplePaint(HDC hdc, int x, int y, act_t act, HDC memDC)
{
	int index = act.index;
	switch (act.act)
	{
	case 0:
		{
			TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERODOWN,RGB(0,0,0));
		}break;
	case ACT_MOV_DOWN: //��
		{
			if (index%3==0)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERODOWN,RGB(0,0,0));
			}
			else if(index%3==1)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERODOWN1,RGB(0,0,0));
			}
			else if(index%3==2)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERODOWN2,RGB(0,0,0));
			}
		}break;
	case ACT_MOV_LEFT:	//��
		{
			if (index%3==0)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HEROLEFT,RGB(0,0,0));
			}
			else if(index%3==1)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HEROLEFT1,RGB(0,0,0));
			}
			else if(index%3==2)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HEROLEFT2,RGB(0,0,0));
			}
		}break;
	case ACT_MOV_UP: //��
		{
			if (index%3==0)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HEROUP,RGB(0,0,0));
			}
			else if(index%3==1)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HEROUP1,RGB(0,0,0));
			}
			else if(index%3==2)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HEROUP2,RGB(0,0,0));
			}
		}break;
	case ACT_MOV_RIGHT: //��
		{
			if (index%3==0)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERORIGHT,RGB(0,0,0));
			}
			else if(index%3==1)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERORIGHT1,RGB(0,0,0));
			}
			else if(index%3==2)
			{
				TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERORIGHT2,RGB(0,0,0));
			}
		}break;
	default:
		TransparentBlt(hdc,32*x,32*y,32,31,memDC,HERODOWN,RGB(0,0,0));
		break;
	}
}

//Ӣ������
void HeroPaint(HDC hdc, HDC memDC)
{
	/*	
	user_action_t act;
	;//TextOut(hdc, x, y - 20, name, lstrlen(name));
	switch (Mkeydown)
	{
	case 1: //��
	act.act = ACT_MOV_DOWN;
	act.index = idown;
	break;
	case 2:	//��
	act.act = ACT_MOV_LEFT;		
	act.index = ileft;
	break;
	case 3: //��
	act.act = ACT_MOV_UP;				
	act.index = iup;
	break;
	case 4: //��
	act.act = ACT_MOV_RIGH;
	act.index = iright;
	}
	*/
	PeoplePaint(hdc, (int)hero.x, (int)hero.y, hr.act, memDC);
}

void OtherUserPaint(HDC hdc, HDC memDC)
{
	int i;
	for (i = 0; i < MAX_OTHER_USER; ++i)
	{
		if (players[i].rolename[0] == NULL)
			return;
		PeoplePaint(hdc, (int)players[i].position.x, (int)players[i].position.y, players[i].act, memDC);
	}
}

//////////////////////////////////////////////////////////////////////////
//��ȡ��ͼ
void LoadMap(HDC hdc,HDC memDC)
{
	int i,j;
	for (i=0;i<20;i++)
	{
		for (j=0;j<15;j++)
		{
			////switch (maps[i][j].Expect)
			////{
			////case 0:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT1,RGB(255,255,255));
			////	break;
			////case 1:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT2,RGB(255,255,255));
			////	break;
			////case 2:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT3,RGB(255,255,255));
			////	break;
			////case 3:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT4,RGB(255,255,255));
			////	break;
			////case 4:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT5,RGB(255,255,255));
			////	break;
			////case 5:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT6,RGB(255,255,255));
			////	break;
			////case 6:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT7,RGB(255,255,255));
			////	break;
			////case 7:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT8,RGB(255,255,255));
			////	break;
			////case 8:
			////	TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT9,RGB(255,255,255));
			////	break;
			////case 9:
			//////////////////////////////////////////////////////////////////////////
			//������ �˴�����
			TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT6,RGB(255,255,255));
			//////////////////////////////////////////////////////////////////////////
			//TransparentBlt(hdc,32*i,32*j,32,32,memDC,EXPECT10,RGB(255,255,255));
			//	break;
			//default:break;
			//}
		}
	}

}






////////////////////////////////////////////////////////////////////////////////
//  Main_OnKey
//E2 (����2)30����ҩˮ 31��+ 32��+ 33���� 34���� 35��¥�� 36��¥��
void Main_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	InvalideHero.left=32*((int)hero.x-2);
	InvalideHero.right=32*((int)hero.x+2);
	InvalideHero.top=32*((int)hero.y-2);
	InvalideHero.bottom=32*((int)hero.y+2);
	switch (vk)
	{
	case VK_DOWN:
		{	
			//			if (maps[hero.x][hero.y].Events==0)
			{
				hr.act.act = ACT_MOV_DOWN;
				++hr.act.index;
				//Mkeydown=1;
				if ((hero.y<14)/*&&(!maps[hero.x][hero.y+1].Block)*/) //��ײ·�����
				{
					hero.y++;
					//					idown++;
					//					if (maps[hero.x][hero.y].Events!=0)
					//					{

					InvalidateRect(hwnd,FALSE,NULL);
					return;
					//					}
					//					InvalidateRect(hwnd,&InvalideHero,FALSE);
				}
				else
				{

				}
			}

			//else if (maps[hero.x][hero.y].Events>29)
			//{
			//	switch (maps[hero.x][hero.y].Events)
			//	{
			//	case 30:
			//		{
			//			if (hr.Life<400)
			//			{
			//				hr.Life+=100;
			//				
			//			}
			//			else
			//			{
			//				hr.Life=500;
			//			}
			//			
			//		}
			//		break;
			//	case 31:
			//		{
			//			hr.attack+=20;
			//		}break;
			//	case 32:
			//		{
			//			hr.recover+=5;
			//		}break;
			//	case 33:
			//		{
			//			hr.addedattack+=20;
			//			
			//		}break;
			//	case 34:
			//		{
			//			hr.addrecover+=5;
			//	
			//		}break;
			//	case 35:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	case 36:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	default:break;
			//	}
			//	maps[hero.x][hero.y].Events=0;
			//	InvalidateRect(hwnd,&InvalideHero,FALSE);
			//}
		}break;
	case VK_LEFT:
		{
			//			if (maps[hero.x][hero.y].Events==0)
			{
				hr.act.act = ACT_MOV_LEFT;
				++hr.act.index;				
				//Mkeydown=2;
				if ((hero.x>0)/*&&(!maps[hero.x-1][hero.y].Block)*/)
				{
					hero.x--;
					//ileft++;
					//if (maps[hero.x][hero.y].Events!=0)
					//{

					InvalidateRect(hwnd,FALSE,NULL);
					return;
					//}
					//InvalidateRect(hwnd,&InvalideHero,FALSE);
				}
				else
				{

				}
			}
			//else if (maps[hero.x][hero.y].Events>29)
			//{
			//	switch (maps[hero.x][hero.y].Events)
			//	{
			//	case 30:
			//		{
			//			if (hr.Life<400)
			//			{
			//				hr.Life+=100;
			//			}
			//			else
			//			{
			//				hr.Life=500;
			//			}
			//		}
			//		break;
			//	case 31:
			//		{
			//			hr.attack+=20;
			//		}break;
			//	case 32:
			//		{
			//			hr.recover+=5;
			//		}break;
			//	case 33:
			//		{
			//			hr.addedattack+=20;
			//		}break;
			//	case 34:
			//		{
			//			hr.addrecover+=5;
			//		}break;
			//	case 35:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	case 36:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	default:break;
			//	}
			//	maps[hero.x][hero.y].Events=0;
			//	InvalidateRect(hwnd,&InvalideHero,FALSE);
			//}
		}break;
	case VK_RIGHT:
		{
			//			if (maps[hero.x][hero.y].Events==0)
			{
				hr.act.act = ACT_MOV_RIGHT;
				++hr.act.index;				
				//Mkeydown=4;
				if ((hero.x<19)/*&&(!maps[hero.x+1][hero.y].Block)*/)
				{
					hero.x++;
					//iright++;
					//if (maps[hero.x][hero.y].Events!=0)
					//{

					InvalidateRect(hwnd,FALSE,NULL);
					return;
					//}
					//InvalidateRect(hwnd,&InvalideHero,FALSE);
				}
				else
				{

				}
			}
			//else if (maps[hero.x][hero.y].Events>29)
			//{
			//	switch (maps[hero.x][hero.y].Events)
			//	{
			//	case 30:
			//		{
			//			if (hr.Life<400)
			//			{
			//				hr.Life+=100;
			//			}
			//			else
			//			{
			//				hr.Life=500;
			//			}
			//		}
			//		break;
			//	case 31:
			//		{
			//			hr.attack+=20;
			//		}break;
			//	case 32:
			//		{
			//			hr.recover+=5;
			//		}break;
			//	case 33:
			//		{
			//			hr.addedattack+=20;
			//		}break;
			//	case 34:
			//		{
			//			hr.addrecover+=5;
			//		}break;
			//	case 35:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	case 36:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	default:break;
			//	}
			//	maps[hero.x][hero.y].Events=0;
			//	InvalidateRect(hwnd,&InvalideHero,FALSE);
			//}
		}break;
	case VK_UP:
		{
			//			if (maps[hero.x][hero.y].Events==0)
			{
				hr.act.act = ACT_MOV_UP;
				++hr.act.index;				
				//Mkeydown=3;
				if ((hero.y>0)/*&&(!maps[hero.x][hero.y-1].Block)*/)
				{
					hero.y--;
					//iup++;
					//if (maps[hero.x][hero.y].Events!=0)
					//{

					InvalidateRect(hwnd,FALSE,NULL);
					return;
					//}
					//InvalidateRect(hwnd,&InvalideHero,FALSE);
				}
				else
				{

				}
			}
			//else if (maps[hero.x][hero.y].Events>29)
			//{
			//	switch (maps[hero.x][hero.y].Events)
			//	{
			//	case 30:
			//		{
			//			if (hr.Life<400)
			//			{
			//				hr.Life+=100;
			//			}
			//			else
			//			{
			//				hr.Life=500;
			//			}
			//		}
			//		break;
			//	case 31:
			//		{
			//			hr.attack+=20;
			//		}break;
			//	case 32:
			//		{
			//			hr.recover+=5;
			//		}break;
			//	case 33:
			//		{
			//			hr.addedattack+=20;
			//		}break;
			//	case 34:
			//		{
			//			hr.addrecover+=5;
			//		}break;
			//	case 35:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	case 36:
			//		{
			//			
			//			HDC hdc=GetDC(hwnd);
			//			HDC memDC=CreateCompatibleDC(hdc);
			//			LoadMap(hdc,memDC);
			//			ReleaseDC(hwnd,memDC);
			//			ReleaseDC(hwnd,hdc);
			//		}break;
			//	default:break;
			//	}
			//	maps[hero.x][hero.y].Events=0;
			//	InvalidateRect(hwnd,&InvalideHero,FALSE);
			//}
		}break;
		//case VK_RETURN:
		//	{
		//		if ((hr.Life>0)&&(maps[hero.x][hero.y].Events!=0))
		//		{
		//			maps[hero.x][hero.y].Events=0;
		//			maps[hero.x][hero.y].Global=0;
		//			InvalidateRect(hwnd,NULL,FALSE);
		//		}
		//	
		//	}break;
		//case VK_ESCAPE:
		//	{
		//		if (IDYES==MessageBox(hwnd,TEXT("ȷ���˳�?"),TEXT("ȷ����Ϣ"),MB_YESNO|MB_ICONINFORMATION|MB_DEFBUTTON2))
		//		{
		//			PostQuitMessage(0);
		//		}
		//		
		//	}break;
	default:break;
	}

	if (g_connect_handle) {
		user_action_req(g_connect_handle, hr.act, hero.x, hero.y);
	}
}


//////////////////////////////////////////////////////////////////////////
//����ƫ��
//void SenceMove(HWND hwnd,HDC hdc,HDC memDC,int Screen)
//{
//	switch (Screen)
//	{
//	case 1: //ս������
//		{
//			PaintBattle(hwnd,hdc,memDC);
//		}break;
//	case 2: //����ս������
//		{
//			PaintBattle(hwnd,hdc,memDC);
//			toolgate++;
//			_sleep(500);
//			InvalidateRect(hwnd,NULL,FALSE);
//			ZeroMemory(maps,sizeof(maps));
//			TCHAR togate[256];
//			ZeroMemory(togate,sizeof(togate)/sizeof(TCHAR));
//			wsprintf(togate,TEXT("%d.map"),toolgate);
//			FILE *fp;
//			fp=fopen((const NEINT8 *)togate, (const NEINT8 *)"r+");
//			fread(maps,sizeof(MAP),320,fp);
//			fclose(fp);
//			LoadMap(hdc,memDC);
//			SenceMove(hwnd,hdc,memDC,maps[hero.x][hero.y].Global);
//			
//		}break;
//	case 3: //�̵��ȡ
//		{
//			
//		}break;
//	case 4:
//		{
//			PaintEnd(hwnd,hdc,memDC);//��ȡ��������
//		}break;
//	case 5: //�ؿ�ƫ�� �� ¥�ݣ�������ȡ
//		{
//			switch (maps[hero.x][hero.y].Events)
//			{
//			case 35:
//				toolgate++;
//				break;
//			case 36:
//				toolgate--;
//				break;
//			default:
//				break;
//			}
//			ZeroMemory(maps,sizeof(maps));
//			TCHAR togate[256];
//			ZeroMemory(togate,sizeof(togate)/sizeof(TCHAR));
//			wsprintf(togate,TEXT("%d.map"),toolgate);
//			FILE *fp;
//			fp=fopen((const NEINT8 *)togate, (const NEINT8 *)"r+");
//			fread(maps,sizeof(MAP),320,fp);
//			fclose(fp);
//			LoadMap(hdc,memDC);
//			InvalidateRect(hwnd,NULL,FALSE);
//		}break;
//	case 99: //��������
//		{
//			_sleep(500);
//			PaintLevel(hwnd,hdc,memDC);
//		}break;
//	case 100: //��������
//		{
//			PaintDead(hwnd,hdc,memDC);
//		}break;
//
//	
//	default:break;
//	}
//}



//////////////////////////////////////////////////////////////////////////
//���������ȡ
void EnemyPaint(HWND hwnd,HDC hdc,HDC memDC)
{
	HINSTANCE hi=(HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);
	HBITMAP Enemy=LoadBitmap(hi,MAKEINTRESOURCE(IDB_ENEMY));
	HBITMAP Enemy2=LoadBitmap(hi,MAKEINTRESOURCE(IDB_ENEMY2));


	int i,j;
	for (i=0;i<20;i++)
	{
		for (j=0;j<15;j++)
		{
			switch (maps[i][j].Events)
			{
			case 0:
				break;
			case 10:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_1,RGB(47,47,47));
				break;
			case 11:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_2,RGB(47,47,47));
				break;
			case 12:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_3,RGB(47,47,47));
				break;
			case 13:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_4,RGB(47,47,47));
				break;
			case 14:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_5,RGB(47,47,47));
				break;
			case 15:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_6,RGB(47,47,47));
				break;
			case 16:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_7,RGB(47,47,47));
				break;
			case 17:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_8,RGB(47,47,47));
				break;
			case 18:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_9,RGB(47,47,47));
				break;
			case 19:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_10,RGB(47,47,47));
				break;
			case 20:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_11,RGB(47,47,47));
				break;
			case 21:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_12,RGB(47,47,47));
				break;
			case 22:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_13,RGB(47,47,47));
				break;
			case 23:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_14,RGB(47,47,47));
				break;
			case 24:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_15,RGB(47,47,47));
				break;
			case 25:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_16,RGB(47,47,47));
				break;
			case 26:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_17,RGB(47,47,47));
				break;
			case 27:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_18,RGB(47,47,47));
				break;
			case 28:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_19,RGB(47,47,47));
				break;
			case 29:
				SelectObject(memDC,Enemy);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY1_20,RGB(47,47,47));
				break;

				//////////////////////////////////////////////////////////////////////////
				//E2 (����2)30����ҩˮ 31��+ 32��+ 33���� 34���� 35��¥�� 36��¥��
			case 30:
				SelectObject(memDC,Enemy2);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY2_1,RGB(0,0,0));
				break;
			case 31:
				SelectObject(memDC,Enemy2);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY2_2,RGB(0,0,0));
				break;
			case 32:
				SelectObject(memDC,Enemy2);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY2_3,RGB(0,0,0));
				break;
			case 33:
				SelectObject(memDC,Enemy2);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY2_4,RGB(0,0,0));
				break;
			case 34:
				SelectObject(memDC,Enemy2);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY2_5,RGB(0,0,0));
				break;
			case 35:
				SelectObject(memDC,Enemy2);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY2_6,RGB(0,0,0));
				break;
			case 36:
				SelectObject(memDC,Enemy2);
				TransparentBlt(hdc,32*i,32*j,32,32,memDC,ENEMY2_7,RGB(0,0,0));
				break;
			default:break;
			}
		}
	}
	DeleteObject(Enemy);
	DeleteObject(Enemy2);
}



//////////////////////////////////////////////////////////////////////////
//ս����������ȡ
//void BattleEnemyPaint(HDC hdc,HDC memDC)
//{
//			switch (maps[hero.x][hero.y].Events)
//			{
//			case 0:
//				break;
//			case 10:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_1,RGB(47,47,47));
//				break;
//			case 11:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_2,RGB(47,47,47));
//				break;
//			case 12:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_3,RGB(47,47,47));
//				break;
//			case 13:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_4,RGB(47,47,47));
//				break;
//			case 14:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_5,RGB(47,47,47));
//				break;
//			case 15:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_6,RGB(47,47,47));
//				break;
//			case 16:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_7,RGB(47,47,47));
//				break;
//			case 17:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_8,RGB(47,47,47));
//				break;
//			case 18:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_9,RGB(47,47,47));
//				break;
//			case 19:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_10,RGB(47,47,47));
//				break;
//			case 20:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_11,RGB(47,47,47));
//				break;
//			case 21:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_12,RGB(47,47,47));
//				break;
//			case 22:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_13,RGB(47,47,47));
//				break;
//			case 23:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_14,RGB(47,47,47));
//				break;
//			case 24:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_15,RGB(47,47,47));
//				break;
//			case 25:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_16,RGB(47,47,47));
//				break;
//			case 26:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_17,RGB(47,47,47));
//				break;
//			case 27:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_18,RGB(47,47,47));
//				break;
//			case 28:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_19,RGB(47,47,47));
//				break;
//			case 29:
//				TransparentBlt(hdc,450,150,64,62,memDC,ENEMY1_20,RGB(47,47,47));
//				break;
//
//
//			default:break;
//			}
//}
//
////////////////////////////////////////////////////////////////////////////
////ս������
//void PaintBattle(HWND hwnd,HDC hdc,HDC memDC)
//{
//	RECT rt;
//	GetClientRect(hwnd, &rt);
//	FillRect(hdc,&rt,CreateSolidBrush(RGB(0,0,0)));
//	HINSTANCE hi=(HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);
//	HBITMAP VSBitmap=LoadBitmap(hi,MAKEINTRESOURCE(IDB_VS));
//	SelectObject(memDC,VSBitmap);
//	TransparentBlt(hdc,250,100,100,150,memDC,0,0,100,150,RGB(47,47,47));
//	DeleteObject(VSBitmap);
//	HBITMAP BitMap=LoadBitmap(hi,MAKEINTRESOURCE(IDB_HERO));
//	SelectObject(memDC,BitMap);
//	TransparentBlt(hdc,80,150,64,62,memDC,HERODOWN,RGB(0,0,0));
//	DeleteObject(BitMap);
//	HBITMAP EnemyBit=LoadBitmap(hi,MAKEINTRESOURCE(IDB_ENEMY));
//	SelectObject(memDC,EnemyBit);
//	BattleEnemyPaint(hdc,memDC);
//	DeleteObject(EnemyBit);
//	
//	//���ٶ����Ӣ�ۺ͵��˽ṹ������Ϣ
//	//////////////////////////////////////////////////////////////////////////
////	PaintBattleInfo(hwnd,hdc,memDC);
//	SetBkColor(hdc,RGB(0,0,0));
//	SetTextColor(hdc,RGB(255,255,0));
//	
//	int a=Battle(hwnd,maps[hero.x][hero.y].Events);
//	if (a==1) //����
//	{
//		
//		maps[hero.x][hero.y].Global=100;
//		InvalidateRect(hwnd,NULL,FALSE);
//		return;
//	}
//	TextOut(hdc,250,362,lose,lstrlen(lose));
//	
//	ZeroMemory(lose,sizeof(lose)/sizeof(TCHAR));
//	wsprintf(lose,TEXT("����ʧ�� %d ����"),LoseLife);
//	ZeroMemory(money,sizeof(money)/sizeof(TCHAR));
//	wsprintf(money,TEXT("����� %d ��Ǯ"),enemy[maps[hero.x][hero.y].Events].Money);
//	TextOut(hdc,450,360,money,lstrlen(money));
//	int Lup=CheckLevelUP(hwnd);
//	if (Lup==1)
//	{
//		maps[hero.x][hero.y].Global=99;
//		InvalidateRect(hwnd,NULL,FALSE);
//		
//	}
//	//////////////////////////////////////////////////////////////////////////
//}

//void PaintBattleInfo(HWND hwnd,HDC hdc,HDC memDC)
//{
//	SetBkColor(hdc,RGB(0,0,0));
//	SetTextColor(hdc,RGB(255,255,0));
//	TextOut(hdc,80,250,TEXT("����"),4); //����DRAWTEXT����
//	TextOut(hdc,450,250,TEXT("����"),4);
//	TextOut(hdc,80,290,TEXT("����"),4);
//	TextOut(hdc,450,290,TEXT("����"),4);
//	TextOut(hdc,450,330,TEXT("����"),4);
//	TextOut(hdc,80,330,TEXT("����"),4);
//	TCHAR buff[256];
//	ZeroMemory(buff,sizeof(buff)/sizeof(TCHAR));
//	wsprintf(buff,TEXT("��Ǯ:   %d   "),hr.Money);
//	TextOut(hdc,80,360,buff,lstrlen(buff));
//	TextOut(hdc,250,400,TEXT("Press Enter to continue!   "),26);
//
//	HPEN LifePen=CreatePen(PS_SOLID,10,RGB(255,0,0));
//	SelectObject(hdc,LifePen);
//	MoveToEx(hdc,510,255,NULL);
//	LineTo(hdc,510+(enemy[maps[hero.x][hero.y].Events].Life/3),255);
//	MoveToEx(hdc,140,255,NULL);
//	LineTo(hdc,140+(hr.Life/3),255);
//
//	DeletePen(LifePen);
//
//
//	HPEN AttackPen=CreatePen(PS_SOLID,10,RGB(50,50,255));
//	SelectObject(hdc,AttackPen);
//	MoveToEx(hdc,140,295,NULL);
//	LineTo(hdc,140+hr.attack,295);
//	MoveToEx(hdc,510,295,NULL);
//	LineTo(hdc,510+(enemy[maps[hero.x][hero.y].Events].attack),295);
//
//	DeletePen(AttackPen);
//	
//
//	HPEN RecoverPen=CreatePen(PS_SOLID,10,RGB(0,255,125));
//	SelectObject(hdc,RecoverPen);
//	MoveToEx(hdc,140,335,NULL);
//	LineTo(hdc,140+hr.recover,335);
//	MoveToEx(hdc,510,335,NULL);
//	LineTo(hdc,510+(enemy[maps[hero.x][hero.y].Events].recover),335);
//	DeletePen(RecoverPen);
//}
//
//
//
//void PaintDead(HWND hwnd,HDC hdc,HDC memDC)
//{
//	RECT rc;
//	GetClientRect(hwnd,&rc);
//	FillRect(hdc,&rc,CreateSolidBrush(RGB(0,0,0)));
//	HINSTANCE hi=(HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);
//	HBITMAP DeadMap=LoadBitmap(hi,MAKEINTRESOURCE(IDB_DEAD));
//	SelectObject(memDC,DeadMap);
//	TransparentBlt(hdc,100,100,400,250,memDC,0,0,400,140,RGB(255,255,255));
//	DeleteObject(DeadMap);
//}
//
//
//void PaintLevel(HWND hwnd,HDC hdc,HDC memDC)
//{
//	RECT rc;
//	GetClientRect(hwnd,&rc);
//	FillRect(hdc,&rc,CreateSolidBrush(RGB(0,0,0)));
//	HINSTANCE hi=(HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE);
//	HBITMAP DeadMap=LoadBitmap(hi,MAKEINTRESOURCE(IDB_LEVELUP));
//	SelectObject(memDC,DeadMap);
//	TransparentBlt(hdc,100,50,400,86,memDC,0,0,427,85,RGB(254,254,254));
//	DeleteObject(DeadMap);
//	HBITMAP BitMap=LoadBitmap(hi,MAKEINTRESOURCE(IDB_HERO));
//	SelectObject(memDC,BitMap);
//	TransparentBlt(hdc,80,220,64,62,memDC,HERODOWN,RGB(0,0,0));
//	DeleteObject(BitMap);
//	SetBkColor(hdc,RGB(0,0,0));
//	SetTextColor(hdc,RGB(255,255,0));
//	TextOut(hdc,250,180,TEXT("����:"),5);
//	TextOut(hdc,250,260,TEXT("����:"),5);
//	TextOut(hdc,250,340,TEXT("����:"),5);
//	TextOut(hdc,250,400,TEXT("Press Enter to continue!   "),26);
//
//	HPEN LifePen=CreatePen(PS_SOLID,10,RGB(255,0,0));
//	SelectObject(hdc,LifePen);
//	MoveToEx(hdc,300,185,NULL);
//	LineTo(hdc,300+(hr.Life/3),185);
//	DeletePen(LifePen);
//
//	HPEN AttackPen=CreatePen(PS_SOLID,10,RGB(50,50,255));
//	SelectObject(hdc,AttackPen);
//	MoveToEx(hdc,300,265,NULL);
//	LineTo(hdc,300+hr.attack,265);
//	DeletePen(AttackPen);
//
//	HPEN RecoverPen=CreatePen(PS_SOLID,10,RGB(0,255,125));
//	SelectObject(hdc,RecoverPen);
//	MoveToEx(hdc,300,345,NULL);
//	LineTo(hdc,300+hr.recover,345);
//	DeletePen(RecoverPen);
//}


//void PaintEnd(HWND hwnd,HDC hdc,HDC memDC)
//{
//	SetBkMode(hdc,TRANSPARENT);
//	SetTextColor(hdc,RGB(138,140,255));
//	TextOut(hdc,240,20,TEXT(" ���������Ѫ�����ı������������         "),35);
//	TextOut(hdc,320,50,TEXT("��! "),4);
//}
