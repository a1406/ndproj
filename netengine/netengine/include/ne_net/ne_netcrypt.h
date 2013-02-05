#ifndef _NE_NETCRPYT_ 
#define _NE_NETCRPYT_

#include "ne_common/ne_common.h"

#define CRYPT_KEK_SIZE 16

/*define nd net crypt function and struct*/
typedef struct _crypt_key
{
	NEINT32 size ;
	NEUINT8 key[CRYPT_KEK_SIZE] ;
}ne_cryptkey;

static __INLINE__ void init_crypt_key(ne_cryptkey *key)
{
	key->size = 0;
}

static __INLINE__ NEINT32 is_valid_crypt(ne_cryptkey *key)
{
	return (key->size > 0) ;
}
/* ����/���ܺ���
 * input : @data �����ܵ�����
 *			@len ���������ݵĳ���
 *			@key������Կ
 * output : @data ���ܺ������
 * ����ʱ������ݳ��Ȳ������ܵ�Ҫ��,���ڱ����ܵ����ݺ��油��ո�,���һ���ַ���¼������ݵĳ���
 * ������ݸպ�,����Ҫ����κζ���.
 * return value :on error return 0, else return data length of encrypted
 * ע��:���ܺ����ݲ���Ƚ���ǰ��
 */
typedef NEINT32 (*ne_netcrypt)(NEUINT8 *data, NEINT32 len, void *key) ;

/*���ü��ܺ����ͼ��ܵ�Ԫ����*/
NE_NET_API void ne_net_set_crypt(ne_netcrypt encrypt_func, ne_netcrypt decrypt_func,NEINT32 crypt_unit) ;

#endif
