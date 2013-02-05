#if defined(__LINUX__)
#include "ne_common/ne_os.h"
#include "ne_common/list.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_srvcore/ne_listensrv.h"
#include "ne_common/ne_common.h"
#include "ne_common/ne_alloc.h"
#include "ne_common/ne_bintree.h"
#include <sys/epoll.h>

//static LIST_HEAD(__wait_close) ;
//static ne_mutex __lock_wait = PTHREAD_MUTEX_INITIALIZER ;

static NEINT32 create_sub_update_thread(struct listen_contex *listen_info) ;
/*epoll ʹ�õĹرպ���,ֻ�Ǽ򵥵ķŵ����رն�����*/
static NEINT32 epoll_close(struct ne_client_map *client_map , NEINT32 force)
{
	struct listen_contex *lc;
	ne_assert(client_map) ;
	ne_assert(((struct ne_tcp_node*)(client_map))->fd ) ;

	lc = (struct listen_contex *) (client_map->connect_node.srv_root);
	ne_assert(lc) ;
	TCPNODE_SET_CLOSED(client_map) ;

	INIT_LIST_HEAD(&client_map->map_list) ;
	
	ne_mutex_lock(&lc->wait_free.lock) ;
		list_add(&client_map->map_list, &lc->wait_free.list) ;
	ne_mutex_unlock(&lc->wait_free.lock) ;
	
	return 0 ;
}

NEINT32 _linux_listen_srv(struct listen_contex *listen_info)
{
	NEINT32 ret ;
	swap_list_t *swap ;
	nesocket_t listen_fd = get_listen_fd(listen_info);
	NEINT32 nfds ,epoll_fd, event_num,i,current_fds;
	NEINT32 max_client ;
	struct epoll_event ev_listen, *ev_buf ;
	struct ne_client_map *client_map; 
	struct ne_srv_node *root = &(listen_info->tcp);
	ne_handle thread_handle ;

	swap = &listen_info->wait_free ;
	ne_swap_list_init(swap) ;
	
	if(-1==create_sub_update_thread(listen_info)) {
		ne_swap_list_destroy(swap) ;
		ne_logerror("create epoll sub thread error\n") ;
		return -1 ;
	}

	ne_assert(root);
	NETRACF("new linux epoll IO Module!\n") ;

	thread_handle = ne_thsrv_gethandle(0) ;
	ne_assert(thread_handle) ;

	max_client = ne_cm_allocator_capacity(root->cm_alloctor) ;
	
	event_num = max_client + 1;

	ev_buf = (struct epoll_event*)malloc(event_num * sizeof(*ev_buf)) ;
	if(!ev_buf){
		ne_logerror("alloc memory ") ;
		return -1 ;
	}
	
	if(-1==ne_socket_nonblock(listen_fd,1)) {
		ne_logerror("set socket nonblock ") ;
		goto LISTEN_EXIT ;
	}	
	
	epoll_fd = epoll_create(event_num) ;
	if(epoll_fd < 0){
		ne_logerror("create epoll error :%s" AND ne_last_error()) ;
		goto LISTEN_EXIT ;
	}
		
	ev_listen.data.u32 = (__uint32_t)0 ;
	ev_listen.events = EPOLLIN | EPOLLET;

	if(-1==epoll_ctl(epoll_fd,EPOLL_CTL_ADD, listen_fd,&ev_listen) ) {
		ne_logerror("epoll ctrl error :%s" AND ne_last_error()) ;
		//return -1 ;
		goto LISTEN_EXIT ;
	}
	
	current_fds = 1 ;
	while (!ne_thsrv_isexit(thread_handle)){
		nfds = epoll_wait(epoll_fd,ev_buf,event_num,1000) ;
		//if(-1==nfds){
		//	ne_showerror() ;
		//	break ;
		//}
		for (i=0; i<nfds; i++){
			if(ev_buf[i].data.u32==(__uint32_t)0) {
				for (;;) {
					client_map = accetp_client_connect(listen_info) ;
					if(client_map && client_map->connect_node.session_id) {
						ne_socket_nonblock(client_map->connect_node.fd, 1) ;

						ev_listen.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
						ev_listen.data.u32 =(__uint32_t)(client_map->connect_node.session_id);
						if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_map->connect_node.fd, &ev_listen) < 0) {
							tcp_client_close(client_map,1) ;
						}
						client_map->connect_node.close_entry =(ne_close_callback ) epoll_close ;
						++current_fds ;
					}
					else
						break;
				}
			}
			else {
				
				NEINT32 fd_tmp ;
				NEUINT16 session_id =(NEUINT16) (ev_buf[i].data.u32);
				//ne_assert(ev_buf[i].events == EPOLLIN) ;
				client_map = root->conn_manager.lock(&root->conn_manager,session_id) ;
				//ne_assert(client_map) ;
				if(!client_map)
					continue ;
				//handle received message and send result!
				fd_tmp = client_map->connect_node.fd ;	
			
				/*if(tryto_close_tcpsession((ne_session_handle)client_map, listen_info->operate_timeout ) ) {
					//continue ;
				}
				else*/ if(ev_buf[i].events & EPOLLIN){
					ret = ne_do_netmsg(client_map,root) ;					
					if(ret == 0){
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_tmp, &ev_listen);
						tcp_client_close(client_map,1) ;
						--current_fds ;
					}
				}				
				else /*if(ev_buf[i].events & (EPOLLERR + EPOLLHUP) )*/ {
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_tmp, &ev_listen);
					tcp_client_close(client_map,1) ;
					--current_fds ;
				}
				root->conn_manager.unlock(&root->conn_manager,session_id) ;
			}		//end if
		}			//end for
		//try to close connect			
		if(swap->list.next != &swap->list) {
			NEUINT16 session_id ;
			nesocket_t fd_tmp ;
			LIST_HEAD(header) ;
			struct list_head *pos ;
			
			if(0==ne_mutex_trylock(&swap->lock) ) {
				list_add(&header, &swap->list) ;
				list_del_init(&swap->list) ;
				ne_mutex_unlock(&swap->lock) ;
				
				list_for_each(pos, &header) {
					client_map = list_entry(pos, struct ne_client_map, map_list) ;
					session_id = client_map->connect_node.session_id;
					ne_assert(session_id) ;

					fd_tmp = client_map->connect_node.fd ;

					client_map = root->conn_manager.lock(&root->conn_manager,session_id) ;
					if(!client_map)
						continue ;
					if(fd_tmp) {
						if(0==epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_tmp, &ev_listen) ) {
							--current_fds ;
						}
					}
					tcp_client_close(client_map,0) ;
					root->conn_manager.unlock(&root->conn_manager,session_id) ;
				}		//end list_for_each				
			}			//end lock
		}				//end if
	}					//end while
	
