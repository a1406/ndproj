#define NE_IMPLEMENT_HANDLE
typedef struct netui_info *ne_handle;

#include "ne_common/ne_common.h"
#include "ne_net/ne_netlib.h"
#include "ne_common/ne_alloc.h"
//#include "ne_net/ne_srv.h"
//消息入口函数节点
struct msg_entry_node
{
	NEINT32					level ;	//权限等级
	ne_usermsg_func		entry ;	//入口函数
};

/*主消息结果*/
struct sub_msgentry 
{
	struct msg_entry_node   msg_buf[SUB_MSG_NUM] ;
};

/* 消息处理结构入口节点 */
struct msgentry_root
{
	NEINT32	main_num ;			//包含多少个消息类别
	NEINT32 msgid_base ;		//主消息号起始地址
	struct sub_msgentry sub_buf[0] ;
};


NEINT32 srv_default_handler(ne_handle session_handle , ne_usermsgbuf_t *msg)
{
	neprintf("received message maxid=%d minid=%d param=%d len=%d\n", 
		NE_USERMSG_MAXID(msg) ,NE_USERMSG_MINID(msg) ,NE_USERMSG_PARAM(msg) , NE_USERMSG_LEN(msg) ) ;
	//ne_sessionmsg_send(session_handle,msg) ;
	return 0;
}
NEINT32 connector_default_handler(ne_handle session_handle , ne_usermsgbuf_t *msg,ne_handle hlisten)
{
	neprintf("received message maxid=%d minid=%d param=%d len=%d\n", 
		NE_USERMSG_MAXID(msg) ,NE_USERMSG_MINID(msg) ,NE_USERMSG_PARAM(msg) , NE_USERMSG_LEN(msg) ) ;
	//ne_sessionmsg_send(session_handle,msg) ;
	return 0;
}

NEINT32 connector_default_handler1(NEUINT16 session_id , ne_usermsgbuf_t *msg,ne_handle hlisten)
{
	neprintf("received message maxid=%d minid=%d param=%d len=%d\n", 
		NE_USERMSG_MAXID(msg) ,NE_USERMSG_MINID(msg) ,NE_USERMSG_PARAM(msg) , NE_USERMSG_LEN(msg) ) ;
	//ne_sessionmsg_send(session_handle,msg) ;
	return 0;
}

static struct msgentry_root *create_msgroot(NEINT32 max_mainmsg, NEINT32 base) 
{
	size_t size ;
	struct msgentry_root * root ;

	size = sizeof(struct sub_msgentry ) * max_mainmsg + sizeof(struct msgentry_root) ;
	root = (struct msgentry_root *)malloc(size) ;
	if(root) {
		memset(root, 0, size) ;
		root->main_num = max_mainmsg ;	
		root->msgid_base = base ;
	}
	return root ;	
};

/* 为连接句柄创建消息入口表
 * @mainmsg_num 主消息的个数(有多数类消息
 * @base_msgid 主消息开始号
 * return value : 0 success on error return -1
 */
NEINT32 ne_msgtable_create(ne_netui_handle handle, NEINT32 mainmsg_num, NEINT32 base_msgid) 
{
	void **p = NULL ;
	ne_assert(handle) ;
	
	if(mainmsg_num>MAX_MAIN_NUM || mainmsg_num <= 0) {
		return -1;
	}
	
	if(handle->nodetype==NE_TCP){
		p = & (((struct ne_tcp_node*)handle)->user_msg_entry ) ; 
	}
	else {
		p = & (((ne_udt_node*)handle)->user_msg_entry ) ; 
	}

	*p = create_msgroot(mainmsg_num, base_msgid)  ;
	if(*p) {
		return 0 ;
	}
	else {
		return -1;
	}
}

void ne_msgtable_destroy(ne_netui_handle handle) 
{	
	ne_assert(handle) ;
		
	if(handle->nodetype==NE_TCP){
		if(((struct ne_tcp_node*)handle)->user_msg_entry) {
			free(((struct ne_tcp_node*)handle)->user_msg_entry) ;
			((struct ne_tcp_node*)handle)->user_msg_entry = 0 ;
		}
	}
	else {
		if(((ne_udt_node*)handle)->user_msg_entry) {
			free(((ne_udt_node*)handle)->user_msg_entry) ;
			((ne_udt_node*)handle)->user_msg_entry = 0 ;
		}
	}

}

