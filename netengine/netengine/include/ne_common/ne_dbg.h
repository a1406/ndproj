#ifndef _NDDBG_H_
#define _NDDBG_H_

#include "ne_common/nechar.h"
typedef enum _error_ID{
	NE_MSG	= 0,			//������Ϣ
	NE_MSGDEBUG	,			//��¼������Ϣ
	NE_WARN	,				//������Ϣ
	NE_ERROR ,				//������Ϣ
	NE_FATAL_ERR 			//���ش���
}edg_ID ;

#define NOT_SUPPORT_THIS_FUNCTION 0			//��assert����ʾ��֧�ֵĹ���
NE_COMMON_API void set_log_file(NEINT8 *file_name)  ;
//log file operate
NE_COMMON_API NEINT32 _logmsg(NEINT8 *file, NEINT32 line, NEINT32 level, const nechar_t *stm,...) ;
NE_COMMON_API nechar_t *ne_get_timestr(void);			//�õ��ַ�����ʽ��ʱ��
NE_COMMON_API nechar_t *ne_get_datastr(void);			//�õ��ַ�����ʽ������
NE_COMMON_API nechar_t *ne_get_datatimestr(void);		//�õ��ַ�����ʽ��ʱ�������

#define AND ,
#ifdef WIN32
#define ne_logmsg(msg) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_MSG ,## msg)
#define ne_logerror(msg) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_ERROR ,## msg)
#define ne_logfatal(msg) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_FATAL_ERR ,## msg)
#define ne_logwarn(msg) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_WARN , ##msg)
//#define ne_logdebug
#else 
#define ne_logmsg(fmt,arg...) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_MSG ,fmt, ##arg)
#define ne_logerror(fmt,arg...) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_ERROR ,fmt,##arg)
#define ne_logfatal(fmt,arg...) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_FATAL_ERR ,fmt,##arg)
#define ne_logwarn(fmt,arg...) _logmsg((NEINT8 *)__FILE__, __LINE__, NE_WARN ,fmt, ##arg)
#endif

//��¼��Դ��ʹ�����
//�ڳ����˳��Ժ����ȷ����Щ��Դû���ͷ�
// 
// ne_source_log ��¼��Դsource ������operate������� ����ʹ�����msg
// NEINT32 ne_sourcelog(void *source, NEINT8 *operate, NEINT8 *msg) ;
// �ͷ���Դsource
// it ne_source_release(void *source) ;
// �����˳�,dump ��δ�ͷŵ���Դ,����Ҫ�ֶ��ͷ�
// void ne_sourcelog_dump() ; 

#ifdef NE_DEBUG
#define ne_logdebug(msg) _logmsg(__FILE__, __LINE__, NE_MSGDEBUG , ##msg)

#define NE_RUN_HERE()	neprintf("[%s : %d] run here...\n" AND __FILE__, __LINE__)
//Log source 
NE_COMMON_API NEINT32 ne_sourcelog_init() ;
NE_COMMON_API NEINT32 _source_log(void *p, NEINT8 *operate, NEINT8 *msg, NEINT8 *file, NEINT32 line) ;
NE_COMMON_API NEINT32 _source_release(void *source) ;
CPPAPI void ne_sourcelog_dump() ;
#define ne_sourcelog(source, operatename,msg) \
	_source_log(source,operatename,msg,__FILE__,__LINE__)
#define ne_source_release(source) 	_source_release(source) 
//define file open
NE_COMMON_API FILE *ne_fopen_dbg( const NEINT8 *filename, const NEINT8 *mode, NEINT8 *file, NEINT32 line);
NE_COMMON_API void ne_fclose_dbg(FILE *fp);

#define printf_dbg neprintf
#else 
#define ne_logdebug(msg) (void) 0
#define ne_sourcelog(source, operatename,msg) (void)0
#define ne_source_release(source) (void)0
#define ne_sourcelog_dump()	  (void)0
#define ne_sourcelog_init()	  (void)0
#define printf_dbg //

#define NE_RUN_HERE()	//
//#define ne_
#endif	//NE_DEBUG


#endif