LISTEN_EXIT:
	close(epoll_fd) ;
	ne_swap_list_destroy(swap) ;
	free(ev_buf) ;
	return 0 ;
}
NEINT32 epoll_update_entry(struct listen_contex *listen_info)
{
	NEINT32 ret  ;
	NEINT32 i, num ,sleep = 0 ;

	struct cm_manager *pmanger  ;
	struct ne_client_map  *client;
	
	NEUINT16 session_id = 0;
	cmlist_iterator_t cm_iterator ;
	
	pmanger = ne_listensrv_get_cmmamager((ne_listen_handle)listen_info) ;	

	for(client = pmanger->lock_first(pmanger,&cm_iterator) ; client;	client = pmanger->lock_next(pmanger,&cm_iterator) ) {
	
		if(tryto_close_tcpsession((ne_session_handle )client, listen_info->operate_timeout ) ) {
			continue ;
		}
		ne_tcpnode_flush_sendbuf(&(client->connect_node)) ; 
	}
	ne_sleep(1000) ;
	return 0 ;
}

NEINT32 create_sub_update_thread(struct listen_contex *listen_info)
{
	struct ne_thsrv_createinfo subth_info = {
		SUBSRV_RUNMOD_LOOP,	//srv_entry run module (ref e_subsrv_runmod)
		(ne_threadsrv_entry)epoll_update_entry,			//service main entry function address
		listen_info,		//param of srv_entry 
		NULL ,					//handle received message from other thread!
		NULL,					//clean up when server is terminal
		_NET("epoll_update"),			//service name
		listen_info		//user data
	};
	

	listen_info->sub_id = ne_thsrv_createex(&subth_info,NET_PRIORITY_LOW,0) ;
	if(!listen_info->sub_id )
		return -1;
	return 0 ;
}
#endif			//linux
