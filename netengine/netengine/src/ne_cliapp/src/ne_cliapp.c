//#ifdef NE_DEBUG
//#pragma comment(lib,"ne_common_dbg.lib")
//#pragma comment(lib,"ne_net_dbg.lib")
//#pragma comment(lib,"ne_crypt_dbg.lib")
//
//#else 
//#pragma comment(lib,"ne_common.lib")
//#pragma comment(lib,"ne_net.lib")
//#pragma comment(lib,"ne_crypt.lib")
//#endif 

#include "ne_cliapp/ne_cliapp.h"
#include "ne_crypt/ne_crypt.h"

//ne_handle __connect_handle ;
extern NE_RSA_CONTEX __rsa_contex;

static struct connect_config __conn_config;

struct connect_config * get_config_info()
{
	return & __conn_config ;
}

nechar_t *__cli_config_file ;

#define OUT_ERROR(msg) do {				\
	ne_msgbox("test error [%s]\n", msg) ;	\
	return -1 ;							\
} while(0)
/*
ne_handle get_connect_handle()
{
	return __connect_handle;
}
*/
nechar_t *cli_getconfig() 
{
	return __cli_config_file ;
}
NEINT32 ne_cliapp_init(NEINT32 argc, NEINT8 *argv[])
{
	NEINT32 i ;
	NEINT8 *config_file = NULL ;
	
	if (argv)
		ne_arg(argc, argv);
	ne_common_init() ;
	
	//get config file 	
	for (i=1; i<argc-1; i++){
		if(0 == nestrcmp(argv[i],"-f")) {
			config_file = argv[i+1] ;
			break ;
		}
	}

	if(!config_file) {
		goto noconfig;
		//ne_msgbox("usage: -f config-file", "error") ;
		//return -1 ;
	}
	__cli_config_file = config_file ;
	if(-1==read_config(config_file, &__conn_config) ) {
		ne_msgbox("READ config error!", "error") ;
		return -1 ;
		//neprintf("press ANY key to continue\n") ;
		//getch() ;
		//exit(1) ;
	}
	//end get config file
noconfig:	
	ne_net_init() ;
		
	RSAinit_random(&__rsa_contex.randomStruct);

	ne_net_set_crypt((ne_netcrypt)ne_TEAencrypt, (ne_netcrypt)ne_TEAdecrypt, sizeof(tea_v)) ;

	return 0 ;
//	__connect_handle = create_connector() ;
//	return __connect_handle? 0 : -1 ;
	
}

NEINT32 ne_cliapp_end(NEINT32 force)
{
	/*
	if(__connect_handle) {
		ne_msgtable_destroy(__connect_handle);
		//ne_connector_close(__connect_handle,force);
		ne_object_destroy(__connect_handle, force) ;
		__connect_handle = 0 ;
	}
	*/
	RSAdestroy_random(&__rsa_contex.randomStruct);

	ne_net_destroy() ;
	ne_common_release() ;
	return 0;
}

ne_handle create_connector()
{
	ne_handle handle_net ;
	//connect to host 
	handle_net = ne_object_create(__conn_config.protocol_name ) ;
	
	if(!handle_net){		
		ne_logerror("connect error :%s!" AND ne_last_error()) ;
		return 0;
	}

	//set message handle	
	ne_msgtable_create(handle_net, MSG_CLASS_NUM, MAXID_BASE) ;
	
	install_msg(handle_net);

	if(-1 == ne_connector_openex( handle_net,__conn_config.host, __conn_config.port ) ) {
		ne_logerror("connect error :%s!" AND ne_last_error()) ;
		return 0;
	}
	return handle_net ;
}
void destroy_connect(ne_handle h_connect)
{
	if(h_connect){
		ne_connector_close(h_connect, 0) ;
		ne_msgtable_destroy(h_connect);
		//ne_connector_close(__connect_handle,force);
		ne_object_destroy(h_connect, 0) ;		
	}
}
