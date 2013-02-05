#ifndef _NE_CLIAPP_H_
#define _NE_CLIAPP_H_

#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_crypt/ne_crypt.h"
#include "ne_app/app_msgid.h"

//#ifdef __cplusplus
//#include "ne_appcpp/ne_object.h"
//#include "ne_appcpp/ne_msgpacket.h"
//#include "ne_appcpp/ne_connector.h"
//#else 
//#endif 

struct connect_config
{
	NEINT32 port ;
	NEINT8 protocol_name[64] ;
	NEINT8 host[256] ;
};

CPPAPI NEINT32 read_config(NEINT8 *file, struct connect_config *readcfg) ;

CPPAPI ne_handle get_connect_handle();

CPPAPI NEINT32 ne_cliapp_init(NEINT32 argc, NEINT8 *argv[]);

CPPAPI NEINT32 ne_cliapp_end(NEINT32 force);
CPPAPI NEINT32 start_encrypt(ne_handle connect_handle);

CPPAPI NEINT32 end_session(ne_handle connect_handle) ;

CPPAPI ne_handle create_connector() ;
CPPAPI void destroy_connect(ne_handle h_connect) ;
CPPAPI void install_msg(ne_handle connector) ;
CPPAPI struct connect_config * get_config_info() ;
#endif