/*在handle连接句柄上安装消息处理函数*/
NEINT32 ne_msgentry_install(ne_netui_handle handle, ne_usermsg_func func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) 
{
	struct msgentry_root *root_entry = NULL;
	ne_assert(handle) ;
		
	if(handle->nodetype==NE_TCP){
		root_entry = (struct msgentry_root *) (((struct ne_tcp_node*)handle)->user_msg_entry ) ; 
	}
	else {
		root_entry = (struct msgentry_root *) (((ne_udt_node*)handle)->user_msg_entry ) ; 
	}
	if(root_entry) {
		nemsgid_t main_index =(nemsgid_t) (maxid - root_entry->msgid_base );
		if(main_index >= root_entry->main_num )
			return -1 ;
		if(minid >= SUB_MSG_NUM )
			return -1 ;

		root_entry->sub_buf[main_index].msg_buf[minid].entry = func ;
		root_entry->sub_buf[main_index].msg_buf[minid].level = level ;
		return 0 ;
	}
	return -1;
}

/* 为listen句柄创建消息入口表
 * @mainmsg_num 主消息的个数(有多数类消息
 * @base_msgid 主消息开始号
 * return value : 0 success on error return -1
 */
NEINT32 ne_srv_msgtable_create(ne_handle listen_handle, NEINT32 mainmsg_num, NEINT32 base_msgid) 
{
	struct ne_srv_node* srv_node = (struct ne_srv_node* )listen_handle ;

	ne_assert(srv_node) ;
	
	if(mainmsg_num>MAX_MAIN_NUM || mainmsg_num <= 0) {
		return -1;
	}

	srv_node->user_msg_entry = (void*)create_msgroot(mainmsg_num, base_msgid)  ;
	if(srv_node->user_msg_entry) {
		return 0 ;
	}
	else {
		return -1;
	}
}


void ne_srv_msgtable_destroy(ne_handle listen_handle) 
{
	struct ne_srv_node* srv_node = (struct ne_srv_node* )listen_handle ;
	
	if(srv_node->user_msg_entry) {
		free(srv_node->user_msg_entry) ;
		srv_node->user_msg_entry = 0 ;
		
	}
	
}

/*在listen句柄上安装消息处理函数*/
NEINT32 ne_srv_msgentry_install(ne_handle listen_handle, ne_usermsg_func func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) 
{	
	struct msgentry_root *root_entry ;
	struct ne_srv_node* srv_node = (struct ne_srv_node* )listen_handle ;
	
	ne_assert(srv_node) ;

	root_entry = (struct msgentry_root *) (srv_node->user_msg_entry ) ; 
		
	if(root_entry) {
		nemsgid_t main_index = maxid - root_entry->msgid_base ;
		if(main_index >= root_entry->main_num )
			return -1 ;
		if(minid >= SUB_MSG_NUM )
			return -1 ;

		root_entry->sub_buf[main_index].msg_buf[minid].entry = func ;
		root_entry->sub_buf[main_index].msg_buf[minid].level = level ;
		return 0 ;
	}
	return -1 ;
}

