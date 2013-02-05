#include "ne_common/ne_common.h"
#include "ne_common/ne_xml.h"
#include "ne_app/ne_app.h"
#define T_ERROR(msg) do {				\
	ne_msgbox("test error [%s]\n", msg) ;	\
	return -1 ;							\
} while(0)

NEINT32 read_config(NEINT8 *file, struct srv_config *readcfg)
{
	NEINT32 ret;
	NEINT8 *value ;
	nexml *xml_sub, *xml_node, *xml_read ;
	nexml_root xmlroot;
	
	ne_assert(file) ;
	ret = nexml_load(file, &xmlroot) ;
	if(0!=ret) {
		T_ERROR("load xml from file") ;
	}
	xml_sub = nexml_getnode(&xmlroot,"server_config") ;
	if(!xml_sub) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read server_config error") ;
	}
	
	xml_node = nexml_refsub(xml_sub,"run_config") ;
	if(!xml_node) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config error") ;
	}

	//read logfile
	xml_read = nexml_refsub(xml_node,"logfile") ;
	if(!xml_read) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config error") ;
	}
	value = nexml_getval(xml_read) ;
	if(!value){
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config value error") ;
	}
	set_log_file(value) ;

	//read port
	xml_read = nexml_refsub(xml_node,"port") ;
	if(!xml_read) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config error") ;
	}
	value = nexml_getval(xml_read) ;
	if(!value){
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config value error") ;
	}
	readcfg->port = atoi(value) ;
	
	//read max connect	
	xml_read = nexml_refsub(xml_node,"max_connect") ;
	if(!xml_read) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config error") ;
	}
	value = nexml_getval(xml_read) ;
	if(!value){
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config value error") ;
	}
	readcfg->max_connect = atoi(value) ;

	//read listen mod
	xml_read = nexml_refsub(xml_node,"listen_mod") ;
	if(!xml_read) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config error") ;
	}
	value = nexml_getval(xml_read) ;
	if(!value){
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config value error") ;
	}
	nestrncpy(readcfg->listen_name,value, sizeof(readcfg->listen_name)) ;

	nexml_destroy(&xmlroot);
	//readcfg->config_file = file ;
	return 0;

}
