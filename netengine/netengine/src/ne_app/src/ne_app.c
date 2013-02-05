#include "ne_common/ne_os.h"
#include "ne_crypt/ne_crypt.h"
#include "ne_net/ne_netlib.h"
#include "ne_app.h"


#ifdef WIN32

#ifdef NE_DEBUG
#pragma comment(lib,"ne_srvcore_dbg.lib")
#pragma comment(lib,"ne_net_dbg.lib")
#pragma comment(lib,"ne_crypt_dbg.lib")
#pragma comment(lib,"ne_common_dbg.lib")
#else 
#pragma comment(lib,"ne_srvcore.lib")
#pragma comment(lib,"ne_net.lib")
#pragma comment(lib,"ne_crypt.lib")
#pragma comment(lib,"ne_common.lib")
#endif 

#pragma comment(lib,"Ws2_32.lib")
#include <conio.h>

#else 
	#ifndef DEBUG_WITH_GDB
		#define HANDLE_UNIX_SIGNAL 1        //handle unix signal
	#endif

#endif
#define MAX_CONNECTS  1100
extern void install_start_session() ;

extern NEINT32 create_rsa_key();

extern NEINT32 accept_entry(NE_SESSION_T nethandle, SOCKADDR_IN *addr, ne_handle h_listen) ;

extern void close_entry(NE_SESSION_T nethandle, ne_handle h_listen)  ;

extern NEINT32 pre_close_entry(NE_SESSION_T nethandle, ne_handle h_listen) ;


extern NEINT32 start_timer_srv(netime_t interval) ;
void install_message_entries(ne_handle listen_handle) ;

static ne_handle listen_handle ;

static size_t session_size = 0 ;

nechar_t *__config_file ;

NEINT32 io_mod = NE_LISTEN_UDT_STREAM;// NE_LISTEN_OS_EXT;// NE_LISTEN_COMMON;//NE_LISTEN_UDT_DATAGRAM;// ; //

nechar_t *srv_getconfig() 
{
	return __config_file ;
}

ne_handle get_listen_handle()
{
	return listen_handle ;
}

void set_session_size(size_t size)
{
	session_size = size ;
}

NEINT32 init_module()
{
	if(-1 == ne_common_init() ) {
		ne_logfatal("init common error!\n") ;
		return -1 ;
    }

	if(-1 == ne_net_init() ) {
		ne_logfatal("init net error!\n") ;
		return -1 ;
    }

	if(-1 == ne_srvcore_init() ) {
		ne_logfatal("init srvcore error!\n") ;
		return -1 ;
    }
    
	return 0 ;
}

void destroy_module() 
{
	ne_srvcore_destroy() ;
	ne_net_destroy() ;
	ne_common_release() ;
}

NEINT32 init_server_app(struct srv_config *run_config)
{
	NEINT32 ssize = 0;
	create_rsa_key() ;
	
	
	//open net server 
	listen_handle = ne_object_create(run_config->listen_name) ;
	if(!listen_handle) {
		ne_logfatal("create object!\n") ;
		return -1 ;
	}
	
#ifdef SINGLE_THREAD_MOD
	ne_listensrv_set_single_thread(listen_handle) ;
#else

	//set listen attribute
	if(session_size < sizeof(player_header_t))
		session_size = sizeof(player_header_t) ;
	
	ssize = session_size + ne_getclient_hdr_size(((struct listen_contex*)listen_handle)->io_mod) ;
#endif
	if(-1==ne_listensrv_session_info(listen_handle, run_config->max_connect, ssize) ) {
		ne_logfatal("create client map allocator!\n") ;
		return -1 ;
	}

	NE_SET_ONCONNECT_ENTRY(listen_handle,accept_entry,pre_close_entry,close_entry) ;
	//ne_listensrv_set_entry(listen_handle,accept_entry,pre_close_entry,close_entry) ;
	
	ne_srv_msgtable_create(listen_handle, MSG_CLASS_NUM, MAXID_BASE) ;
	
	install_start_session(listen_handle) ;
	
	NE_INSTALL_HANDLER(listen_handle,srv_echo_handler,MAXID_SYS,SYM_ECHO,EPL_CONNECT) ;

	NE_INSTALL_HANDLER(listen_handle,srv_broadcast_handler,MAXID_SYS,SYM_BROADCAST,EPL_CONNECT) ;

	//set crypt function
	ne_net_set_crypt((ne_netcrypt)ne_TEAencrypt, (ne_netcrypt)ne_TEAdecrypt, sizeof(tea_v)) ;
	
	if(-1==ne_listensrv_open(run_config->port,   listen_handle)  ) {
		ne_logfatal("open port!\n") ;
		return -1 ;
	}

#ifndef SINGLE_THREAD_MOD
	if(-1==start_timer_srv(5000) ) {
		ne_logfatal("start time server!\n") ;
		return -1 ;
	}
#endif
/**/		
	return 0;
}

