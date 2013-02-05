//#include "ne_common/ne_common.h"
#include "ne_common/ne_os.h"
#include "ne_crypt/ne_crypt.h"

#include <stdlib.h>
#include <stdio.h>

/*
#if defined _DEBUG || defined DEBUG
#pragma comment(lib, "ne_common_dbg.lib")
#else 
#pragma comment(lib, "ne_common.lib")

#endif
*/

/* stuff src 
 * input :  src stuffed data 
 			dataln current data length
 			stufflen target data len 
 * return : data start address 
 */
#define STUFF_ELE	(NEINT8)0x20
NEINT8 *crypt_stuff(NEINT8 *src, NEINT32 datalen, NEINT32 stufflen )
{
	NEINT32 i ;
	NEINT8 *p = src ;
	if( stufflen <1 || stufflen <= datalen){
		return 0 ;
	}
	src += datalen ;
	stufflen -= datalen ;
	//memset(src, STUFF_ELE , stufflen - datalen ) ;	//stuff STUFF_ELE to buffer between src+datalen and src+datalen
	//p[stufflen-1] = (NEINT8)(stufflen - datalen) ;	//recrod stuff length 
	
	for(i=0; i<stufflen -1; i++){
		*src = STUFF_ELE ;src++ ;
	}
	*src = stufflen ;	//recrod stuff length 
	return p ;
}


/* 加密/解密函数
 * input : @data 被加密的数据
 *			@len 被加密数据的长度
 *			@key加密密钥
 * output : @data 加密后的数据
 * 加密时如果数据长度不够加密的要求,则在被加密的数据后面补充空格,最后一个字符记录填充数据的长度
 * 如果数据刚好,则不需要填充任何东西.
 * return value :on error return 0, else return data length of encrypted
 * 注意:解密后数据不会比解密前长
 */
NEINT32 ne_TEAencrypt(NEUINT8 *data, NEINT32 data_len, tea_k *key) 
{
	NEINT32 vlen = sizeof(tea_v) ;			/*加密单元的长度*/
	NEINT32 stuff_len ,i,n,new_len;
	
	tea_v *v = (tea_v *)data;

	stuff_len = data_len % vlen ;
	n = data_len / vlen ;
	
	if(stuff_len){
		++n ;
		new_len = n * vlen;
		crypt_stuff(data, data_len, new_len ) ;
	}
	else {
		new_len = data_len ;
	}
	for(i=0; i<n; i++){
		tea_enc(key, v) ;
		++v ;
	}
	return new_len ;
}

NEINT32 ne_TEAdecrypt(NEUINT8 *data, NEINT32 data_len, tea_k *key) 
{
	NEINT32 vlen = sizeof(tea_v) ;			/*加密单元的长度*/
	NEINT32 i,n;
	
	tea_v *v = (tea_v *)data;
	
#if defined _DEBUG || defined DEBUG
	NEINT32 stuff_len = data_len % vlen ;
	if(stuff_len){
		//ne_assert(0);		
		return -1 ;
	}
#endif

	n = data_len / vlen ;
	
	for(i=0; i<n; i++){
		tea_dec(key, v) ;
		++v ;
	}
	
	return data_len ;
}

/* convert crypt string to output string 
 * add by duan !
 */
NEINT8* MD5ToString(NEUINT8 src[16], NEUINT8 desc[33])
{
	NEUINT32 i ;
	NEINT8 *p = desc ;
	for(i=0; i<16; i++)
	{
		sprintf(p, "%02x",src[i]) ;
		p += 2 ;
	}
	desc[32] = 0 ;
	return desc ;
}


/*加密可打印的字符(\0的字符串)*/
NEINT8 *MD5CryptStr16(NEINT8 *input, NEINT8 output[16]) 
{
	MD5_CTX context;
	NEUINT32 len = strlen (input);
	
	MD5Init (&context);
	MD5Update (&context, input, len);
	MD5Final (output, &context);
	
	return output ;
}

/* 输入字符是二进制字符
 * @inlen input length
 * @input data address of input
 * @output buffer address NEINT8[16]
 */
NEINT8 *MD5Crypt16(NEINT8 *input, NEINT32 inlen, NEINT8 output[16])
{
	MD5_CTX context;
	
	MD5Init (&context);
	MD5Update (&context, input, inlen);
	MD5Final (output, &context);
	
	return output ;
}

NEINT8 *MD5CryptToStr32(NEINT8 *input, NEINT32 inlen, NEINT8 output[33])
{
	NEINT8 tmp[16] ;
	MD5_CTX context;
	
	MD5Init (&context);
	MD5Update (&context, input, inlen);
	MD5Final (tmp, &context);
	
	MD5ToString(tmp, output);
	return output ;
}

NEINT32 MD5cmp(NEINT8 src[16], NEINT8 desc[16])
{
	NEINT32 ret = 0 ;
	UINT4 *s4 =(UINT4 *) src ;
	UINT4 *d4 =(UINT4 *) desc ;

	ret = s4[0] - d4[0] ;
	if(ret)
		return ret ;
	ret = s4[1] - d4[1] ;
	if(ret)
		return ret ;
	ret = s4[2] - d4[2] ;
	if(ret)
		return ret ;
	ret = s4[3] - d4[3] ;
	return ret ;
}
