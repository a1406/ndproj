#ifndef _NE_STR_H_
#define _NE_STR_H_

#include "ne_common/ne_common.h"
 
#define _DOT  '.'
#define _MINUS '-' 
#define _NE_SPACE			0x20
#define _NE_TAB				0x09
#define _NE_QUOT			0x22		//˫����
#define _NE_SINGLE_QUOT		0x27		//������
#define IS_NUMERALS(a)		((a) >= '0' && (a) <='9')

#define IS_PRINTABLE(a)		((a) >= 0x20 && (a) < 0x7f)

#define IS_BIG_LATIN(a)		( (a) >= 'A' && (a)<='Z')
#define IS_LITTLE_LATIN(a)	( (a) >= 'a' && (a)<='z')
#define BIG_2_LITTLE(a)		(a)+0x20 
#define LITTLE_2_BIG(a)		(a)-0x20 

/* ȥ���ַ�����ͷ�������õ��ַ������ɴ�ӡ���ַ���*/
NE_COMMON_API NEINT8 *nestr_first_valid(const NEINT8 *src) ;

/* ����ַ��Ƿ�����Ч������*/
NE_COMMON_API NEINT32 nestr_is_numerals(const NEINT8 *src);

//����ַ����Ƿ���Ȼ��
NE_COMMON_API NEINT32 nestr_is_naturalnumber(const NEINT8 *src);

NE_COMMON_API NEINT8 *nestr_read_numerals(const NEINT8 *src, NEINT8 *desc, NEINT32 *isok) ;

//�ֽ�һ������,����ֻ��������,��ĸ���»���
NE_COMMON_API NEINT8 *nestr_parse_word(NEINT8 *src, NEINT8 *outstr);
NE_COMMON_API NEINT8 *nestr_parse_word_n(NEINT8 *src, NEINT8 *outstr, NEINT32 n) ;

/*��ȡһ���ַ�����֪������һ���ƶ��Ľ����ַ�Ϊֹ*/
NE_COMMON_API NEINT8 *nestr_str_end(NEINT8 *src, NEINT8 *outstr, const NEINT8 end);
NE_COMMON_API NEINT8 *nestr_nstr_end(NEINT8 *src, NEINT8 *outstr, const NEINT8 end, NEINT32 n) ;

/*�����ִ�Сд,�Ƚ��ַ���*/
NE_COMMON_API NEINT32 nestricmp(NEINT8 *src, NEINT8 *desc) ;

//��src�в���desc �����ִ�Сд
NE_COMMON_API NEINT8 *nestristr(NEINT8 *src, NEINT8 *desc);

//��src��ָ�ķ�����ǰ�����ַ�ch,
//����ҵ�endλ�û�û���ҵ��򷵻�null
NE_COMMON_API NEINT8 *nestr_reverse_chr(NEINT8 *src, NEINT8 ch, NEINT8 *end) ;
#endif 
