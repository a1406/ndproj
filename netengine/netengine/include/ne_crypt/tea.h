#ifndef _MYTEA_H_
#define _MYTEA_H_

typedef NEUINT32 TEAint32;

typedef struct _tea_k{ TEAint32 k[4];} tea_k;		/* tea key*/
typedef struct _tea_v{ TEAint32 v[2];} tea_v;		/* tea value (en/decrypt code buf)*/

/* generate tea key*/
NE_CRYPT_API NEINT32 tea_key(tea_k *k);

/**********************************************************
   Input values: 	k[4]	128-bit key
			v[2]    64-bit plaintext block
   Output values:	v[2]    64-bit ciphertext block 
 **********************************************************/
NE_CRYPT_API void tean(TEAint32 *k, TEAint32 *v, NEINT32 N) ;

/* tea enctypt data 
 * input :  k 128bit encryptKEY 
 			v 64bit plaintext block
 * output : v ciphertext block 
 */
static __inline void tea_enc(tea_k *k, tea_v *v)
{
	tean((TEAint32 *)k, (TEAint32 *)v, 1) ;
}

/* tea dectypt data 
 * input :  k 128bit encryptKEY 
 			v 64bit plaintext block
 * output : v ciphertext block 
 */
static __inline void  tea_dec(tea_k *k, tea_v *v)
{
	tean((TEAint32 *)k, (TEAint32 *)v, -1) ;
}


#endif 
