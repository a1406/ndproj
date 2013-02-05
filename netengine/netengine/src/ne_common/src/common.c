#include "ne_common/ne_common.h"
#include "ne_common/ne_mempool.h"
NEINT8 *__g_process_name = NULL;			//½ø³ÌÃû×Ö

NEINT8 *ne_process_name() 
{
	static NEINT8 s_proname[128] = {0};
	
	if(!__g_process_name) {
		return "ne_engine" ;
	}
	else if(0==s_proname[0]) {
		NEINT32 len = strlen(__g_process_name) ;
		NEINT8 *p = __g_process_name + len;
		NEINT8 *desc = s_proname;
		while(len-- > 0){
			if(*p==0x2f || *p==0x5c) {   // '/' || '\'
				++p ;
				break ;
			}
			--p ;
		}
		len = 128 ;
		while(*p && len-- > 0) {
			if(*p== '.')
				break ;
			*desc++ = *p++ ;
		}
		*desc = 0 ;
	}
	return s_proname ;
}

NEINT32 ne_arg(NEINT32 argc, NEINT8 *argv[])
{
	__g_process_name = argv[0] ;
	return 0;
}

NEINT32 ne_common_init()
{
	NETRAC("common init\n") ;
	//ne_memory_init() ;
	if(-1==ne_mempool_root_init() )  {
		
		return -1 ;
	}
	ne_sourcelog_init() ;
	return 0 ;
}

void ne_common_release()
{
	destroy_object_register_manager() ;
	ne_sourcelog_dump();
	ne_mempool_root_release();
	//ne_memory_destroy() ;
}