NEINT32 ne_translate_message(ne_netui_handle connect_handle, ne_packhdr_t *msg ) 
{
	struct msgentry_root *root_entry = NULL;
	ne_usermsg_func  func = NULL ;
	ne_usermsghdr_t *usermsg = (ne_usermsghdr_t *) msg ;

	ne_assert(msg) ;
	ne_assert(connect_handle) ;

		
	if(connect_handle->nodetype==NE_TCP){
		root_entry = (struct msgentry_root *) (((struct ne_tcp_node*)connect_handle)->user_msg_entry ) ; 
	}
	else {
		root_entry = (struct msgentry_root *) (((ne_udt_node*)connect_handle)->user_msg_entry ) ; 
	}
	if(root_entry) {
		nemsgid_t main_index , minid;
		NEINT32 level = (NEINT32) ne_connect_level_get(connect_handle)
		ne_netmsg_ntoh(usermsg) ;
		main_index = usermsg->maxid - root_entry->msgid_base;
		minid = usermsg->minid ;
		if(main_index >= root_entry->main_num )
			return -1 ;
		if(minid >= SUB_MSG_NUM)
			return -1 ;
		//连接器不需要权限
		//if(level < root_entry->sub_buf[main_index].msg_buf[minid].level)
		//	return -1 ;
		func = root_entry->sub_buf[main_index].msg_buf[minid].entry ;
		if(func)
			return func(connect_handle,(ne_usermsgbuf_t*)usermsg,NULL) ;
	}

	connector_default_handler(connect_handle,(ne_usermsgbuf_t*)usermsg,NULL) ;
	return 0 ;
}


NEINT32 ne_srv_translate_message(ne_handle listen_handle, ne_netui_handle connect_handle, ne_packhdr_t *msg ) 
{
	struct ne_srv_node* srv_node = (struct ne_srv_node* )listen_handle ;
	struct msgentry_root *root_entry = NULL;
	ne_usermsg_func  func = NULL ;
	ne_usermsghdr_t *usermsg = (ne_usermsghdr_t *) msg ;

	ne_assert(msg) ;
	ne_assert(connect_handle) ;
	
	ne_assert(srv_node) ;

	root_entry = (struct msgentry_root *) (srv_node->user_msg_entry ) ; 
		
	if(root_entry) {
		nemsgid_t  main_index , minid;
		NEINT32 level = (NEINT32) ne_connect_level_get(connect_handle)
		ne_netmsg_ntoh(usermsg) ;
		main_index = usermsg->maxid - root_entry->msgid_base;
		minid = usermsg->minid ;
		if(main_index >= root_entry->main_num )
			return -1 ;
		if(minid >= SUB_MSG_NUM )
			return -1 ;
		if(level < root_entry->sub_buf[main_index].msg_buf[minid].level)
			return -1 ;
		func = root_entry->sub_buf[main_index].msg_buf[minid].entry ;
		if(func)
			return func(connect_handle,(ne_usermsgbuf_t*)usermsg,listen_handle) ;
	}

	connector_default_handler(connect_handle,(ne_usermsgbuf_t*)usermsg,listen_handle) ;
	return 0 ;
}

NEINT32 ne_srv_translate_message1(ne_handle listen_handle, NEUINT16 session_id, ne_packhdr_t *msg ) 
{
	struct ne_srv_node* srv_node = (struct ne_srv_node* )listen_handle ;
	struct msgentry_root *root_entry = NULL;
	ne_usermsg_func1  func = NULL ;
	ne_usermsghdr_t *usermsg = (ne_usermsghdr_t *) msg ;

	ne_assert(msg) ;
	ne_assert(session_id) ;
	
	ne_assert(srv_node) ;

	root_entry = (struct msgentry_root *) (srv_node->user_msg_entry ) ; 
		
	if(root_entry) {
		nemsgid_t  main_index , minid;
		ne_netmsg_ntoh(usermsg) ;
		main_index = usermsg->maxid - root_entry->msgid_base;
		minid = usermsg->minid ;
		if(main_index >= root_entry->main_num )
			return -1 ;
		if(minid >= SUB_MSG_NUM )
			return -1 ;
	//	if(level < root_entry->sub_buf[main_index].msg_buf[minid].level)
	//		return -1 ;
		func =(ne_usermsg_func1) (root_entry->sub_buf[main_index].msg_buf[minid].entry );
		if(func)
			return func(session_id,(ne_usermsgbuf_t*)usermsg,listen_handle) ;
	}

	connector_default_handler1(session_id,(ne_usermsgbuf_t*)usermsg,listen_handle) ;
	return 0 ;
}
//权限等级
NEUINT32 ne_connect_level_get(ne_netui_handle handle) 
{
	return handle->level ;
}

//权限等级
void ne_connect_level_set(ne_netui_handle handle,NEUINT32 level) 
{
	handle->level = level ;
}
#undef NE_IMPLEMENT_HANDLE
