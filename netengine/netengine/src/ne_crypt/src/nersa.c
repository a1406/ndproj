//#include "ne_common/ne_common.h"
#include "ne_common/ne_os.h"
#include "ne_crypt/rsah/global.h"
#include "ne_crypt/rsah/rsaref.h"
#include "ne_crypt/rsah/rsa.h"
#include "ne_crypt/ne_crypt.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#pragma warning(push)
#pragma warning(disable : 4305)
NEINT32 RSAinit_random(R_RANDOM_STRUCT *rStruct)
{
	NEINT32 ret,i, num ;
	NEINT8 *p_randblock, *paddr  ;
	static NEINT8 _randbuf[] = {	0x12, 0xf3 , 0x44, 0x78, 0xad,0x9f,0xf1,0x01,0xb3,0x32,0x5a,0x67,0x8c,0x2f,0xb8,0xde} ;
	if(R_RandomInit(rStruct)){
		return -1;
	}

	R_GetRandomBytesNeeded(&ret, rStruct) ;

	p_randblock = malloc(ret) ;
	if(!p_randblock)
		return 1 ;

	num = ret/sizeof(_randbuf) ;
	paddr = p_randblock ;
	for(i=0; i<num; i++){
		memcpy(paddr,_randbuf,sizeof(_randbuf) ) ;
		paddr += sizeof(_randbuf) ;
	}

	R_RandomUpdate(rStruct, p_randblock, ret) ;
	
	free(p_randblock);
	return 0;
}
#pragma warning(pop)

void RSAdestroy_random(R_RANDOM_STRUCT *rStruct)
{
	R_RandomFinal(rStruct) ;
}


NEINT32 ne_RSAInit(RSA_HANDLE r_contex)
{	
	NEINT32 ret ;
	R_RSA_PROTO_KEY protoKey;                            /* RSA prototype key */
	
	if(RSAinit_random(&r_contex->randomStruct)) {
		return -1 ;
	}
	
	r_contex->publicKey.bits = 512 ; r_contex->privateKey.bits = 512 ; protoKey.bits = 512;
	ret = R_GeneratePEMKeys (&r_contex->publicKey, &r_contex->privateKey, &protoKey, &r_contex->randomStruct) ;
	if(ret ) {
		return -1 ;
	}

	memset(&protoKey,0, sizeof(protoKey)) ;
	return 0 ;
}

void ne_RSAdestroy(RSA_HANDLE h_rsa)
{
	NE_RSA_CONTEX *r_contex = (NE_RSA_CONTEX *)h_rsa ;
	R_RandomFinal(&r_contex->randomStruct) ;
	
}

NEINT32 ne_RSAPublicEncrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa)
{
	return RSAPublicEncrypt(outbuf, outlen, inbuf, inlen, &(h_rsa->publicKey) , &(h_rsa->randomStruct)) ;
}

NEINT32 ne_RSAPrivateEncrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa)
{
	return RSAPrivateEncrypt(outbuf, outlen, inbuf, inlen, &h_rsa->privateKey) ;
}

NEINT32 ne_RSAPublicDecrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa)
{
	return RSAPublicDecrypt(outbuf, outlen, inbuf, inlen, &h_rsa->publicKey) ;
}

NEINT32 ne_RSAPrivateDecrypt(NEINT8 *outbuf, NEINT32 *outlen, NEINT8 *inbuf, NEINT32 inlen,RSA_HANDLE h_rsa)
{
	return RSAPrivateDecrypt(outbuf, outlen, inbuf, inlen, &h_rsa->privateKey ) ;
}

