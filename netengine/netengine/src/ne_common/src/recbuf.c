#include "ne_common/ne_common.h"
#include "ne_common/ne_recbuf.h"

//��д���ݵ�ֱ��ģʽ,���ֻ����һ���߳���ʹ��buf
//������_cbuf_write/_cbuf_read����д�������Ч��
//���ǲ��Ƽ�ʹ��
//����������д���ݵĵײ��������
NEINT32 _cbuf_write(ndrecybuf_t *pbuf,NEINT8 *data, size_t len);
//�ӻ������ж�ȡ����
//buf���ݱ�����ĵ�ַ,size��Ҫ��ȡ�ַ��ĳ���
//return value ��ȡ���ݵĳ���
NEINT32 _cbuf_read(ndrecybuf_t *pbuf,NEINT8 *buf, size_t size);

//__INLINE__ NEINT8 *cbuf_addr(ndrecybuf_t *pbuf) {return pbuf->m_buf ;}
__INLINE__ NEINT8 *cbuf_tail(ndrecybuf_t *pbuf)
	{return (NEINT8*)pbuf->m_buf+(size_t)C_BUF_SIZE;}	//����β��

//�������ݴ�ŵ�˳��0 start < end 
//__INLINE__ NEINT32 cbuf_order(ndrecybuf_t *pbuf)	
//{return (pbuf->m_pend>=pbuf->m_pstart) ;}


//�������ݵĳ���
size_t ndcbuf_datalen(ndrecybuf_t *pbuf)
{
	ne_assert(pbuf->m_nInit) ;
	if(cbuf_order(pbuf)) {
		return (size_t)(pbuf->m_pend - pbuf->m_pstart) ;
	}
	else {
		return C_BUF_SIZE - (size_t)(pbuf->m_pstart-pbuf->m_pend) ;
	}
}

//�õ����л��峤��
//�ܴ洢���ݵ�����ʼ�ձ�capacity ��һ��,�������һ���Ϊ�ǿյ�
size_t ndcbuf_freespace(ndrecybuf_t *pbuf)
{
	size_t ret  ;
	if(cbuf_order(pbuf)) {
		ret = cbuf_capacity(pbuf) - (size_t)(pbuf->m_pend - pbuf->m_pstart);
	}
	else {
		ret = (size_t)(pbuf->m_pstart - pbuf->m_pend)  ;
	}
	return ret -1 ;
}

//��д����
NEINT32 ndcbuf_read(ndrecybuf_t *pbuf,void *buf, size_t size,NEINT32 flag)
{
	size_t datalen = ndcbuf_datalen(pbuf) ;
	if(EBUF_SPECIFIED==flag) {
		if(datalen<size){
			return -1 ;
		}
	}
	else {
		if(datalen<size)
			size = datalen ;
	}
	return _cbuf_read(pbuf,buf,size) ;
}

NEINT32 ndcbuf_write(ndrecybuf_t *pbuf,void *data, size_t datalen,NEINT32 flag)
{
	size_t spacelen = ndcbuf_freespace(pbuf) ;
	if(EBUF_SPECIFIED==flag) {
		if(spacelen<datalen){
			return -1 ;
		}
	}
	else {
		if(spacelen<datalen)
			datalen = spacelen;
	}
	return _cbuf_write(pbuf,data,datalen) ;
}

void ndcbuf_sub_data(ndrecybuf_t *pbuf,size_t len)
{
	pbuf->m_pstart += len ;
	if(pbuf->m_pstart==pbuf->m_pend )
		ndcbuf_reset(pbuf);
}
//����������д���ݵĵײ��������
NEINT32 _cbuf_write(ndrecybuf_t *pbuf,NEINT8 *data, size_t len)
{
	size_t ret = len ;
	size_t  taillen ;
	/*size_t spaceLen = cbuf_freespace(pbuf) ;
	if(len>spaceLen) {
		//�ȴ��������ݱ�����
		//need to do something !
		return -1 ;
	}
	*/
	ne_assert(pbuf->m_nInit) ;
	//���幻��,д������
	taillen = (size_t)(cbuf_tail(pbuf)-pbuf->m_pend );
	if(taillen>=len){
		//�ռ乻�� ����Ҫ��ͷ
		memcpy((void*)pbuf->m_pend,(void*)data,len ) ;
		pbuf->m_pend += len ;
		if(pbuf->m_pend>=cbuf_tail(pbuf)){
			pbuf->m_pend = pbuf->m_buf ;
		}
		
	}
	else {
		//���ݿ�ʼѭ��
		memcpy((void*)pbuf->m_pend,(void*)data,taillen ) ;
		len -= taillen ;
		data += taillen ;
		memcpy((void*)pbuf->m_buf,(void*)data, len) ;
		pbuf->m_pend = pbuf->m_buf+len ;
	}
	return (NEINT32)ret ;
}