//start server
NEINT32 start_server(NEINT32 argc, NEINT8 *argv[])
{
	NEINT32 i ;
	NEINT8 *config_file = NULL;
	struct srv_config readcfg = {0} ;

#ifndef WIN32
	//prctl(PR_SET_DUMPABLE, 1);
#ifdef  HANDLE_UNIX_SIGNAL
	block_signal() ;
#endif 
#endif	
	ne_arg(argc, argv);
	
	if(-1==init_module() ) 
		return -1 ;
	
	//get config file 	
	for (i=1; i<argc-1; i++){
		if(0 == nestrcmp(argv[i],"-f")) {
			config_file = argv[i+1] ;
			break ;
		}
	}

	if(!config_file) {
		neprintf("usage: -f config-file\n press ANY key to continue\n") ;
		getch() ;
		exit(1) ;
		//return -1 ;
	}
	__config_file = config_file ;
	if(-1==read_config(config_file, &readcfg) ) {
		neprintf("press ANY key to continue\n") ;
		getch() ;
		exit(1) ;
	}
	//end get config file

	if(-1==init_server_app(&readcfg) ) {
		return -1 ;
	}
	
	return 0;
}

NEINT32 end_server(NEINT32 force)
{
	/*ne_thsrvid_t timerid = get_timer_thid() ;
	if(timerid) {
		ne_thsrv_destroy(timerid,0) ;
	}*/
	ne_host_eixt() ;
	if(listen_handle) {
		ne_srv_msgtable_destroy(listen_handle) ;
	
		ne_object_destroy(listen_handle,0) ;
		listen_handle = 0 ;
	}
	ne_thsrv_release_all() ;
	destroy_rsa_key() ;
	destroy_module()  ;
	
	neprintf("end server\n") ;
	return 0;
}


NEINT32 wait_services()
{
#ifdef WIN32
	NEINT32 ch;
	while( !ne_host_check_exit() ){
		if(kbhit()) {
			ch = getch() ;
			if(NE_ESC==ch){
				printf_dbg("you are hit ESC, program eixt\n") ;
				break ;
			}
		}
		else {
			ne_sleep(1000) ;
		}
	}
#else
	
#ifdef     HANDLE_UNIX_SIGNAL
	
	while(!ne_host_check_exit() ){
		if(-1==wait_signal_entry() ) {
			printf_dbg("exit from wait signal!\n") ;
			break ;
		}
	}
	unblock_signal() ;
#else 
	NEINT32 ch;
#ifndef DEBUG_WITH_GDB
	installed_sig_handle();
#endif
	while( !ne_host_check_exit() ){
		if(kbhit()) {
			ch = getch() ;
			if(NE_ESC==ch){
				printf_dbg("you are hit ESC, program eixt\n") ;
				break ;
			}
		}
		else {
			ne_sleep(1000) ;
		}
	}
#endif
#endif
	return 0;
}
