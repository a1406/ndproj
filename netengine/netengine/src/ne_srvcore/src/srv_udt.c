#include "ne_common/ne_common.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_net/ne_netlib.h"
#include "ne_srvcore/ne_udtsrv.h"

static NEINT32 udt_data_in(ne_udt_node *conn_socket,size_t ) ;

NEINT32 udt_listen_callback(struct listen_contex *listen_info) ;

static ne_thsrvid_t _update_srv ;

/*打开udt网络*/
NEINT32 _open_udt(NEINT32 port, struct listen_contex *li_contex)
{
	if(-1==udt_open_srvex(port, &li_contex->udt) ) {
		return -1 ;
	}

	udt_set_notify_entry(&li_contex->udt, udt_data_in) ;

	return 0 ;
}

//根据时间驱动udt网络的线程函数
NEINT32 update_udt_entry(struct listen_contex *listen_info)
{
	netime_t tm_start ;
	NEINT32 ret  ;
	ne_handle context = ne_thsrv_gethandle(0) ;
	ne_udtsrv *root = &listen_info->udt ;
	
	struct ne_thsrv_createinfo subth_info = {
		SUBSRV_RUNMOD_STARTUP,	//srv_entry run module (ref e_subsrv_runmod)
		(ne_threadsrv_entry)udt_listen_callback,			//service main entry function address
		listen_info,		//param of srv_entry 
		NULL ,					//handle received message from other thread!
		NULL,					//clean up when server is terminal
		_NET("update_connect"),			//service name
		listen_info		//user data
	};
	if(!context){
		ne_logerror("can not get listen service context") ;
		return -1 ;
	}
	neprintf("start udt listen!\n") ;

	listen_info->sub_id = ne_thsrv_createex(&subth_info,NET_PRIORITY_HIGHT,0) ;
	if(!listen_info->sub_id )
		return -1;

	ret = 0 ;
	tm_start = ne_time() ;
	while (ne_thsrv_msghandler(context)){//处理线程消息
		
#if 0
		doneIO(root,100) ;
#else
		NEINT32 nsleep = 0 ;
//		update_all_socket(root) ;
		if(0==read_udt_handler(root, 100)){
			tm_start = ne_time() ;
			continue;
		}
		tm_start = ne_time() - tm_start;
		
		nsleep = (NEINT32)(LISTEN_INTERVAL - tm_start );
	//	if(nsleep > 10)
	//		ne_sleep(nsleep);
	//	else 
			ne_threadsched() ;

		tm_start = ne_time() ;
#endif 
	}
	return 0 ;
}

NEINT32 udt_listen_callback(struct listen_contex *listen_info)
{
	netime_t tm_start ;
	NEINT32 ret  ;

	ne_handle context = ne_thsrv_gethandle(0) ;
	ne_udtsrv *root = &listen_info->udt ;

	if(!context){
		ne_logerror("can not get listen service context") ;
		return -1 ;
	}

	ret = 0 ;
	tm_start = ne_time() ;
	while (ne_thsrv_msghandler(context)){//处理线程消息		
		/* 需要处理这两个函数之间的互斥问题(listen和update),
		 * 所以测试的时候 注释掉read_udt_handler这个函数.
		 * 主要时为了提高效率而已
		 */
		//read_udt_handler(root, 10);	
		update_all_socket(root) ;	
		ne_sleep(100);
	}

	if(_update_srv){
		ne_thsrv_end(_update_srv) ;
		ne_thsrv_wait(_update_srv) ;
	}
	//ne_thsrv_check_exit()
	//udt_close_srv(root,1) ;
	return 0 ;
}

NEINT32 udt_datagram(ne_udt_node *conn_socket, void *data, size_t len, void *param) 
{
	
	ne_packhdr_t *packet = data;
	if(len >0 && len == (size_t)ne_pack_len(packet)) {
		return srv_stream_data_entry(conn_socket, packet, conn_socket->srv_root) ;
		//return stream_data_entry(socket,packet,param) ;
	}
	return -1;
}


NEINT32 udt_data_in(ne_udt_node *conn_socket, size_t data_len )
{
	if(data_len)
		return parse_udt_stream(conn_socket,srv_stream_data_entry,conn_socket->srv_root) ;
	else {
		// connect has been closed
		return 0 ;
	}
}

void udt_clientmap_init(struct ne_udtcli_map *node, ne_handle h_listen)
{
	ne_udtnode_init(&node->connect_node);

//	node->status = 1 ;
//	node->status = ECS_NONE ;
//	node->extern_send_entry = NULL;
	INIT_LIST_HEAD(&(node->map_list)) ;

	node->connect_node.length = sizeof(struct ne_udtcli_map) ;
	node->connect_node.is_datagram = 0 ;
	node->connect_node.datagram_entry = udt_datagram ;
	node->connect_node.callback_param = NULL ;
	node->connect_node.close_entry =(ne_close_callback ) udt_close ;
//	node->connect_node.write_entry = udt_connector_send ;
}

