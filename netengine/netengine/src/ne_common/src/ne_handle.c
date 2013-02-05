#include "ne_common/ne_common.h"
#include "ne_common/ne_handle.h"
#include "ne_common/ne_alloc.h"

#define ALLOC_MARK 0x7f3e 

static LIST_HEAD(__reg_list) ;

static LIST_HEAD(__alloced_obj_list) ;

//��¼����ע��Ķ�������
struct register_object_info
{
	struct list_head list ;
	struct ne_handle_reginfo obj ;

};

//�Ѿ���̬����Ķ����б�
struct alloced_objects 
{
	struct list_head list ;
	struct tag_ne_handle alloced_obj ;
};
static ne_mutex __alloced_lock ;
static NEINT32 __inited ;

/*����һ������ʵ��*/
ne_handle _object_create(NEINT8 *name) 
{
	struct register_object_info *objlist ;
	struct ne_handle_reginfo *reginfo = 0  ;
	struct list_head *pos = __reg_list.next ;
	if(0 == __inited) {
		ne_mutex_init(&__alloced_lock) ;
		__inited = 1 ;
	}
	while (pos != &__reg_list) {
		objlist = list_entry(pos, struct register_object_info, list) ;		
		if(0 == strcmp(objlist->obj.name,name)) {
			reginfo = &(objlist->obj) ;
			break ;
		}
		pos = pos->next ;
	}

	if(reginfo) {
		size_t alloc_size = reginfo->object_size + sizeof(struct list_head) ;
		struct alloced_objects *ao =(struct alloced_objects *) malloc(alloc_size) ;
		if(ao) {
			
			INIT_LIST_HEAD(&ao->list) ;			
			ne_mutex_lock(&__alloced_lock) ;
			list_add_tail(&ao->list, &__alloced_obj_list) ;
			ne_mutex_unlock(&__alloced_lock) ;

			memset(&ao->alloced_obj, 0, reginfo->object_size) ;
			if(reginfo->init_entry) {
				reginfo->init_entry(&ao->alloced_obj) ;
			}
			ao->alloced_obj.size = reginfo->object_size ;
			ao->alloced_obj.close_entry = reginfo->close_entry ;
			return &(ao->alloced_obj) ;
		}
	}
	return NULL ;
}


/* �ر�һ�����*/
NEINT32 _object_destroy(ne_handle handle, NEINT32 flag) 
{
	NEINT32 ret = -1;
	NEINT32 is_free = 0 ;
	struct list_head  *pos ;
	struct list_head  *free_addr =(struct list_head  *) handle ;
	--free_addr ;

	//try to free !
	ne_mutex_lock(&__alloced_lock) ;
	pos = __alloced_obj_list.next ;
	while(pos != &__alloced_obj_list) {
		if(pos == free_addr) {
			is_free = 1 ;
			list_del(pos) ;
			break ;
		}
		pos = pos->next ; 

	}
	ne_mutex_unlock(&__alloced_lock) ;
	
	if(handle->close_entry) {		
		ret = handle->close_entry(handle, flag) ;		
	}
	if(is_free) {
		free(free_addr) ;
	}
	return ret ;
}

//ע��һ���������,������windows�������͵�ע��
NEINT32 ne_object_register(struct ne_handle_reginfo *reginfo) 
{
	struct register_object_info *pobj;
	ne_assert(reginfo) ;
	
	pobj = (struct register_object_info *)malloc(sizeof(*pobj)) ;
	if(!pobj) {
		return -1 ;
	}

	INIT_LIST_HEAD(&pobj->list) ;
	memcpy(&pobj->obj, reginfo, sizeof(*reginfo)) ;
	pobj->obj.name[OBJECT_NAME_SIZE-1] = 0 ;

	list_add_tail(&pobj->list, &__reg_list) ;
	return 0 ;
}

NEINT32 destroy_object_register_manager(void) 
{
	struct register_object_info *objlist ;
	struct ne_handle_reginfo *reginfo = 0  ;
	struct list_head *pos = __reg_list.next ;

	while (pos != &__reg_list) {
		objlist = list_entry(pos, struct register_object_info, list) ;
		pos = pos->next ;
		list_del_init(&objlist->list);
		free(objlist) ;
	}
	return 0 ;
}

NEINT8 *ne_object_errordesc(ne_handle h) 
{
	NEINT8 *perr[] = {
		"NEERR_SUCCESS|��ȷ" ,
		"NEERR_TIMEOUT|��ʱ" ,
		"NEERR_NOSOURCE|û���㹻��Դ" ,
		"NEERR_OPENFILE|���ܴ��ļ�" ,
		"NEERR_BADTHREAD|���ܴ��߳�" ,
		"NEERR_LIMITED|��Դ��������" ,
		"NEERR_USER|�����û����ݳ���(��Ϣ�ص���������-1" ,
		"NEERR_INVALID_INPUT | ��Ч������(DATA IS TO BIG OR ZERO" ,
		"NEERR_IO | IO bad SYSTEM IO BAD" ,
		"NEERR_WUOLD_BLOCK | ��Ҫ����	",
		"NEERR_CLOSED | socket closed by peer" ,
		"NEERR_BADPACKET  | �����������ݴ���(too NEINT32 or NEINT16)" ,
		"NEERR_BADSOCKET | ��Ч��socket" ,
		
		"NEERR_UNKNOW	| unknowwing error"
	} ;

	if(h->myerrno <= NEERR_UNKNOW)
		return perr[h->myerrno] ;
	else 
		return "unknowwing" ;

}
