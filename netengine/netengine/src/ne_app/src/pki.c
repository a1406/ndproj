#include "ne_app.h"
#include "ne_crypt/ne_crypt.h"

static NEINT8 __rsa_digest[16] ;
NE_RSA_CONTEX  __rsa_contex ;

NEINT32 create_rsa_key()
{
	if(ne_RSAInit(&__rsa_contex))
		return -1 ;

	MD5Crypt16((NEINT8*)&(__rsa_contex.publicKey), sizeof(R_RSA_PUBLIC_KEY ) , __rsa_digest);
	//MD5ToString(__rsa_digest, md5text);
	//NETRACF("RSA text=[%s]",md5text);
	return 0 ;
}

void destroy_rsa_key()
{
	ne_RSAdestroy(&__rsa_contex);
}

NE_RSA_CONTEX  *get_rsa_contex()
{
	return &__rsa_contex;
}

NEINT8 *get_pubkey_digest()
{
	return __rsa_digest ;
}
