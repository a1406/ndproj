/*
 * nechar ������TCHAR
 * ��Ϊ�Ҳ���ֱ�ʵ��ansi��unicode�����ư汾,ֻ�����ڱ���ʱ�������ѡ��!
 */
#ifndef _NECHAR_H_
#define _NECHAR_H_

#include <wchar.h>
#include <stdio.h>

#define NE_ESC		0x1b
#ifdef WIN32 

//#if _MSC_VER < 1300 // 1200 == VC++ 6.0

#define snprintf 		_snprintf
#define snwprintf 		_snwprintf
#define vsnwprintf		_vsnwprintf
#define vsnprintf		_vsnprintf

//#else 
//#endif

#endif 

#ifdef NE_UNICODE

#define nechar_t wchar_t
#define _NET(x)		L ## x
#define neisalnum     iswalnum		//�����ַ��Ƿ�Ϊ���ֻ���ĸ 
#define neisalpha     iswalpha 		// �����ַ��Ƿ�����ĸ 
#define neiscntrl     iswcntrl 		//�����ַ��Ƿ��ǿ��Ʒ� 
#define neisdigit     iswdigit 		//�����ַ��Ƿ�Ϊ���� 
#define neisgraph     iswgraph 		//�����ַ��Ƿ��ǿɼ��ַ� 
#define neislower     iswlower 		//�����ַ��Ƿ���Сд�ַ� 
#define neisprint     iswprint 		//�����ַ��Ƿ��ǿɴ�ӡ�ַ� 
#define neispunct     iswpunct 		//�����ַ��Ƿ��Ǳ����� 
#define neisspace     iswspace 		//�����ַ��Ƿ��ǿհ׷��� 
#define neisupper     iswupper 		//�����ַ��Ƿ��Ǵ�д�ַ� 
#define neisxdigit    iswxdigit		//�����ַ��Ƿ���ʮ�����Ƶ����� 

#define netolower     towlower 		//���ַ�ת��ΪСд 
#define netoupper     towupper 		//���ַ�ת��Ϊ��д 
#define necscoll      wcscoll 		//�Ƚ��ַ��� 

/*
��ӡ��ɨ���ַ����� 
���ַ��������� 
*/
#define nefprintf		fwprintf     //ʹ��vararg�����ĸ�ʽ����� 
#define neprintf		wprintf      //ʹ��vararg�����ĸ�ʽ���������׼��� 
#define nesprintf		swprintf     //����vararg�������ʽ�����ַ��� 
#define nevfprintf		vfwprintf    //ʹ��stdarg�������ʽ��������ļ� 
#define nevsprintf		vsnwprintf    //��ʽ��stdarg������д���ַ��� 

#define nesnprintf 		snwprintf

#define nestrtod 		wcstod    //�ѿ��ַ��ĳ�ʼ����ת��Ϊ˫���ȸ����� 
#define nestrtol		wcstol     //�ѿ��ַ��ĳ�ʼ����ת��Ϊ������ 
#define nestrtoul		wcstoul    //���ַ��ĳ�ʼ����ת��Ϊ�޷��ų����� 

/*
�ַ��������� 
���ַ�����        ��ͨC�������� 
*/
#define nestrcat	wcscat			//strcat��һ���ַ����ӵ���һ���ַ�����β�� 
#define nestrncat	wcsncat			//strncat
#define nestrchr	wcschr			//strchr�������ַ����ĵ�һ��λ�� 
#define nestrrchr	wcsrchr			// strrchr����     ��β����ʼ�������ַ������ֵĵ�һ��λ�� 
#define nestrpbrk	wcspbrk         //strpbrk����     ��һ�ַ��ַ����в�����һ�ַ������κ�һ���ַ���һ�γ��ֵ�λ�� 
#define nestrstr	wcsstr			//    strstr����     ��һ�ַ����в�����һ�ַ�����һ�γ��ֵ�λ�� 
#define nestrcspn	wcscspn			//����         strcspn����     ���ز������ڶ����ַ����ĵĳ�ʼ��Ŀ 
#define nestrspn	wcsspn			//����         strspn����     ���ذ����ڶ����ַ����ĳ�ʼ��Ŀ 
#define nestrcpy	wcscpy			//����         strcpy����     �����ַ��� 
#define nestrncpy	wcsncpy			//����         strncpy����     ������wcscpy������ ͬʱָ����������Ŀ 
#define nestrcmp	wcscmp			//����         strcmp����     �Ƚ��������ַ��� 
#define nestrncmp	wcsncmp			//����         strncmp����     ������wcscmp������ ��Ҫָ���Ƚ��ַ��ַ�������Ŀ 
#define nestrlen	wcslen			//����         strlen����     ��ÿ��ַ�������Ŀ 
#define nestrtok	wcstok			//����         strtok����     ���ݱ�ʾ���ѿ��ַ����ֽ��һϵ���ַ��� 

