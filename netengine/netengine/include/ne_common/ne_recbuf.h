/*
 * ����udpʵ����tcp���ܵ�ʱ����Ҫ�����Ļ���,������дһ������
 * c++ ��class (NE_CRecyBuf)
 */

#ifndef _CBUF_H_
#define _CBUF_H_

enum{ C_BUF_SIZE = NE_BUFSIZE} ;	
typedef struct ne_recbuf
{
	NEINT8 *m_pstart, *m_pend ;	
#ifdef NE_DEBUG
	NEINT32 m_nInit ;		//�Ƿ��ʼ��
#endif
	NEINT8 m_buf[C_BUF_SIZE] ;
}ndrecybuf_t ;	

enum _eNDRecyRead{
	EBUF_ALL=0,	//��ȡ���������������ָ������,���ȡ��������������.д��ʱ���岻������������
	EBUF_SPECIFIED	//��ȡ(д��)ָ����,�������(�ռ�)��������
};

static __INLINE__ size_t cbuf_capacity(ndrecybuf_t *pbuf) 
{
	return (size_t)C_BUF_SIZE;
}	//�õ�����

static __INLINE__ void ndcbuf_reset(ndrecybuf_t *pbuf)
{
	pbuf->m_pstart = pbuf->m_buf;
	pbuf->m_pend = pbuf->m_buf ;	
}

static __INLINE__ void ndcbuf_init(ndrecybuf_t *pbuf) 
{
	ndcbuf_reset(pbuf);

#ifdef NE_DEBUG
	pbuf->m_nInit = 1 ; 
#endif
}

static __INLINE__ void ndcbuf_destroy(ndrecybuf_t *pbuf)
{
#ifdef NE_DEBUG
	pbuf->m_nInit = 0 ; 
#endif
}

static __INLINE__ void* ndcbuf_data_addr(ndrecybuf_t *pbuf)
{
	return pbuf->m_pstart ;
}

static __INLINE__ void ndcbuf_add_data(ndrecybuf_t *pbuf,size_t len)
{
	pbuf->m_pend += len ;
}

//�������ݴ�ŵ�˳�� 0 start < end 
static __INLINE__ NEINT32 cbuf_order(ndrecybuf_t *pbuf)	
{
	return (pbuf->m_pend>=pbuf->m_pstart) ;
}

NE_COMMON_API void ndcbuf_sub_data(ndrecybuf_t *pbuf,size_t len);
NE_COMMON_API size_t ndcbuf_datalen(ndrecybuf_t *pbuf); //�������ݵĳ���
NE_COMMON_API size_t ndcbuf_freespace(ndrecybuf_t *pbuf);//�õ����л��峤��

//��д����
//return value -1error else return data length of  read/write 
NE_COMMON_API NEINT32 ndcbuf_read(ndrecybuf_t *pbuf,void *buf, size_t size,NEINT32 flag);
NE_COMMON_API NEINT32 ndcbuf_write(ndrecybuf_t *pbuf,void *data, size_t datalen,NEINT32 flag);

/************************************************************************/
/* define line io buffer                                                */
/************************************************************************/

struct ne_linebuf
{
	NEUINT32 buf_capacity ;
	NEINT8 *__start, *__end ;	
	NEINT8 __buf[C_BUF_SIZE] ;	
};

//�������Ի���ͷ
struct line_buf_hdr
{
	NEUINT32 buf_capacity ;
	NEINT8 *__start, *__end ;	
};
static __INLINE__ size_t _lbuf_capacity(struct ne_linebuf *pbuf) 
{
	return (size_t)pbuf->buf_capacity;
}	//�õ�����


static __INLINE__ size_t _lbuf_datalen(struct ne_linebuf *pbuf)
{
	return (size_t)(pbuf->__end - pbuf->__start) ;
}

static __INLINE__ size_t _lbuf_free_capacity(struct ne_linebuf *pbuf) 
{
	return _lbuf_capacity(pbuf) - _lbuf_datalen(pbuf);
}	//�õ�����


static __INLINE__ void _lbuf_reset(struct ne_linebuf *pbuf)
{
	pbuf->__start = pbuf->__buf;
	pbuf->__end = pbuf->__buf ;	
}

