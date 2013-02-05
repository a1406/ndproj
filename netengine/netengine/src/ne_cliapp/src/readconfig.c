#include "ne_common/ne_common.h"
#include "ne_common/ne_xml.h"
#include "ne_cliapp/ne_cliapp.h"
#define T_ERROR(msg) do {				\
	ne_msgbox("test error [%s]\n", msg) ;	\
	return -1 ;							\
} while(0)

NEINT32 read_config(NEINT8 *file, struct connect_config *readcfg)
{
	NEINT32 ret;
	NEINT8 *value ;
	nexml  *xml_node, *xml_read ;
	nexml_root xmlroot;
	
	ne_assert(file) ;
	ret = nexml_load(file, &xmlroot) ;
	if(0!=ret) {
		T_ERROR("load xml from file") ;
	}
	xml_node = nexml_getnode(&xmlroot,"connector") ;
	if(!xml_node) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read server_config error") ;
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
	xml_read = nexml_refsub(xml_node,"remote_port") ;
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
	
	//read connect	protocol
	xml_read = nexml_refsub(xml_node,"connect_protocol") ;
	if(!xml_read) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config error") ;
	}
	value = nexml_getval(xml_read) ;
	if(!value){
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config value error") ;
	}
	nestrncpy(readcfg->protocol_name,value, sizeof(readcfg->protocol_name)) ;

	//read listen mod
	xml_read = nexml_refsub(xml_node,"host") ;
	if(!xml_read) {		
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config error") ;
	}
	value = nexml_getval(xml_read) ;
	if(!value){
		nexml_destroy(&xmlroot);
		T_ERROR("read run_config value error") ;
	}
	nestrncpy(readcfg->host,value, sizeof(readcfg->host)) ;

	nexml_destroy(&xmlroot);
	return 0;

}
