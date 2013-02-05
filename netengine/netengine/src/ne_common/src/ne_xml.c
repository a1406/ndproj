#include "ne_common/ne_common.h"

#define MAX_FILE_SIZE 4*4096
#define XML_H_END   0x3e2f			// /> xml node end mark
#define XML_T_END   0x2f3c			// </ xml end node mark,高字节在高地址 /地址高于 >

nexml *parse_xmlbuf(NEINT8 *xmlbuf, NEINT32 size,NEINT8 **parse_end, NEINT8 **error_addr) ;
nexml *alloc_xml();
void  dealloc_xml(nexml *node );
struct nexml_attr *alloc_attrib_node(NEINT8 *name, NEINT8 *value);
void dealloc_attrib_node(struct nexml_attr *pnode);
NEINT32 xml_write(nexml *xmlnode, FILE *fp , NEINT32 deep) ;
nexml *_create_xmlnode(NEINT8 *name, NEINT8 *value)  ;

static void _errlog (NEINT8 *errdesc) ;
static void show_xmlerror(const NEINT8 *file, NEINT8 *error_addr, NEINT8 *xmlbuf, size_t size) ;

static xml_errlog __xml_logfunc = _errlog ;

NEINT32 nexml_load(const NEINT8 *file,nexml_root *xmlroot)
{
	NEINT32 data_len,buf_size ;
	FILE *fp;
	NEINT8 *text_buf = NULL , *parse_addr;

	fp = fopen(file, "r+b") ;
	if(!fp) {
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	buf_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(buf_size==0) {
		fclose(fp) ;
		return -1 ;
	}
	buf_size += 1 ;
	text_buf =(NEINT8*) malloc(buf_size) ;
	
	if(!text_buf){
		fclose(fp) ;
		return -1 ;
	}
	data_len = fread(text_buf,1, buf_size, fp) ;
	if(data_len==0 || data_len>= buf_size) {
		fclose(fp) ;
		free(text_buf) ;
		return -1 ;

	}
	text_buf[data_len] = 0 ;
	fclose(fp) ;
	
	//nexml_root *xmlroot;
	nexml_initroot(xmlroot) ;
	parse_addr = text_buf ;
	do {
		NEINT8 *error_addr = 0;
		nexml *xmlnode = parse_xmlbuf(parse_addr, data_len, &parse_addr, &error_addr);
		if(xmlnode) {
			list_add_tail(&xmlnode->lst_self, &xmlroot->lst_xml);
			xmlroot->num++ ;
		}
		else if(error_addr) {
			nexml_destroy(xmlroot) ;
			show_xmlerror(file, error_addr, text_buf, (size_t) data_len) ;
			return -1;
		}
	} while(parse_addr && *parse_addr);

	free(text_buf) ;
	return 0 ;

}

void nexml_destroy(nexml_root *xmlroot)
{
	nexml *sub_xml; 
	struct list_head *pos = xmlroot->lst_xml.next ;
	while (pos!=&xmlroot->lst_xml) {
		sub_xml = list_entry(pos,struct tagxml, lst_self) ;
		pos = pos->next ;
		dealloc_xml(sub_xml) ;
		
	}
	nexml_initroot(xmlroot) ;
}

//显示xml错误
void show_xmlerror(const NEINT8 *file, NEINT8 *error_addr, NEINT8 *xmlbuf, size_t size)
{
	NEINT32 line = 0 ;
	NEINT8 *pline, *pnext ;
	NEINT8 errbuf[1024] ;

	errbuf[0] = 0 ;
	pline = xmlbuf ;

	//pnext = pline ;

	while (pline<xmlbuf+size) {
		//pline = pnext ;
		pnext = strchr(pline, '\n') ;
		if(!pnext) {
			snprintf(errbuf, 1024, "know error in file %s  line %d", file, line) ;
			break ;				
		}
		pnext  ;
		++line ;
		if(error_addr>=pline && error_addr< pnext) {
//			size_t l_size ;
//			NEINT8 line_text[512] ;
			//found error code 
			pline = nestr_first_valid(pline) ;

//			l_size = pnext - pline ;
//			l_size = min(l_size, 512) ;
			//strncpy(line_text, pline, l_size) ;
//			nestr_nstr_end(pline, line_text, '\n', 512) ;
			//snprintf(errbuf, 1024, "error %s %d lines [%s]", file, line, line_text) ;
			snprintf(errbuf, 1024, "parse error in file %s  line %d \n", file, line) ;
			break ;				
		}
		pline = ++pnext ;
	}
	
	if(__xml_logfunc && errbuf[0]) {
		__xml_logfunc(errbuf) ;
	}
	return ;
}

 xml_errlog ne_setxml_log(xml_errlog logfunc) ;
NEINT32 nexml_save(nexml_root *xmlroot, const NEINT8 *file)
{
	FILE *fp;
	nexml *sub_xml; 
	struct list_head *pos = xmlroot->lst_xml.next ;

	fp = fopen(file, "w") ;
	if(!fp) {
		return -1;
	}
	while (pos!=&xmlroot->lst_xml) {
		sub_xml = list_entry(pos,struct tagxml, lst_self) ;
		pos = pos->next ;
		xml_write(sub_xml,fp, 0) ;
		
	}

	fclose(fp) ;
	
	return 0;
}

NEINT32 nexml_merge(nexml_root *host, nexml_root *merged) 
{
	if(merged->num > 0) {
		list_join(&merged->lst_xml, &host->lst_xml) ;
		host->num += merged->num ;

		nexml_initroot(merged);
	}
	return 0 ;
}

nexml *nexml_getnode(nexml_root *xmlroot, NEINT8 *name) 
{
	nexml *sub_xml; 
	struct list_head *pos = xmlroot->lst_xml.next ;
	while (pos!=&xmlroot->lst_xml) {
		sub_xml = list_entry(pos,struct tagxml, lst_self) ;
		pos = pos->next ;
		if (0==nestricmp((NEINT8*)name,sub_xml->name)){
			return sub_xml ;
		}
	}
	return NULL ;
}
nexml *nexml_getnodei(nexml_root *xmlroot, NEINT32 index) 
{
	NEINT32 i = 0 ;
	nexml *sub_xml; 
	struct list_head *pos = xmlroot->lst_xml.next ;

	while (pos!=&xmlroot->lst_xml) {
		sub_xml = list_entry(pos,struct tagxml, lst_self) ;
		pos = pos->next ;
		if(i==index){
			return sub_xml ;
		}
		++i ;
	}
	return NULL ;
}

nexml *nexml_addnode(nexml_root *xmlroot, NEINT8 *name,NEINT8 *value) 
{
	nexml *xmlnode = _create_xmlnode(name, value) ;
	if(xmlnode){
		list_add_tail(&xmlnode->lst_self, &xmlroot->lst_xml);
		xmlroot->num++ ;
	}
	return xmlnode ;
}

NEINT32 nexml_delnode(nexml_root *xmlroot, NEINT8 *name) 
{
	nexml *node = nexml_getnode(xmlroot,name) ;
	if(!node)
		return -1 ;
	list_del(&node->lst_self) ;
	xmlroot->num-- ;
	dealloc_xml(node);
	return 0 ;
}
NEINT32 nexml_delnodei(nexml_root *xmlroot, NEINT32 index) 
{
	nexml *node = nexml_getnodei(xmlroot,index) ;
	if(!node)
		return -1 ;
	list_del(&node->lst_self) ;
	xmlroot->num-- ;
	dealloc_xml(node);
	return 0 ;
}
//引用一个子节点
nexml *nexml_refsub(nexml *root, const NEINT8 *name) 
{
	nexml *sub_xml; 
	struct list_head *pos = root->lst_sub.next ;
	while (pos!=&root->lst_sub) {
		sub_xml = list_entry(pos,struct tagxml, lst_self) ;
		pos = pos->next ;
		if (0==nestricmp((NEINT8*)name,sub_xml->name)){
			return sub_xml ;
		}
	}
	return NULL ;
}

//引用一个子节点
nexml *nexml_refsubi(nexml *root, NEINT32 index) 
{
	NEINT32 i = 0 ;
	nexml *sub_xml; 
	struct list_head *pos = root->lst_sub.next ;

	while (pos!=&root->lst_sub) {
		sub_xml = list_entry(pos,struct tagxml, lst_self) ;
		pos = pos->next ;
		//if (0==nestricmp(name,sub_xml->name)){
		if(i==index){
			return sub_xml ;
		}
		++i ;
	}
	return NULL ;
}

//得到xml的值
NEINT8 *nexml_getval(nexml *node)
{
	if(node->value && node->value[0])
		return node->value ;
	else
		return NULL ;
}

//得到属性值
struct nexml_attr *nexml_getattrib(nexml *node , NEINT8 *name)
{
	struct nexml_attr *attr ;
	struct list_head *pos = node->lst_attr.next;
	while (pos!= &node->lst_attr){
		attr = list_entry(pos,struct nexml_attr, lst) ;
		pos=pos->next ;
		if(0==nestricmp(name, (NEINT8*)(attr+1))) {
			return attr ;
		}
	}
	return NULL ;
}

struct nexml_attr  *nexml_getattribi(nexml *node, NEINT32 index)
{
	NEINT32 i=0 ;
	struct nexml_attr *attr ;
	struct list_head *pos = node->lst_attr.next;
	while (pos!= &node->lst_attr){
		attr = list_entry(pos,struct nexml_attr, lst) ;
		pos=pos->next ;
		//if(0==nestricmp(name,attr->name)) {
		if(i==index){
			return attr ;
		}
		++i ;
	}
	return NULL ;

}

//////////////////////////////////////////////////////////////////////////
//给xml增加一个属性,
struct nexml_attr  *nexml_addattrib(nexml *parent, NEINT8 *name, NEINT8 *value) 
{
	struct nexml_attr *attrib_node;
	if(!name || !value)
		return NULL ;
	attrib_node = alloc_attrib_node(name, value) ;
	if(attrib_node) {
		list_add_tail(&attrib_node->lst, &parent->lst_attr) ;
		(parent->attr_num)++ ;
	}
	return attrib_node ;

}


NEINT32 nexml_setattrval(nexml *parent, NEINT8 *name, NEINT8 *value) 
{
	NEINT32 len ;
	struct nexml_attr  * attr ;

	if( !name) {
		return -1 ;
	}
	if(!value || (len = strlen(value)) == 0) {
		return nexml_delattrib(parent, name) ;
	}

	attr = nexml_getattrib(parent,  name);

	if(!attr) {
		struct nexml_attr* attr = nexml_addattrib(parent,name, value)  ;
		if(attr) {
			return 0 ;
		}
		return -1 ;
	}
	
	if(len > attr->value_size) {
		struct nexml_attr *attrib_node;
		attrib_node = alloc_attrib_node((NEINT8*)(attr + 1), value) ;
		if(!attrib_node) {
			return -1 ;
		}
		list_add(&attrib_node->lst, &attr->lst);

		list_del(&attr->lst) ;
		dealloc_attrib_node(attr);
	}
	else {
		strcpy((NEINT8*)(attr + 1)+attr->name_size, value) ;
	}
	return 0;
}
NEINT32 nexml_setattrvali(nexml *parent, NEINT32 index, NEINT8 *value) 
{
	
	NEINT32 len ;
	struct nexml_attr  * attr ;

	if( index <0 || index>= parent->attr_num) {
		return -1 ;
	}
	if(!value || (len = strlen(value)) == 0) {
		return nexml_delattribi(parent, index) ;
	}

	attr = nexml_getattribi(parent,  index);

	if(!attr) {
		return -1 ;
	}
	
	if(len > attr->value_size) {
		struct nexml_attr *attrib_node;
		attrib_node = alloc_attrib_node((NEINT8*)(attr + 1), value) ;
		if(!attrib_node) {
			return -1 ;
		}
		list_add(&attrib_node->lst, &attr->lst);

		list_del(&attr->lst) ;
		dealloc_attrib_node(attr);
		return 0 ;
	}
	else {
		strcpy((NEINT8*)(attr + 1)+attr->name_size, value) ;
	}
	return 0;
}
//给xml增加一个子节点需要输入新节点的名字和值,返回新节点地址
nexml *nexml_addsubnode(nexml *parent, NEINT8 *name, NEINT8 *value) 
{
	nexml *xmlnode = _create_xmlnode(name, value) ;
	if(xmlnode){
		list_add_tail(&xmlnode->lst_self, &parent->lst_sub);
		parent->sub_num++ ;
	}
	return xmlnode ;
}

//设置XML的值
NEINT32 nexml_setval(nexml *node , NEINT8 *val) 
{
	NEINT32 len;
	if(!val)
		return -1 ;

	len = strlen(val) ;
	if(len==0)
		return -1 ;

	++len;
	if(node->value){
		if(len>node->val_size) {
			NEINT8 *tmp ;
			
			len += 4 ; len &= ~3;
			tmp = malloc(len);
			if(!tmp)
				return -1 ;
			free(node->value) ;
			node->value = tmp ;
		}
	}
	else {
		len += 4 ; len &= ~3;
		node->value = malloc(len);
		if(!node->value)
			return -1 ;
	}
	strcpy(node->value,val) ;
	return 0 ;
}
//删除一个属性节点
NEINT32 nexml_delattrib(nexml *parent, NEINT8 *name) 
{
	struct nexml_attr *attr = nexml_getattrib(parent , name);
	if(!attr)
		return -1 ;
	list_del(&attr->lst) ;
	parent->attr_num-- ;
	dealloc_attrib_node(attr);
	return 0 ;

}
NEINT32 nexml_delattribi(nexml *parent, NEINT32 index) 
{
	struct nexml_attr *attr = nexml_getattribi(parent , index);
	if(!attr)
		return -1 ;
	list_del(&attr->lst) ;
	parent->attr_num-- ;
	dealloc_attrib_node(attr);
	return 0 ;
}
//删除一个子节点
NEINT32 nexml_delsubnode(nexml *parent, NEINT8 *name) 
{
	nexml *node = nexml_refsub(parent,name) ;
	if(!node)
		return -1 ;
	list_del(&node->lst_self) ;
	parent->sub_num-- ;
	dealloc_xml(node);
	return 0 ;
}
NEINT32 nexml_delsubnodei(nexml *parent, NEINT32 index) 
{
	nexml *node = nexml_refsubi(parent,index) ;
	if(!node)
		return -1 ;
	list_del(&node->lst_self) ;
	parent->sub_num-- ;
	dealloc_xml(node);
	return 0 ;
}
//////////////////////////////////////////////////////////////////////////
//去掉注释
static NEINT8* parse_marked(NEINT8 *xmlbuf, NEINT32 size, NEINT8 **error_addr) 
{
	NEINT8 *pstart = xmlbuf ;
	NEINT8 *paddr ;
	
	while(pstart< xmlbuf+size) {
		*error_addr = pstart ;
		paddr = strchr(pstart, '<') ;

		if(!paddr || paddr >= xmlbuf+size) {
			*error_addr = 0 ;
			return NULL ;
		}
		if('?'==paddr[1]) {
			paddr = strstr(paddr+2, "?>") ;
			if(!paddr || paddr >= xmlbuf+size) {
				return NULL ;
			}
			paddr += 2 ;
		}
		else if(paddr[1]=='!' ) {
			if(paddr[2]=='-' && paddr[3]=='-') {
				paddr = strstr(paddr+4, "-->") ;
				if(!paddr || paddr >= xmlbuf+size) {
					return NULL ;
				}
				paddr += 3 ;
			}
			else {
				return NULL ;
			}
		}
		else {
			*error_addr = NULL ;
			return paddr ;
		}
		
		pstart = paddr ;
	}
	return NULL ;
}
//从内存块中解析出一个XML节点
nexml *parse_xmlbuf(NEINT8 *xmlbuf, NEINT32 size, NEINT8 **parse_end, NEINT8 **error_addr)
{
	nexml *xmlnode = NULL ;
	NEINT8 *paddr ;//, *error_addr =NULL;
	NEINT8 buf[1024] ;
	
	paddr = parse_marked(xmlbuf, size, error_addr)  ;
	if(!paddr){
		*parse_end = NULL ;
		return NULL ;
	}
	if(XML_T_END==*((NEINT16 *)paddr)) {
		*parse_end = paddr ;
		*error_addr = NULL ;
		return NULL ;
		//goto READ_END ;
	}
	++paddr ;

	paddr = nestr_first_valid(paddr) ;
	if(!paddr) {
		*parse_end = NULL ;
		return NULL ;
	}
	//check valid 
	if(*paddr=='>' || *paddr=='<' || *paddr=='\"' || *paddr=='/') {
		*parse_end = NULL ;
		*error_addr = paddr ;
		return NULL ;
	}

	xmlnode = alloc_xml() ;
	if(!xmlnode) {
		*parse_end = NULL ;
		return NULL ;
	}
	paddr = nestr_parse_word(paddr, buf) ;		//xml node name
	strncpy(xmlnode->name, buf, MAX_XMLNAME_SIZE) ;

	paddr = nestr_first_valid(paddr) ;
	//read attribe
	while(*paddr) {
		struct nexml_attr *attrib_node ;
		NEINT8 attr_name[MAX_XMLNAME_SIZE] ;
		if(*((NEINT16 *)paddr)==XML_H_END ) {
			//这个xml节点结束了,应该返回了
			*parse_end = paddr + 2 ;
			return xmlnode ;
		}
		else if('>'==*paddr) {
			//节点头结束,退出属性读取循环
			paddr++ ;
			break ;
		}
		//read attrib name 
		paddr = nestr_parse_word(paddr, attr_name) ;
		paddr = strchr(paddr,_NE_QUOT) ;
		if(!paddr) {
			dealloc_xml(xmlnode) ;
			*parse_end = NULL ;
			return NULL ;
		}
		++paddr ;
		//read attrib VALUE
		paddr = nestr_str_end(paddr,buf, _NE_QUOT) ;
		++paddr ;
		
		attrib_node = alloc_attrib_node(attr_name, buf) ;
		if(attrib_node) {
			list_add_tail(&attrib_node->lst, &xmlnode->lst_attr) ;
			(xmlnode->attr_num)++ ;
		}
		paddr = nestr_first_valid(paddr) ;
	}
	
	//read value and sub-xmlnode
	paddr = nestr_first_valid(paddr) ;
	if(XML_T_END==*((NEINT16*)paddr)) {
		//xml node end </
		goto READ_END ;
	}
	else if('<'==*paddr) {
		//使用递归去解析子xml node
		while (*paddr)	{
			NEINT8 *parsed ;
			NEINT32  left_size = size -( paddr - xmlbuf) ;
			nexml *new_xml = parse_xmlbuf(paddr, left_size, &parsed, error_addr) ;
			if(new_xml) {
				list_add_tail(&new_xml->lst_self, &xmlnode->lst_sub);
				xmlnode->sub_num++ ;
			}
			else if(*error_addr || NULL==parsed) {
				//parse error 
				dealloc_xml(xmlnode) ;
				*parse_end = NULL ;
				return NULL ;
			}
			paddr = nestr_first_valid(parsed) ;
			if(XML_T_END==*((NEINT16*)paddr)) {
				goto READ_END ;
			}

		}
	}
	else {
		//read xml value 
		size_t val_size ;
		NEINT8 *tmp  ;

		tmp = nestr_reverse_chr(paddr, '>', xmlbuf) ;
		if(!tmp){
			tmp = paddr ;
		}
		else {
			++tmp ;
		}
		paddr = nestr_str_end(tmp,buf, '<') ;		//读取xml值,一直到"<"结束
		val_size = paddr - tmp ;
		
		//store value 
		val_size += 4 ;val_size &= ~3 ;

		xmlnode->value = malloc(val_size) ;
		if(xmlnode->value) {
			strcpy(xmlnode->value, buf) ;
			xmlnode->val_size = val_size ;
		}
		else {
			dealloc_xml(xmlnode) ;
			*parse_end = NULL ;
			return NULL ;
		}
	}
	
	//read end
READ_END :
	{
		NEINT8 end_name[MAX_XMLNAME_SIZE] ;
		
		if(XML_T_END != *((NEINT16*)paddr)) {
			//解析出错,没有遇到结束标志
			if(xmlnode)
				dealloc_xml(xmlnode) ;
			*parse_end = NULL ;
			return NULL ;
		}
		
		//check xml end
		paddr += 2 ; //skip </
		paddr = nestr_first_valid(paddr) ;
		
		paddr = nestr_parse_word(paddr,end_name) ;
		if(nestricmp(xmlnode->name, end_name)) {
			//解析出错,没有遇到结束标志,或者是结束标志写错了
			dealloc_xml(xmlnode) ;
			*parse_end = NULL ;
			return NULL ;
		}
		paddr = strchr(paddr,'>') ;
		paddr++ ;
		*parse_end = paddr ;
	} 
	return xmlnode ;
}

//申请一个节点内存
nexml *alloc_xml()
{
	nexml *node = malloc(sizeof(nexml)) ;
	if(node) {
		init_xml_node(node) ;
	}
	return node ;
}

//释放一个XML节点的所以资源
void  dealloc_xml(nexml *node )
{
	struct list_head *pos ;
	
	//dealloc value 
	if(node->value) {
		free(node->value) ;
		node->value = 0 ;
	}

	//dealloc attribute
	pos = node->lst_attr.next ;
	while(pos != &node->lst_attr) {
		struct nexml_attr *attrnode ;
		attrnode = list_entry(pos, struct nexml_attr, lst) ;
		pos = pos->next ;
		dealloc_attrib_node(attrnode) ;
	}

	//dealloc sub-xmlnode 
	pos = node->lst_sub.next ;
	while(pos != &node->lst_sub) {
		struct tagxml  *_xml ;
		_xml = list_entry(pos, struct tagxml, lst_self) ;
		pos = pos->next ;
		list_del(&_xml->lst_self) ;
		dealloc_xml(_xml) ;
	}

	free(node) ;
}

//申请一个属性节点的内存
struct nexml_attr *alloc_attrib_node(NEINT8 *name, NEINT8 *value)
{
	NEINT8 *p ;
	struct nexml_attr *pnode ;
	NEINT32 len = strlen(name) ;
	NEINT32 val_len = strlen(value) ;
	if(!len || !val_len)  {
		return NULL;
	}
	len += 4 ;
	len &= ~3 ;

	val_len += 4 ;
	val_len &= ~3 ;

	pnode = malloc(sizeof(struct nexml_attr) + len + val_len) ;
	if(!pnode) {
		return NULL ;
	}
	init_xml_attr(pnode) ;

	pnode->name_size = len ;
	pnode->value_size = val_len ;
	//pnode->name = (NEINT8*) (pnode + 1) ;
	//pnode->value = pnode->name + len ;
	p = (NEINT8*) (pnode + 1) ;
	strcpy(p , name) ;

	p += len ;
	strcpy(p, value) ;

	return pnode ;
}

//释放一个属性节点内存资源
void dealloc_attrib_node(struct nexml_attr *pnode)
{
	list_del(&pnode->lst);
	free(pnode) ;
}

//create a xml node input name and value 
nexml *_create_xmlnode(NEINT8 *name, NEINT8 *value) 
{
	nexml *xmlnode ;
	
	if(!name)
		return NULL ;

	xmlnode = alloc_xml() ;
	if(!xmlnode) {
		return NULL ;
	}
	strncpy(xmlnode->name,	name,MAX_XMLNAME_SIZE) ;
	if(value) {
		NEINT32 len = strlen(value) ;
		if(len> 0) {
			len += 4 ;
			len &= ~3 ;
			xmlnode->value = malloc(len) ;
			if(xmlnode->value){
				strcpy(xmlnode->value, value) ;	
				xmlnode->val_size = len ;
			}else {
				free(xmlnode) ;
				return NULL ;
			}
		}
	}
	
	return xmlnode ;
}

//#define INDENT(fp, n) while(n--) {	\
//	fprintf(fp,"\t"); } 

static __INLINE__ void indent(FILE *fp, NEINT32 deep)
{
	while(deep-- > 0) {
		fprintf(fp,"\t"); 
	}
}
//把xml写到文件中
//@deep 节点的深度
NEINT32 xml_write(nexml *xmlnode, FILE *fp , NEINT32 deep)
{
	struct list_head *pos ;

	indent(fp,deep) ;
	fprintf(fp, "<%s", xmlnode->name) ;
	
	//save attribute to file 
	pos = xmlnode->lst_attr.next ;
	while (pos != &xmlnode->lst_attr){
		struct nexml_attr *xml_attr = list_entry(pos, struct nexml_attr, lst) ;
		pos = pos->next ;
		//if(xml_attr->name && xml_attr->value)
			fprintf(fp, " %s=\"%s\"", (NEINT8*)(xml_attr + 1),(NEINT8*)(xml_attr + 1) +xml_attr->name_size) ;
	}
	
	//save value of sub-xmlnode
	if(xmlnode->value && xmlnode->value[0]) {
		fprintf(fp, ">%s</%s>\n", xmlnode->value, xmlnode->name) ;
	}
	else if (xmlnode->sub_num>0){
		fprintf(fp, ">\n") ;

		pos = xmlnode->lst_sub.next ;
		while (pos != &xmlnode->lst_sub){
			nexml *subxml = list_entry(pos, struct tagxml, lst_self) ;
			pos = pos->next ;
			xml_write(subxml, fp , deep+1);
		}
		indent(fp,deep) ;
		fprintf(fp, "</%s>\n", xmlnode->name) ;
	}
	else {
		fprintf(fp, "/>\n") ;
	}
	return 0 ;
}

void _errlog (NEINT8 *errdesc) 
{
	fprintf(stderr, errdesc) ;	
}