static __INLINE__ void _lbuf_init(struct ne_linebuf *pbuf, size_t data_size) 
{
	pbuf->buf_capacity = data_size ;
	_lbuf_reset(pbuf);
}

static __INLINE__ void _lbuf_destroy(struct ne_linebuf *pbuf)
{}
static __INLINE__ void *_lbuf_data(struct ne_linebuf *pbuf)
{
	return (void*) pbuf->__start ;
}
static __INLINE__ void *_lbuf_tail(struct ne_linebuf *pbuf)
{
	return (void*) pbuf->__end ;
}
static __INLINE__ void *_lbuf_addr(struct ne_linebuf *pbuf)
{
	return (void*) pbuf->__end ;
}
static __INLINE__ void *_lbuf_raw_addr(struct ne_linebuf *pbuf)
{
	return (void*) pbuf->__buf ;
}

static __INLINE__ size_t _lbuf_freespace(struct ne_linebuf *pbuf)
{
	ne_assert(pbuf->__end <= pbuf->__buf+_lbuf_capacity(pbuf));
	return (size_t) (pbuf->__buf + _lbuf_capacity(pbuf) - pbuf->__end) ;
}
NE_COMMON_API void _lbuf_add_data(struct ne_linebuf *pbuf,size_t len);
//��д����
NE_COMMON_API NEINT32 _lbuf_read(struct ne_linebuf *pbuf,void *buf, size_t size,NEINT32 flag);
NE_COMMON_API NEINT32 _lbuf_write(struct ne_linebuf *pbuf,void *data, size_t datalen,NEINT32 flag);

NE_COMMON_API void _lbuf_sub_data(struct ne_linebuf *pbuf,size_t len);
//�������ƶ���buf head
NE_COMMON_API void _lbuf_move_ahead(struct ne_linebuf *pbuf) ;
NE_COMMON_API void _lbuf_tryto_move_ahead(struct ne_linebuf *pbuf) ;


//////////////////////////////////////////////////////////////////////////

/*
����һ�º��Ŀ����Ϊ���ܹ����������С�Ļ��塣
ֻҪ����һ�¸�ʽ����
struct ne_linebuf_xx
{
	struct line_buf_hdr  hdr ;
	NEINT8 __buf[xx] ;	
};

*/
#define nelbuf_capacity(pbuf)		_lbuf_capacity((struct ne_linebuf*) (pbuf))
#define nelbuf_datalen(pbuf)		_lbuf_datalen((struct ne_linebuf*) (pbuf))  
#define nelbuf_free_capacity(pbuf)	_lbuf_free_capacity((struct ne_linebuf*) (pbuf))
#define nelbuf_reset(pbuf)			_lbuf_reset((struct ne_linebuf*) (pbuf))
#define nelbuf_init(pbuf, size)		_lbuf_init((struct ne_linebuf*) (pbuf), size)
#define nelbuf_destroy(pbuf)			_lbuf_destroy((struct ne_linebuf*) (pbuf))
#define nelbuf_data(pbuf)			_lbuf_data((struct ne_linebuf*) (pbuf))

#define nelbuf_tail(pbuf)			_lbuf_tail((struct ne_linebuf*) (pbuf))

#define nelbuf_addr(pbuf)			_lbuf_addr((struct ne_linebuf*) (pbuf))
#define nelbuf_raw_addr(pbuf)		_lbuf_raw_addr((struct ne_linebuf*) (pbuf))
#define nelbuf_freespace(pbuf)		_lbuf_freespace((struct ne_linebuf*) (pbuf))
#define nelbuf_add_data(pbuf, len)	_lbuf_add_data((struct ne_linebuf*)((pbuf)), len)
#define nelbuf_read(pbuf, buf, size, flag )			_lbuf_read((struct ne_linebuf*) (pbuf),buf, size, flag)

#define nelbuf_write(pbuf,buf, size, flag)			_lbuf_write((struct ne_linebuf*) (pbuf),buf, size, flag)

#define nelbuf_sub_data(pbuf, len) _lbuf_sub_data((struct ne_linebuf*) (pbuf), len)

#define nelbuf_move_ahead(pbuf) _lbuf_move_ahead((struct ne_linebuf*) (pbuf))

#define nelbuf_tryto_move_ahead(pbuf) _lbuf_tryto_move_ahead((struct ne_linebuf*) (pbuf))

#endif 

