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
	int Expect; //��ͼ����
	BOOL Block; //����
	int Events; /*�����¼� �¼���:   \r\n 
				(����1)10 Сʷ��ķ 11��ʷ��ķ 12��ʷ��ķ 13ʷ��ķ�� 14С���� 15������ 16������ 17��Ѫ�� \r\n
	                         18���� 19����սʿ 20�ƽ����� 21���ý��� 22��� 23����� 24�����  25���� 26����սʿ \r\n
							 27������ʦ 28������ʦ 29��Ӱսʿ \r\n
							   δ���

				*/
	int Global; //����ƫ��,��Ϊ��ؿ� 1ս������  5�ؿ�ƫ�� 
}MAP,*LPMAP;

#endif