//�ӻ������ж�ȡ����
//buf���ݱ�����ĵ�ַ,size��Ҫ��ȡ�ַ��ĳ���
//return value ��ȡ���ݵĳ���
NEINT32 _cbuf_read(ndrecybuf_t *pbuf,NEINT8 *buf, size_t size)
{
	size_t ret = size ;
	size_t  taillen =0;
	/*size_t data_len = cbuf_datalen(pbuf) ;
	if(size>data_len) {
		//û����ô������,�ȴ�����д��
		//need to do something !
		return -1 ;
	}
	*/
	ne_assert(pbuf->m_nInit) ;
	//��������ô�����ݹ���
	if(cbuf_order(pbuf)){
		//����û�л�ͷ
		memcpy((void*)buf,(void*)pbuf->m_pstart,size ) ;
		pbuf->m_pstart += size ;
		if(pbuf->m_pstart==pbuf->m_pend) {
			ndcbuf_reset(pbuf) ;
		}
		return size ;
	}

	//���ݿ�ʼѭ��
	taillen = (size_t)(cbuf_tail(pbuf) - pbuf->m_pstart );
	if(taillen>=size) {
		//β�����ݹ���ȡ�ĳ���
		memcpy((void*)buf,(void*)pbuf->m_pstart,size ) ;		
		pbuf->m_pstart += size ;
		if(pbuf->m_pstart>=cbuf_tail(pbuf)) {
			pbuf->m_pstart = pbuf->m_buf ;
		}
	}
	else {
		memcpy((void*)buf,(void*)pbuf->m_pstart,taillen ) ;
		size -= taillen ;
		buf += taillen ;
		memcpy((void*)buf,(void*)pbuf->m_buf, size) ;
		pbuf->m_pstart = pbuf->m_buf + size ;
		if(pbuf->m_pstart==pbuf->m_pend) {
			ndcbuf_reset(pbuf) ;
		}
	}
	return ret ;
}
/************************************************************************/
/* line buff                                                            */
/************************************************************************/

void _lbuf_move_ahead(struct ne_linebuf *pbuf) 
{
	size_t ahead = pbuf->__start - pbuf->__buf;
	size_t len = _lbuf_datalen(pbuf);
	if(len == 0){
		_lbuf_reset(pbuf);
	}
	else if(ahead >= sizeof(size_t)){			//avoid recover
		memcpy((void*)(pbuf->__buf),pbuf->__start, len) ;
		pbuf->__start = pbuf->__buf; 
		pbuf->__end = pbuf->__buf + len ;
	}
	
}

void _lbuf_tryto_move_ahead(struct ne_linebuf *pbuf) 
{
	size_t header = pbuf->__start - pbuf->__buf ;
	//size_t tailer = pbuf->__buf + lbuf_capacity(pbuf) ;
	if(header>= (_lbuf_capacity(pbuf)>>2) ) {
		_lbuf_move_ahead(pbuf) ;
	}
}

void _lbuf_sub_data(struct ne_linebuf *pbuf,size_t len)
{
	ne_assert(pbuf->__start+len <= pbuf->__end) ;
	ne_assert(pbuf->__start+len <= pbuf->__buf + _lbuf_capacity(pbuf)) ;
	pbuf->__start += len ;
	if(pbuf->__start == pbuf->__end)
		_lbuf_reset(pbuf);
	else 
		_lbuf_tryto_move_ahead(pbuf);
}


NEINT32 _lbuf_read(struct ne_linebuf *pbuf,void *buf, size_t size,NEINT32 flag)
{
	size_t datalen = _lbuf_datalen(pbuf) ;
	if(EBUF_SPECIFIED == flag) {
		if(datalen<size){
			return -1 ;
		}
	}
	else {
		if(datalen<size)
			size = datalen ;
	}

	memcpy(buf,pbuf->__start, size) ;
	_lbuf_sub_data(pbuf, size) ;
	return (NEINT32)size ;

}

NEINT32 _lbuf_write(struct ne_linebuf *pbuf,void *data, size_t datalen,NEINT32 flag)
{
	size_t space_len = _lbuf_freespace(pbuf) ;
	if(EBUF_SPECIFIED){
		if(space_len < datalen ) {
			size_t free_capacity = _lbuf_free_capacity(pbuf) ;
			if(free_capacity < datalen)
				return -1 ;
			_lbuf_move_ahead(pbuf) ;
		}
	}
	else {
		if(space_len < datalen ) {
			_lbuf_move_ahead(pbuf) ;
			space_len = _lbuf_free_capacity(pbuf) ;
			if(space_len < datalen)
				datalen = space_len ;
		}
	}
	memcpy(pbuf->__end, data, datalen) ;
	//pbuf->__end += datalen ;
	_lbuf_add_data(pbuf, datalen) ;
	return (NEINT32) datalen ;
}

void _lbuf_add_data(struct ne_linebuf *pbuf,size_t len)
{
	pbuf->__end += len ;
	ne_assert(pbuf->__end <= pbuf->__buf +_lbuf_capacity(pbuf) ) ;
}
