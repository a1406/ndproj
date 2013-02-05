#pragma warning(disable: 4819)

#ifndef _NE_APP_H_
#define _NE_APP_H_

#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_crypt/ne_crypt.h"
#include "ne_srvcore/ne_srvlib.h"

#include "ne_app/ne_appdef.h"
#include "ne_app/app_msgid.h"


struct srv_config
{
	NEINT32 port ;
	NEINT32 max_connect ;
	NEINT8 listen_name[32] ;
};

#define NE_CUR_VERSION 0x1		


CPPAPI nechar_t *srv_getconfig() ;

CPPAPI NEINT32 read_config(NEINT8 *file, struct srv_config *readcfg) ;

CPPAPI NEINT32 init_server_app(struct srv_config *run_config) ;

CPPAPI void destroy_rsa_key();

CPPAPI NE_RSA_CONTEX  *get_rsa_contex();

CPPAPI NEINT8 *get_pubkey_digest() ;	

CPPAPI NEINT32 start_server(NEINT32 argc, NEINT8 *argv[]);

CPPAPI NEINT32 end_server(NEINT32 force);

CPPAPI NEINT32 wait_services() ;


CPPAPI ne_handle get_listen_handle() ;


CPPAPI NEINT32 create_rsa_key() ;

CPPAPI ne_thsrvid_t get_timer_thid() ;

/* set player size*/
CPPAPI void set_session_size(size_t size) ;
CPPAPI struct cm_manager * get_cm_manager() ;

static __INLINE__ void ne_pause()
{
	getch() ;
	neprintf("press ANY key to continue\n") ;
}

MSG_ENTRY_DECLARE(srv_echo_handler) ;
//extern NEINT32 srv_echo_handler(ne_handle session_handle , ne_usermsgbuf_t *msg, ne_handle h_listen) ;
MSG_ENTRY_DECLARE(srv_broadcast_handler) ;
//extern NEINT32 srv_broadcast_handler(ne_handle session_handle , ne_usermsgbuf_t *msg, ne_handle h_listen) ;

//CPPAPI NEINT32 srv_echo_handler(ne_handle session_handle , ne_usermsgbuf_t *msg, ne_handle h_listen) ;

//CPPAPI NEINT32 srv_broadcast_handler(ne_handle session_handle , ne_usermsgbuf_t *msg, ne_handle h_listen) ;

CPPAPI void install_start_session(ne_handle listen_handle);

#ifndef WIN32
CPPAPI NEINT32 installed_sig_handle(void);

CPPAPI NEINT32 wait_signal_entry();

CPPAPI NEINT32 block_signal(void) ;

NEINT8 *get_signal_desc(NEINT32 signo);
CPPAPI NEINT32 unblock_signal(void) ;
CPPAPI NEINT32 ignore_all_signal(void) ;
#endif //WIN32


#endif
