#ifndef _NE_STR_H_
#define _NE_STR_H_

#include "ne_common/ne_common.h"
 
#define _DOT  '.'
#define _MINUS '-' 
#define _NE_SPACE			0x20
#define _NE_TAB				0x09
#define _NE_QUOT			0x22		//双引号
#define _NE_SINGLE_QUOT		0x27		//单引号
#define IS_NUMERALS(a)		((a) >= '0' && (a) <='9')

#define IS_PRINTABLE(a)		((a) >= 0x20 && (a) < 0x7f)

#define IS_BIG_LATIN(a)		( (a) >= 'A' && (a)<='Z')
#define IS_LITTLE_LATIN(a)	( (a) >= 'a' && (a)<='z')
#define BIG_2_LITTLE(a)		(a)+0x20 
#define LITTLE_2_BIG(a)		(a)-0x20 

/* 去掉字符串开头部分无用的字符（不可打印的字符）*/
NE_COMMON_API NEINT8 *nestr_first_valid(const NEINT8 *src) ;

/* 检测字符是否是有效的数字*/
NE_COMMON_API NEINT32 nestr_is_numerals(const NEINT8 *src);

//检测字符串是否自然数
NE_COMMON_API NEINT32 nestr_is_naturalnumber(const NEINT8 *src);

NE_COMMON_API NEINT8 *nestr_read_numerals(const NEINT8 *src, NEINT8 *desc, NEINT32 *isok) ;

//分解一个单词,单词只能是数字,字母和下划线
NE_COMMON_API NEINT8 *nestr_parse_word(NEINT8 *src, NEINT8 *outstr);
NE_COMMON_API NEINT8 *nestr_parse_word_n(NEINT8 *src, NEINT8 *outstr, NEINT32 n) ;

/*读取一个字符串，知道遇到一个制定的结束字符为止*/
NE_COMMON_API NEINT8 *nestr_str_end(NEINT8 *src, NEINT8 *outstr, const NEINT8 end);
NE_COMMON_API NEINT8 *nestr_nstr_end(NEINT8 *src, NEINT8 *outstr, const NEINT8 end, NEINT32 n) ;

/*不区分大小写,比较字符串*/
NE_COMMON_API NEINT32 nestricmp(NEINT8 *src, NEINT8 *desc) ;

//在src中查找desc 不区分大小写
NE_COMMON_API NEINT8 *nestristr(NEINT8 *src, NEINT8 *desc);

//从src所指的方向向前查找字符ch,
//如果找到end位置还没有找到则返回null
NE_COMMON_API NEINT8 *nestr_reverse_chr(NEINT8 *src, NEINT8 ch, NEINT8 *end) ;
#endif 
