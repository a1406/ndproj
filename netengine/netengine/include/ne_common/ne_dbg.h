#ifndef _NDDBG_H_
#define _NDDBG_H_

#include "ne_common/nechar.h"
typedef enum _error_ID{
	NE_MSG	= 0,			//正常信息
	NE_MSGDEBUG	,			//记录调试信息
	NE_WARN	,				//警告信息
	NE_ERROR ,				//错误信息
	NE_FATAL_ERR 			//严重错误
}edg_ID ;

#define NOT_SUPPORT_THIS_FUNCTION 0			//在assert中提示不支持的功能
NE_COMMON_API void set_log_file(NEINT8 *file_name)  ;
//log file operate
NE_COMMON_API NEINT32 _logmsg(NEINT8 *file, NEINT32 line, NEINT32 level, const nechar_t *stm,...) ;
NE_COMMON_API nechar_t *ne_get_timestr(void);			//得到字符串形式的时间
NE_COMMON_API nechar_t *ne_get_datastr(void);			//得到字符串形式的日期
NE_COMMON_API nechar_t *ne_get_datatimestr(void);		//得到字符串形式的时间和日期

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

//记录资源的使用情况
//在程序退出以后可以确定那些资源没有释放
// 
// ne_source_log 记录资源source 被函数operate函数获得 给出使用情况msg
// NEINT32 ne_sourcelog(void *source, NEINT8 *operate, NEINT8 *msg) ;
// 释放资源source
// it ne_source_release(void *source) ;
// 程序退出,dump 出未释放的资源,不需要手动释放
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
