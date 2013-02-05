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

HERO hr;
ENEMY enemy[100];
int LoseLife=0;
Player players[MAX_OTHER_USER];

void InitHeroEnemyInfo(HWND hwnd)
{
	//Ӣ��
	hr.Life=100;
	hr.attack=20;
	hr.recover=5;
	hr.Money=0;
	hr.Experience=0;
	//////////////////////////////////////////////////////////////////////////
	//����
	//Сʷ��ķ
	enemy[10].Life=20;
	enemy[10].attack=10;
	enemy[10].recover=1;
	enemy[10].Money=2;
	enemy[10].Experience=20;
	//��ʷ��ķ
	enemy[11].Life=25;
	enemy[11].attack=12;
	enemy[11].recover=1;
	enemy[11].Money=4;
	enemy[11].Experience=40;
	//��ʷ��ķ
	enemy[12].Life=30;
	enemy[12].attack=15;
	enemy[12].recover=6;
	enemy[12].Money=10;
	enemy[12].Experience=80;
	//ʷ��ķ��
	enemy[13].Life=100;
	enemy[13].attack=20;
	enemy[13].recover=10;
	enemy[13].Money=100;
	enemy[13].Experience=200;
	
	//С����
	enemy[14].Life=25;
	enemy[14].attack=10;
	enemy[14].recover=4;
	enemy[14].Money=3;
	enemy[14].Experience=30;
	//������
	enemy[15].Life=35;
	enemy[15].attack=16;
	enemy[15].recover=6;
	enemy[15].Money=20;
	enemy[15].Experience=80;
	//������
	enemy[16].Life=50;
	enemy[16].attack=20;
	enemy[16].recover=8;
	enemy[16].Money=25;
	enemy[16].Experience=90;
	//��Ѫ��
	enemy[17].Life=450;
	enemy[17].attack=30;
	enemy[17].recover=8;
	enemy[17].Money=100;
	enemy[17].Experience=500;
	
	//����
	enemy[18].Life=100;
	enemy[18].attack=10;
	enemy[18].recover=0;
	enemy[18].Money=0;
	enemy[18].Experience=90;
	//����սʿ
	enemy[19].Life=140;
	enemy[19].attack=14;
	enemy[19].recover=4;
	enemy[19].Money=20;
	enemy[19].Experience=120;
	//�ƽ�����
	enemy[20].Life=180;
	enemy[20].attack=20;
	enemy[20].recover=10;
	enemy[20].Money=150;
	enemy[20].Experience=150;
	//���ý���
	enemy[21].Life=200;
	enemy[21].attack=25;
	enemy[21].recover=12;
	enemy[21].Money=150;
	enemy[21].Experience=250;
	
	//���
	enemy[22].Life=50;
	enemy[22].attack=10;
	enemy[22].recover=4;
	enemy[22].Money=50;
	enemy[22].Experience=50;
	//�����
	enemy[23].Life=60;
	enemy[23].attack=12;
	enemy[23].recover=6;
	enemy[23].Money=55;
	enemy[23].Experience=60;
	//�����
	enemy[24].Life=80;
	enemy[24].attack=18;
	enemy[24].recover=2;
	enemy[24].Money=80;
	enemy[24].Experience=100;

	//����
	enemy[25].Life=200;
	enemy[25].attack=18;
	enemy[25].recover=8;
	enemy[25].Money=50;
	enemy[25].Experience=150;
	//����սʿ
	enemy[26].Life=250;
	enemy[26].attack=22;
	enemy[26].recover=10;
	enemy[26].Money=100;
	enemy[26].Experience=220;
	//������ʦ
	enemy[27].Life=80;
	enemy[27].attack=25;
	enemy[27].recover=1;
	enemy[27].Money=250;
	enemy[27].Experience=240;
	//������ʦ
	enemy[28].Life=100;
	enemy[28].attack=30;
	enemy[28].recover=1;
	enemy[28].Money=300;
	enemy[28].Experience=320;
	//��Ӱսʿ
	enemy[29].Life=500;
	enemy[29].attack=22;
	enemy[29].recover=10;
	enemy[29].Money=100;
	enemy[29].Experience=500;
}


//����Ƿ�����
int CheckLevelUP(HWND hwnd)
{
	if (hr.Experience>500)
	{
		if (hr.Life<=400)
		{
			hr.Life+=100;
			hr.attack+=20;
			hr.recover+=5;
		}
		else
		{
			hr.Life=500;
			hr.attack+=20;
			hr.recover+=5;
		}
		hr.Experience=0;
		return 1;
	}
	return 0;
}

//ս�� Ӣ����������1�����򷵻�0
int Battle(HWND hwnd,int spring)
{
	LoseLife=0;
	while (enemy[spring].Life>0)
	{
		if (enemy[spring].recover<hr.attack)//Ӣ�۹������ڹ������
		{
			enemy[spring].Life=(enemy[spring].Life+enemy[spring].recover)-hr.attack;
			if (enemy[spring].Life>0)
			{
				if (hr.recover<enemy[spring].attack) //���﹥��������Ӣ�۷���
				{
					LoseLife+=(enemy[spring].attack-hr.recover);
				
					hr.Life=(hr.Life+hr.recover)-enemy[spring].attack;
					Sleep(200);
					InvalidateRect(hwnd,FALSE,NULL);	
					if (hr.Life<=0)
					{
						return 1;
					}
				}
				else
				{
					
				}
			}
		}
		else if (enemy[spring].recover>hr.attack)//�����������Ӣ�۹���
		{
			LoseLife+=enemy[spring].attack-hr.Life;
			hr.Life=(hr.Life+hr.recover)-enemy[spring].attack;
			if (hr.Life<=0)
			{
				return 1;
			}
		}
		
	}
	hr.Money+=enemy[spring].Money;
	hr.Experience+=enemy[spring].Experience;
	return 0;
}