#else		//ansi
#define _NET(x) x
#define nechar_t char

#define neisalnum     isalnum		//�����ַ��Ƿ�Ϊ���ֻ���ĸ 
#define neisalpha     isalpha 		// �����ַ��Ƿ�����ĸ 
#define neiscntrl     iscntrl 		//�����ַ��Ƿ��ǿ��Ʒ� 
#define neisdigit     isdigit 		//�����ַ��Ƿ�Ϊ���� 
#define neisgraph     isgraph 		//�����ַ��Ƿ��ǿɼ��ַ� 
#define neislower     islower 		//�����ַ��Ƿ���Сд�ַ� 
#define neisprint     isprint 		//�����ַ��Ƿ��ǿɴ�ӡ�ַ� 
#define neispunct     ispunct 		//�����ַ��Ƿ��Ǳ����� 
#define neisspace     isspace 		//�����ַ��Ƿ��ǿհ׷��� 
#define neisupper     isupper 		//�����ַ��Ƿ��Ǵ�д�ַ� 
#define neisxdigit     isxdigit		//�����ַ��Ƿ���ʮ�����Ƶ����� 

#define netolower     tolower 		//���ַ�ת��ΪСд 
#define netoupper     toupper 		//���ַ�ת��Ϊ��д 
#define necscoll     strcoll 		//�Ƚ��ַ��� 

/*
��ӡ��ɨ���ַ����� 
���ַ��������� 
*/
#define nefprintf		fprintf     //ʹ��vararg�����ĸ�ʽ����� 
#define neprintf		printf      //ʹ��vararg�����ĸ�ʽ���������׼��� 
#define nesprintf		sprintf     //����vararg�������ʽ�����ַ��� 
#define nevfprintf		vfprintf    //ʹ��stdarg�������ʽ��������ļ� 
#define nevsprintf		vsnprintf    //��ʽ��stdarg������д���ַ��� 
#define nesnprintf 		snprintf

#define nestrtod 		strtod		//�ѿ��ַ��ĳ�ʼ����ת��Ϊ˫���ȸ����� 
#define nestrtol		strtol		//�ѿ��ַ��ĳ�ʼ����ת��Ϊ������ 
#define nestrtoul		strtoul		//�ѿ��ַ��ĳ�ʼ����ת��Ϊ�޷��ų����� 

/*
�ַ��������� 
���ַ�����        ��ͨC�������� 
*/
#define nestrcat	strcat			//��һ���ַ����ӵ���һ���ַ�����β�� 
#define nestrncat	strncat			//����ָ��ճ���ַ�����ճ�ӳ���. 
#define nestrchr	strchr			//�������ַ����ĵ�һ��λ�� 
#define nestrrchr	strrchr			//��β����ʼ�������ַ������ֵĵ�һ��λ�� 
#define nestrpbrk	strpbrk			//��һ�ַ��ַ����в�����һ�ַ������κ�һ���ַ���һ�γ��ֵ�λ�� 
#define nestrstr	strstr			//��һ�ַ����в�����һ�ַ�����һ�γ��ֵ�λ�� 
#define nestrcspn	strcspn			//���ز������ڶ����ַ����ĵĳ�ʼ��Ŀ 
#define nestrspn	strspn			//���ذ����ڶ����ַ����ĳ�ʼ��Ŀ 
#define nestrcpy	strcpy			//�����ַ��� 
#define nestrncpy	strncpy			//ͬʱָ����������Ŀ 
#define nestrcmp	strcmp			//�Ƚ��������ַ��� 
#define nestrncmp	strncmp			//ָ���Ƚ��ַ��ַ�������Ŀ 
#define nestrlen	strlen			// ��ÿ��ַ�������Ŀ 
#define nestrtok	strtok

#endif //NE_UNICODE

#endif
