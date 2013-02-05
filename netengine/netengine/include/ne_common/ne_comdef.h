#ifndef _NE_COMDEF_H_
#define _NE_COMDEF_H_

enum ENE_ERROR_TYPE
{
	NEERR_SUCCESS = 0 ,	//��ȷ
	NEERR_TIMEOUT   ,		//��ʱ
	NEERR_NOSOURCE ,		//û���㹻��Դ
	NEERR_OPENFILE,			//���ܴ��ļ�
	NEERR_BADTHREAD,		//���ܴ��߳�
	NEERR_LIMITED,			//��Դ��������
	NEERR_USER,				//�����û����ݳ���(��Ϣ�ص���������-1
	NEERR_INVALID_INPUT ,	//��Ч������(DATA IS TO BIG OR ZERO
	NEERR_IO		,		//IO bad SYSTEM IO BAD
	NEERR_WUOLD_BLOCK ,		//��Ҫ����	
	NEERR_CLOSED,			//socket closed by peer
	NEERR_BADPACKET  ,		//�����������ݴ���(too long or short)
	NEERR_BADSOCKET ,		//��Ч��socket
	
	NEERR_UNKNOW			//unknowwing error
};

//����������
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
