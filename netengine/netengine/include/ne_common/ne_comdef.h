#ifndef _NE_COMDEF_H_
#define _NE_COMDEF_H_

enum ENE_ERROR_TYPE
{
	NEERR_SUCCESS = 0 ,	//正确
	NEERR_TIMEOUT   ,		//超时
	NEERR_NOSOURCE ,		//没有足够资源
	NEERR_OPENFILE,			//不能打开文件
	NEERR_BADTHREAD,		//不能打开线程
	NEERR_LIMITED,			//资源超过上限
	NEERR_USER,				//处理用户数据出错(消息回调函数返回-1
	NEERR_INVALID_INPUT ,	//无效的输入(DATA IS TO BIG OR ZERO
	NEERR_IO		,		//IO bad SYSTEM IO BAD
	NEERR_WUOLD_BLOCK ,		//需要阻塞	
	NEERR_CLOSED,			//socket closed by peer
	NEERR_BADPACKET  ,		//网络输入数据错误(too long or short)
	NEERR_BADSOCKET ,		//无效的socket
	
	NEERR_UNKNOW			//unknowwing error
};

//定义句柄类型
enum ENE_OBJECT_TYPE
{
	NEHANDLE_UNKNOW =0,
	NEHANDLE_MMPOOL , 
	NEHANDLE_TCPNODE , 
	NEHANDLE_UDTNODE , 
	NEHANDLE_TCPSRV ,
	NEHANDLE_UDTSRV , 
	NEHANDLE_CMALLOCATOR ,
	NEHANDLE_NUMBERS
};

#endif 
