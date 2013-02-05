#ifndef _NE_CMALLOCATOR_H_
#define _NE_CMALLOCATOR_H_

#include "ne_net/ne_netlib.h"

#define MAX_CLIENTS				(1024*10)
#define MAX_AFFIX_DATALEN		(1024*1024)

NE_SRV_API NEINT32 ne_cm_allocator_destroy(ne_handle alloctor, NEINT32 flag);

NE_SRV_API void* ne_cm_node_alloc(ne_handle alloctor) ;

NE_SRV_API void ne_cm_node_free(void* cli_handle,ne_handle alloctor ) ;

NE_SRV_API NEINT32 ne_cm_allocator_capacity(ne_handle alloctor)  ;			//��������ɶ��ٸ�clientmap

NE_SRV_API NEINT32 ne_cm_allocator_freenum(ne_handle a) ;					//���нڵ����

NE_SRV_API ne_handle ne_cm_allocator_create(NEINT32 client_num, size_t client_size) ;

#endif
