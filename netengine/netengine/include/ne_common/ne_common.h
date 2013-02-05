#ifndef _NE_COMMON_H_
#define _NE_COMMON_H_

#include "ne_common/nechar.h"

#define NE_BUFSIZE 4096


#include "ne_common/ne_comdef.h"

#include "ne_common/ne_os.h"

#include "ne_common/ne_dbg.h"

#include "ne_common/ne_handle.h"

#include "ne_common/ne_mempool.h"

#include "ne_common/ne_timer.h"

#include "ne_common/ne_recbuf.h"

#include "ne_common/list.h"

#include "ne_common/ne_bintree.h"

#include "ne_common/ne_str.h"

#include "ne_common/ne_xml.h"

#include "ne_common/ne_atomic.h"

typedef NEUINT32 netime_t ;	//Ê±¼ä 1/1000 s

NE_COMMON_API netime_t ne_time(void) ;
NE_COMMON_API NEINT32 ne_common_init();
NE_COMMON_API void ne_common_release();

NE_COMMON_API NEINT8 *ne_process_name() ;
NE_COMMON_API NEINT32 ne_arg(NEINT32 argc, NEINT8 *argv[]);
#ifdef NE_DEBUG
#undef  fopen
#undef  fclose
#define fopen(filename, mod) ne_fopen_dbg(filename, mod, (NEINT8 *)__FILE__, __LINE__)
#define fclose(fp)			ne_fclose_dbg(fp)
#endif
#endif 
