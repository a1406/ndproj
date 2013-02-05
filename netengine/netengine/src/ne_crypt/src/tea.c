#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "ne_common/ne_os.h"
#include "ne_crypt/ne_crypt.h"

#define ROUNDS 16
#define DELTA 0x9e3779b9 /* sqr(5)-1 * 2^31 */
const static TEAint32 tea_limit = (ROUNDS * DELTA);

/**********************************************************
   Input values: 	k[4]	128-bit key
			v[2]    64-bit plaintext block
   Output values:	v[2]    64-bit ciphertext block 
 **********************************************************/
void tean(TEAint32 *k, TEAint32 *v, NEINT32 N) 
{
	register TEAint32 y=v[0], z=v[1];
	register TEAint32 sum=0;
	if(N>0) { 
		/* ENCRYPT */
		while(sum != tea_limit) {
			y+=((z<<4)^(z>>5)) + (z^sum) + k[sum&3];
			sum+=DELTA;
			z+=((y<<4)^(y>>5)) + (y^sum) + k[(sum>>11)&3];
		}
	} 
	else { 
		/* DECRYPT */
		sum = tea_limit;
		while(sum) {
			z-=((y<<4)^(y>>5)) + (y^sum) + k[(sum>>11)&3];
			sum-=DELTA;
			y-=((z<<4)^(z>>5)) + (z^sum) + k[sum&3];
		}
	}
	v[0]=y; v[1]=z;
}


/*generate tea crypt key */
static void tea_init_rnd()
{
	static rnd_inited = 0 ;
	
	
	//NEINT32 time();
	NEINT16 seed[3];
	NEINT32 _getpid( void );
	
	if(1==rnd_inited){
		return ;
	}
	else {
		rnd_inited = 1 ;
	}
	seed[0] = time((time_t *)0) & 0xFFFF;
#ifdef WIN32
	seed[1] = _getpid() & 0xFFFF;
	seed[2] = (time((time_t *)0) >> 16) & 0xFFFF;
	{
		NEUINT32 *nseed = (NEUINT32 *) &seed[0] ;
		srand(*nseed) ;
	}
#else
	seed[1] = getpid() & 0xFFFF;
	seed[2] = (time((time_t *)0) >> 16) & 0xFFFF;
	(void)seed48( seed );
#endif
}

#ifdef WIN32
#include <windows.h>
NEINT32 tea_key(tea_k *k)
{
	TEAint32 *np = (TEAint32*)k ;
	LARGE_INTEGER curtim ;
	
	tea_init_rnd() ;
	
	QueryPerformanceCounter(&curtim) ;
	np[0] = rand() ^ (NEINT32)curtim.QuadPart ;
	
	QueryPerformanceCounter(&curtim) ;
	np[2] = rand() & (NEINT32)curtim.QuadPart;
	
	np[1] = rand()* (NEINT32)curtim.QuadPart ;
	
	QueryPerformanceCounter(&curtim) ;
	np[3] = rand() ^ (NEINT32)(~curtim.QuadPart) ;
	return 0 ;
}
#else
#include <sys/time.h>
NEINT32 tea_key(tea_k *k)
{
	TEAint32 *np = (TEAint32 *)k ;
	struct timeval tv ;
	
	tea_init_rnd() ;
	
	gettimeofday(&tv, NULL);
	np[0] = lrand48() ^ tv.tv_usec ;
	
	gettimeofday(&tv, NULL);
	np[2] = lrand48() & tv.tv_usec ;
	
	np[1] = (lrand48() * tv.tv_sec) | tv.tv_usec;
	
	gettimeofday(&tv, NULL);
	np[3] = (lrand48() * tv.tv_usec) & tv.tv_sec ;
	return 0 ;
}
#endif 

