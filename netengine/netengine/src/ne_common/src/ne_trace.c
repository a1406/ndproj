#include "ne_common/ne_os.h"
#include "ne_common/ne_dbg.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

static NEINT8 __log_filename[128] ;
extern NEINT8 *ne_process_name() ;
void set_log_file(NEINT8 *file_name) 
{
	strncpy(__log_filename, file_name,128) ;
}
NEINT8 *get_log_file() 
{
	if(__log_filename[0]) 
		return __log_filename ;
	else 
		return "nelog.log" ;
}

#define NE_LOG_FILE get_log_file()

nechar_t *strtowcs(NEINT8 *src, nechar_t *desc,NEINT32 len) ;

static const nechar_t *_dg_msg_str[] = 
{_NET("Common message: ") , _NET("Debug: "),_NET("Warn : " ), 
_NET("Error : "),_NET("Fatal error : ")} ;

static __INLINE__ const nechar_t *log_level_str(NEINT32 level)
{
	if((level) >=  NE_MSG && (level) <= NE_FATAL_ERR) {
		return _dg_msg_str[level]  ;
	}
	else {
		return _NET(" ") ;
	}
}	

/* log_msg mutex */
//static ne_mutex 		log_mutex  ;

//得到字符串形式的时间
nechar_t *ne_get_timestr(void)
{
	static nechar_t timebuf[64] ;
	time_t nowtm ;
	struct tm *gtm ;

	time(&nowtm) ;
	gtm = localtime( &nowtm );

	nesnprintf(timebuf, 64, _NET("%d:%d:%d"), gtm->tm_hour,
		gtm->tm_min,gtm->tm_sec) ;
	return timebuf ;
}
//得到字符串形式的日期
nechar_t *ne_get_datastr(void)
{
	static nechar_t datebuf[64] ;
	time_t nowtm ;
	struct tm *gtm ;

	time(&nowtm) ;
	gtm = localtime( &nowtm );

	nesnprintf(datebuf, 64, _NET("%d-%d-%d"), gtm->tm_year+1900,gtm->tm_mon+1,
		gtm->tm_mday) ;
	return datebuf ;
}
//得到字符串形式的时间和日期
nechar_t *ne_get_datatimestr(void)
{
	static nechar_t timebuf[64] ;
	time_t nowtm ;
	struct tm *gtm ;

	time(&nowtm) ;
	gtm = localtime( &nowtm );

	nesnprintf(timebuf, 64, _NET("%d-%d-%d %d:%d:%d"), 
		gtm->tm_year+1900,gtm->tm_mon+1,gtm->tm_mday,
		gtm->tm_hour,gtm->tm_min,gtm->tm_sec) ;
	return timebuf ;
}
NEINT32 _logmsg(NEINT8 *file, NEINT32 line, NEINT32 level, const nechar_t *stm,...) 
{
	nechar_t buf[1024*4] ;
	nechar_t *p = buf;
#ifdef NE_UNICODE
	nechar_t filebufs[128] ;
#endif
	va_list arg;
	NEINT32 done;
	FILE *log_fp = fopen(NE_LOG_FILE, "a");
	
	if(!log_fp) {
		return -1 ;
	}
#ifdef NE_UNICODE
	strtowcs(file,filebufs,128) ;
	nesnprintf(p, 4096,_NET("%s in %s [%s: file %s line %d ]"), 
			log_level_str(level), ne_process_name(), ne_get_datatimestr(),filebufs, line) ;
#else 
	nesnprintf(p, 4096,_NET("%s in %s [%s: file %s line %d ]"), 
			log_level_str(level),ne_process_name(),  ne_get_datatimestr(),file, line) ;
#endif

	p += nestrlen(p) ;
	
	va_start (arg, stm);
	done = nevsprintf (p, sizeof(buf),stm, arg);
	//done = _vsnprintf (p, sizeof(buf),stm, arg);
	va_end (arg);
	
	nefprintf(log_fp,_NET("%s\n"), buf) ;
	
	fclose(log_fp) ;
	printf_dbg(buf) ;
	//NETRAC(buf) ;
	//NETRAC("\n") ;
	return done ;
}

nechar_t *strtowcs(NEINT8 *src, nechar_t *desc, NEINT32 len)
{
	nechar_t *ret = desc ;
	while(*src && len-->0){
		if((NEUINT8)*src>0x7f){
			*desc++= *(nechar_t*)src++ ;	//ignore little-big endian
		}
		else
			*desc++ = *src++ ;
	}
	return ret ;
}

#ifdef NE_DEBUG

#ifdef _NE_COMMON_H_
#error not include ne_common.h
#endif
FILE *ne_fopen_dbg( const NEINT8 *filename, const NEINT8 *mode ,NEINT8 *file, NEINT32 line)
{
	FILE *fp = fopen(filename,mode) ;
	_source_log(fp, "fopen ", "file not closed", file, line) ;
	return fp;
}
void ne_fclose_dbg(FILE *fp)
{	
	ne_assert(fp) ;
	fclose(fp) ;
	ne_source_release(fp) ;
}

#endif
