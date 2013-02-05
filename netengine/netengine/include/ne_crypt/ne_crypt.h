/*
 * This module include MD5, tea and rsa.

  MD5ToString 把加密好的MD5转换成可打印的字符
 * NEINT8* MD5ToString(NEUINT8 src[16], NEUINT8 desc[33]);

  把字符串加密成MD5
 * NEINT8 *MD5CryptStr16(NEINT8 *input, NEINT8 output[16]) ;

  把2进制加密成MD5
 * NEINT8 *MD5Crypt16(NEINT8 *input, NEINT32 inlen, NEINT8 output[16]);

  产生一个tea的密钥
 * NEINT32 tea_key(tea_k *k);

  tea加密
 * void tea_enc(tea_k *k, tea_v *v);

  tea解密
 * void  tea_dec(tea_k *k, tea_v *v);

  rsa加密/解密
  返回0成功
	NEINT32 ne_RSAcreate(RSA_HANDLE); 产生一对加密解密密钥
 void ne_RSAdestroy(RSA_HANDLE *h_rsa);	//销毁加密解密密钥
 //加密或者解密函数,公开密钥加密的只能用私人密钥解密,反之亦然
 NEINT32 ne_RSAPublicEncrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);
 NEINT32 ne_RSAPrivateEncrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);
 NEINT32 ne_RSAPublicDecrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);
 NEINT32 ne_RSAPrivateEncrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);
  
*/

#ifndef _NE_CRYPT_
#define _NE_CRYPT_

#ifndef CPPAPI
	#ifdef __cplusplus
	#define CPPAPI extern "C" 
	#else 
	#define CPPAPI 
	#endif
#endif 

#if defined(WIN32) || defined(_WINDOWS) || defined(_WIN32)
	#if  defined(NE_CRYPT_EXPORTS) 
	# define NE_CRYPT_API 				CPPAPI __declspec(dllexport)
	#else
	# define NE_CRYPT_API 				CPPAPI __declspec(dllimport)
	#endif
#else 
	# define NE_CRYPT_API 				CPPAPI
#endif 

#include "ne_crypt/tea.h"

#include "ne_crypt/rsah/global.h"
#include "ne_crypt/rsah/rsaref.h"
#include "ne_crypt/rsah/rsa.h"


NE_CRYPT_API NEINT32 ne_TEAencrypt(NEUINT8 *data, NEINT32 data_len, tea_k *key) ;
NE_CRYPT_API NEINT32 ne_TEAdecrypt(NEUINT8 *data, NEINT32 data_len, tea_k *key) ;

/* mix-in data */
NE_CRYPT_API NEINT8 *crypt_stuff(NEINT8 *src, NEINT32 datalen, NEINT32 stufflen ) ;	/*stuff data to align*/


/* convert crypt string to output string 
 * add by duan !
 */
NE_CRYPT_API NEINT8* MD5ToString(NEUINT8 src[16], NEUINT8 desc[33]);

/*加密可打印的字符(\0的字符串)*/
NE_CRYPT_API NEINT8 *MD5CryptStr16(NEINT8 *input, NEINT8 output[16]) ;

/* 输入字符是二进制字符
 * @inlen input length
 * @input data address of input
 * @output buffer address NEINT8[16]
 */
NE_CRYPT_API NEINT8 *MD5Crypt16(NEINT8 *input, NEINT32 inlen, NEINT8 output[16]);
/* 计算md5,输出可打印的字符*/
NE_CRYPT_API NEINT8 *MD5CryptToStr32(NEINT8 *input, NEINT32 inlen, NEINT8 output[33]);
NE_CRYPT_API NEINT32 MD5cmp(NEINT8 src[16], NEINT8 desc[16]) ;

/* rsa crypt*/

typedef struct  {	
	R_RSA_PUBLIC_KEY publicKey;                          /* new RSA public key */
	R_RSA_PRIVATE_KEY privateKey;                       	/* new RSA private key */
	R_RANDOM_STRUCT randomStruct;                        /* random structure */
}NE_RSA_CONTEX ;
typedef NE_RSA_CONTEX *RSA_HANDLE ;

NE_CRYPT_API NEINT32 RSAinit_random(R_RANDOM_STRUCT *rStruct);
NE_CRYPT_API void RSAdestroy_random(R_RANDOM_STRUCT *rStruct);
NE_CRYPT_API NEINT32 ne_RSAInit(RSA_HANDLE h_rsa);
NE_CRYPT_API void ne_RSAdestroy(RSA_HANDLE h_rsa);
/* rsa de/encrypt*/
NE_CRYPT_API NEINT32 ne_RSAPublicEncrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);
NE_CRYPT_API NEINT32 ne_RSAPrivateEncrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);
NE_CRYPT_API NEINT32 ne_RSAPublicDecrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);
NE_CRYPT_API NEINT32 ne_RSAPrivateDecrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa);

#endif	//_NE_CRYPT_
