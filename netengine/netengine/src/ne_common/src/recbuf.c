#include "ne_common/ne_common.h"
#include "ne_common/ne_recbuf.h"

//读写数据的直接模式,如果只是在一个线程中使用buf
//可以用_cbuf_write/_cbuf_read来读写数据提高效率
//但是不推荐使用
//往缓冲区种写数据的底层帮助函数
NEINT32 _cbuf_write(ndrecybuf_t *pbuf,NEINT8 *data, size_t len);
//从缓冲区中读取数据
//buf数据被读入的地址,size需要读取字符的长度
//return value 读取数据的长度
NEINT32 _cbuf_read(ndrecybuf_t *pbuf,NEINT8 *buf, size_t size);

//__INLINE__ NEINT8 *cbuf_addr(ndrecybuf_t *pbuf) {return pbuf->m_buf ;}
__INLINE__ NEINT8 *cbuf_tail(ndrecybuf_t *pbuf)
	{return (NEINT8*)pbuf->m_buf+(size_t)C_BUF_SIZE;}	//缓冲尾巴

//返回数据存放的顺序0 start < end 
//__INLINE__ NEINT32 cbuf_order(ndrecybuf_t *pbuf)	
//{return (pbuf->m_pend>=pbuf->m_pstart) ;}


//返回数据的长度
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

//得到空闲缓冲长度
//能存储数据的容量始终比capacity 少一个,如果相等我会认为是空的
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

//读写数据
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
//往缓冲区种写数据的底层帮助函数
NEINT32 _cbuf_write(ndrecybuf_t *pbuf,NEINT8 *data, size_t len)
{
	size_t ret = len ;
	size_t  taillen ;
	/*size_t spaceLen = cbuf_freespace(pbuf) ;
	if(len>spaceLen) {
		//等待缓冲数据被读走
		//need to do something !
		return -1 ;
	}
	*/
	ne_assert(pbuf->m_nInit) ;
	//缓冲够用,写入数据
	taillen = (size_t)(cbuf_tail(pbuf)-pbuf->m_pend );
	if(taillen>=len){
		//空间够用 不需要回头
		memcpy((void*)pbuf->m_pend,(void*)data,len ) ;
		pbuf->m_pend += len ;
		if(pbuf->m_pend>=cbuf_tail(pbuf)){
			pbuf->m_pend = pbuf->m_buf ;
		}
		
	}
	else {
		//数据开始循环
		memcpy((void*)pbuf->m_pend,(void*)data,taillen ) ;
		len -= taillen ;
		data += taillen ;
		memcpy((void*)pbuf->m_buf,(void*)data, len) ;
		pbuf->m_pend = pbuf->m_buf+len ;
	}
	return (NEINT32)ret ;
}

//从缓冲区中读取数据
//buf数据被读入的地址,size需要读取字符的长度
//return value 读取数据的长度
NEINT32 _cbuf_read(ndrecybuf_t *pbuf,NEINT8 *buf, size_t size)
{
	size_t ret = size ;
	size_t  taillen =0;
	/*size_t data_len = cbuf_datalen(pbuf) ;
	if(size>data_len) {
		//没有这么多数据,等待数据写入
		//need to do something !
		return -1 ;
	}
	*/
	ne_assert(pbuf->m_nInit) ;
	//缓冲有着么多数据够用
	if(cbuf_order(pbuf)){
		//数据没有回头
		memcpy((void*)buf,(void*)pbuf->m_pstart,size ) ;
		pbuf->m_pstart += size ;
		if(pbuf->m_pstart==pbuf->m_pend) {
			ndcbuf_reset(pbuf) ;
		}
		return size ;
	}

	//数据开始循环
	taillen = (size_t)(cbuf_tail(pbuf) - pbuf->m_pstart );
	if(taillen>=size) {
		//尾部数据够读取的长度
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
