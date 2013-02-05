#ifndef _NE_NETLIB_H_
#define _NE_NETLIB_H_

#include "ne_common/ne_common.h"

#if defined(WIN32) || defined(_WINDOWS) || defined(_WIN32)
	#if  defined(NE_NET_EXPORTS) 
	# define NE_NET_API 				CPPAPI __declspec(dllexport)
	#else
	# define NE_NET_API 				CPPAPI __declspec(dllimport)
	#endif
#else 
	# define NE_NET_API 				CPPAPI
#endif 

#include "ne_net/ne_sock.h"
#include "ne_net/ne_tcp.h"
#include "ne_net/ne_srv.h"
#include "ne_net/ne_netui.h"
#include "ne_net/ne_netcrypt.h"
#include "ne_net/ne_iphdr.h"
#include "ne_net/ne_msgentry.h"
#include "ne_net/ne_netpack.h"
#include "ne_net/ne_udt.h"
#include "ne_net/ne_udtsrv.h"

NE_NET_API NEINT32 ne_net_init(void);

NE_NET_API void ne_net_destroy(void);
#endif
