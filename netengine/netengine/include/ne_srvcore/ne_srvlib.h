#ifndef _NE_SRVLIB_H_
#define _NE_SRVLIB_H_

#if defined(WIN32) || defined(_WINDOWS) || defined(_WIN32)
	#if  defined(NE_SRVCORE_EXPORTS) 
	# define NE_SRV_API 				CPPAPI __declspec(dllexport)
	#else
	# define NE_SRV_API 				CPPAPI __declspec(dllimport)
	#endif
#else 
	# define NE_SRV_API 				CPPAPI
#endif 

#include "ne_srvcore/ne_threadsrv.h"
#include "ne_srvcore/ne_cmallocator.h"
#include "ne_srvcore/client_map.h"
#include "ne_srvcore/ne_session.h"
#include "ne_srvcore/ne_listensrv.h"
#include "ne_srvcore/ne_udtsrv.h"

NE_SRV_API NEINT32 ne_srvcore_init(void);
NE_SRV_API void ne_srvcore_destroy(void);
#ifdef WIN32
#endif
#endif
