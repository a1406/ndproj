#include "ne_common/ne_str.h"


/* 去掉字符串开头部分无用的字符（不可打印的字符）*/
NEINT8 *nestr_first_valid(const NEINT8 *src) 
{
	NEUINT8 *tmp = (NEUINT8 *)src ;
	while(*tmp <=(NEUINT8) _NE_SPACE) {
		if(*tmp==0) {
			return NULL;
		}
		tmp++ ;
	}
	return (NEINT8*)tmp ;
		
}

/* 检测字符是否是有效的数字*/
NEINT32 nestr_is_numerals(const NEINT8 *src)
{
	NEINT32 dot = 0 ;
	NEINT32 ret = 0 ;
	src = nestr_first_valid(src) ;
	
	if(*src==_MINUS)
		++src ;
	else if(IS_NUMERALS(*src)){
		++src ; ret = 1 ;
	}
	else 
		return 0 ;
	while(*src) {
		if(IS_NUMERALS(*src) ){
			ret = 1 ;
		}
		else if(_DOT==*src) {
			dot++ ;
			if(dot > 1)
				return 0 ;
		}
		else
			return 0 ;
		++src ;
	}
	return ret ;	
}

//检测字符串是否自然数
NEINT32 nestr_is_naturalnumber(const NEINT8 *src)
{	
	NEINT32 ret = 1 ;
	src = nestr_first_valid(src) ;
	
	if(*src=='+')
		++src ;
	else if(IS_NUMERALS(*src)){
		++src ; ret = 1 ;
	}
	else 
		return 0 ;
	while(*src) {
		if(!IS_NUMERALS(*src) ){
			ret = 0 ;
			break ;
		}
		++src ;
	}
	return ret ;	
}


/* 读取有效数字,*isok == 0出错*/
NEINT8 *nestr_read_numerals(const NEINT8 *src, NEINT8 *desc, NEINT32 *isok) 
{
	NEINT32 dot = 0 ;
	*isok = 0 ;
	src = nestr_first_valid(src) ;
	
	if(*src==_MINUS||*src=='+')
		*desc++ =*src++ ;
	else if(*src==_DOT){
		*desc++ = '0' ;
		*desc++ =*src++ ;
		dot++ ;
	}
	else if(IS_NUMERALS(*src)){
		*desc++ =*src++ ;
		*isok = 1 ;
	}
	else {
		*isok = 0 ;
		return (NEINT8*)src ;
	}
	while(*src) {
		if(IS_NUMERALS(*src) ){
			*isok = 1 ;
		}
		else if(_DOT==*src) {
			dot++ ;
			if(dot > 1) {
				*isok = 0 ;
				break ;
			}
		}
		else{
			break ;
		}
		*desc++ =*src++ ;
	}
	*desc = 0;
	return (NEINT8*)src ;	
}


//分解一个单词,单词只能是数字,字母和下划线
NEINT8 *nestr_parse_word(NEINT8 *src, NEINT8 *outstr)
{
	register NEUINT8 a ;
	while(*src) {
		a = (NEUINT8)*src ;
		if(IS_NUMERALS(a) || IS_BIG_LATIN(a) || IS_LITTLE_LATIN(a) || a=='_' ){			
			*outstr++ = *src++ ;
		}
		
		else if(a>(NEUINT8)0x80){		//chinese		
			*outstr++ = *src++ ;
			*outstr++ = *src++ ;
		}
		else{
			break ;
		}
	}
	*outstr = 0 ;
	return *src?src:NULL ;
}


NEINT8 *nestr_parse_word_n(NEINT8 *src, NEINT8 *outstr, NEINT32 n)
{
	register NEUINT8 a ;
	while(*src && n-- > 0) {
		a = (NEUINT8)*src ;
		if(IS_NUMERALS(a) || IS_BIG_LATIN(a) || IS_LITTLE_LATIN(a) || a=='_' ){			
			*outstr++ = *src++ ;
		}
		
		else if(a>(NEUINT8)0x80){		//chinese		
			*outstr++ = *src++ ;
			*outstr++ = *src++ ;
			--n ;
		}
		else{
			break ;
		}
	}
	*outstr = 0 ;
	return *src?src:NULL ;
}

/*读取一个字符串，知道遇到一个制定的结束字符为止*/
NEINT8 *nestr_str_end(NEINT8 *src, NEINT8 *outstr, const NEINT8 end)
{
	while(*src) {
		if(end== *src ){
			break ;
		}
		if((NEUINT8)*src>(NEUINT8)0x80)		//chinese		
			*outstr++ = *src++ ;
		*outstr++ = *src++ ;
	}
	*outstr = 0 ;
	return *src?src:NULL ;
}
NEINT8 *nestr_nstr_end(NEINT8 *src, NEINT8 *outstr, const NEINT8 end, NEINT32 n)
{
	while(*src && n>0) {
		if(end== *src ){
			break ;
		}
		if((NEUINT8)*src>(NEUINT8)0x80){	//chinese		
			*outstr++ = *src++ ;
			--n ;
		}
		*outstr++ = *src++ ;
		--n ;
	}
	*outstr = 0 ;
	return *src?src:NULL ;
}

/*不区分大小写,比较字符串*/
NEINT32 nestricmp(NEINT8 *src, NEINT8 *desc) 
{
	NEINT32 ret ;
	do {
		ret = *src - *desc ;
		if(ret){
			NEINT8 a ;
			if(IS_BIG_LATIN(*src)) {
				a = BIG_2_LITTLE(*src) ;
			}
			else if(IS_LITTLE_LATIN(*src)) {
				a = LITTLE_2_BIG(*src) ;
			}
			else {
				return ret ;
			}
			if(a != *desc)
				return ret ;
		}
		desc++ ;
	}while (*src++) ;
	return ret ;
}

//在src中查找desc 不区分大小写
NEINT8 *nestristr(NEINT8 *src, NEINT8 *desc)
{
	NEINT32 ret=0 ;
	while(*src) {
		NEINT8 *tmp = src;
		NEINT8 *aid = desc;
		
		while(*aid) {
			ret = *tmp - *aid ;
			if(ret){
				NEINT8 a ;
				if(IS_BIG_LATIN(*tmp)) {
					a = BIG_2_LITTLE(*tmp) ;
				}
				else if(IS_LITTLE_LATIN(*tmp)) {
					a = LITTLE_2_BIG(*tmp) ;
				}
				else {
					break ;
				}
				
				if(a != *aid)
					break ;
				else 
					ret = 0 ;
			}
			aid++ ;
			tmp++ ;
		}

		if(0==ret)
			return src ;
		++src;
	}
	return NULL ;
}

//从src所指的方向向前查找字符ch,
//如果找到end位置还没有找到则返回null
NEINT8 *nestr_reverse_chr(NEINT8 *src, NEINT8 ch, NEINT8 *end)
{
	while(end <= src) {
		if(ch== *src ){
			return src ;
		}
		--src;
	}
	
	return NULL;
}
