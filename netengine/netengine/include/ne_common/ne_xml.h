#ifndef _NE_XML_H_
#define _NE_XML_H_

#define MAX_XMLNAME_SIZE 64

typedef void (*xml_errlog) (NEINT8 *errdesc) ;		//������������
//typedef struct tagxml nexml ;
//xml �ڵ�,���Ե�������
typedef struct tagxml 
{
	struct list_head  lst_self ;
	NEINT8 name[MAX_XMLNAME_SIZE];
	NEINT8 *value ;
	size_t val_size ;

	NEINT32 attr_num ;
	struct list_head lst_attr; 

	NEINT32 sub_num ;
	struct list_head lst_sub ;
	
} nexml;

//xml�ĸ��ڵ�,������nexml,
//��Ҫ�����ļ����غͱ���ʱʹ��
typedef struct tagxmlroot
{
	NEINT32 num ;
	struct list_head lst_xml ;	
}nexml_root;

//xml���Խڵ�
/*struct nexml_attr {
	NEINT8 *name ;
	NEINT8 *value ;
	struct list_head lst; 
};
*/
struct nexml_attr {
	NEUINT16 name_size ;
	NEUINT16 value_size ;
	struct list_head lst; 
};

static __INLINE__ void init_xml_attr(struct nexml_attr *attr) 
{
	attr->name_size = 0 ;
	attr->value_size = 0 ;
	//attr->name = 0 ;
	//attr->value = 0 ;
	INIT_LIST_HEAD(&attr->lst);
}

static __INLINE__ void init_xml_node(nexml *xmlnode)
{
	xmlnode->name[0] = 0 ;
	xmlnode->value = 0 ;
	xmlnode->val_size = 0 ;

	xmlnode->attr_num = 0 ;
	xmlnode->sub_num = 0;
	INIT_LIST_HEAD(&xmlnode->lst_self);
	INIT_LIST_HEAD(&xmlnode->lst_attr);
	INIT_LIST_HEAD(&xmlnode->lst_sub);
}

static __INLINE__ void nexml_initroot(nexml_root *root)
{
	root->num = 0 ;
	INIT_LIST_HEAD(&root->lst_xml) ;
}

//����xml��������ʱ��log����,����Ĭ�Ϻ���
NE_COMMON_API xml_errlog ne_setxml_log(xml_errlog logfunc) ;
//���ļ��м���һ��xml�б�
NE_COMMON_API NEINT32 nexml_load(const NEINT8 *file,nexml_root *xmlroot) ;

//���ҵ�һ��xml�ӵ�
NE_COMMON_API nexml *nexml_getnode(nexml_root *xmlroot, NEINT8 *name) ;
NE_COMMON_API nexml *nexml_getnodei(nexml_root *xmlroot, NEINT32 index) ;

NE_COMMON_API NEINT32 nexml_merge(nexml_root *host, nexml_root *merged) ;
//���һ��xml�ڵ�
NE_COMMON_API nexml *nexml_addnode(nexml_root *xmlroot, NEINT8 *name,NEINT8 *value) ;

//ɾ��һ��XML�ڵ�
NE_COMMON_API NEINT32 nexml_delnode(nexml_root *xmlroot, NEINT8 *name) ;
NE_COMMON_API NEINT32 nexml_delnodei(nexml_root *xmlroot, NEINT32 index) ;

//�������xml����
NE_COMMON_API void nexml_destroy(nexml_root *xmlroot) ;

//��xml���浽�ļ���
NE_COMMON_API NEINT32 nexml_save(nexml_root *xmlroot, const NEINT8 *file) ;
//����һ���ӽڵ�
NE_COMMON_API nexml *nexml_refsub(nexml *root, const NEINT8 *name) ;

//����һ���ӽڵ�
NE_COMMON_API nexml *nexml_refsub(nexml *root, const NEINT8 *name) ;

//ͨ���±�����һ���ӽڵ�
NE_COMMON_API nexml *nexml_refsubi(nexml *root, NEINT32 index) ;

//�õ�xml��ֵ
NE_COMMON_API NEINT8 *nexml_getval(nexml *node);
//�õ����Խڵ�
NE_COMMON_API struct nexml_attr *nexml_getattrib(nexml *node , NEINT8 *name);
NE_COMMON_API struct nexml_attr  *nexml_getattribi(nexml *node, NEINT32 index);

//////////////////////////////////////////////////////////////////////////
//��xml����һ������,
NE_COMMON_API struct nexml_attr  *nexml_addattrib(nexml *parent, NEINT8 *name, NEINT8 *value) ;

//����xml����ֵ
NE_COMMON_API NEINT32 nexml_setattrval(nexml *parent, NEINT8 *name, NEINT8 *value) ;
NE_COMMON_API NEINT32 nexml_setattrvali(nexml *parent, NEINT32 index, NEINT8 *value) ;

//��xml����һ���ӽڵ���Ҫ�����½ڵ�����ֺ�ֵ,�����½ڵ��ַ
NE_COMMON_API nexml *nexml_addsubnode(nexml *parent, NEINT8 *name, NEINT8 *value) ;
//����XML��ֵ
NE_COMMON_API NEINT32 nexml_setval(nexml *node , NEINT8 *val) ;
//ɾ��һ�����Խڵ�
NE_COMMON_API NEINT32 nexml_delattrib(nexml *parent, NEINT8 *name) ;
NE_COMMON_API NEINT32 nexml_delattribi(nexml *parent, NEINT32 index) ;
//ɾ��һ���ӽڵ�
NE_COMMON_API NEINT32 nexml_delsubnode(nexml *parent, NEINT8 *name) ;
NE_COMMON_API NEINT32 nexml_delsubnodei(nexml *parent, NEINT32 index) ;
//////////////////////////////////////////////////////////////////////////

static __INLINE__ NEINT8 *nexml_getname(nexml *node)
{
	return node->name ;
}
static __INLINE__ NEINT32 nexml_getattr_num(nexml *node)
{
	return node->attr_num ;
}
static __INLINE__ NEINT32 nexml_num(nexml_root *root)
{
	return root->num ;
}
static __INLINE__ NEINT32 nexml_getsub_num(nexml *node)
{
	return node->sub_num ;
}
static __INLINE__ NEINT8 *nexml_getattr_name(nexml *node, NEINT32 index )
{
	struct nexml_attr* attr = nexml_getattribi(node, index);
	if(attr) {
		return (NEINT8*) (attr+1) ;
	}
	return NULL ;
}

static __INLINE__ NEINT8 *nexml_getattr_val(nexml *node, NEINT8 *name )
{
	struct nexml_attr  * attr = nexml_getattrib(node, name);
	if(attr) {
		return  ((NEINT8*) (attr+1) ) + attr->name_size ;
	}
	return NULL;
}

static __INLINE__ NEINT8 *nexml_getattr_vali(nexml *node, NEINT32 index )
{
	struct nexml_attr* attr = nexml_getattribi(node, index);
	if(attr) {
		return  ((NEINT8*) (attr+1) ) + attr->name_size ;
	}
	return NULL;
}
#endif //_NE_XML_H_
