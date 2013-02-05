#include "ne_cliapp/ne_cliapp.h"
#include "ne_crypt/ne_crypt.h"
#include "ne_srvcore/ne_srvlib.h"
#include "ne_srvcore/client_map.h"
#include "ne_srvcore/ne_session.h"

CLI_ENTRY_DECLARE(_default_cli_msg_func) ;

NEINT32 __ser_version=-1;		//服务器版本
NEINT32 __pki_ok ;
static NEINT8 __rsa_digest[16] ;
NE_RSA_CONTEX __rsa_contex;
static tea_k	__crypt_key;
RSA_HANDLE cli_get_pubkey()
{
	if(__pki_ok)
		return &__rsa_contex;
	return NULL;
}

NEINT32 loging_message_handle(ne_handle nethandle,ne_usermsgbuf_t *recv_msg, ne_handle h_listen) 
{
	//NETMSG_T msgid, ,
	NEINT32 ret = 0 ;
	NEINT32 minid = NE_USERMSG_MINID(recv_msg) , maxid = NE_USERMSG_MAXID(recv_msg);
	NEUINT32 param = NE_USERMSG_PARAM(recv_msg) ;
	NEUINT8 *data = NE_USERMSG_DATA(recv_msg) ; 
	NEINT32 data_len = NE_USERMSG_LEN(recv_msg) - NE_USERMSG_HDRLEN;

	if(maxid != MAXID_START_SESSION){
		ne_logmsg("error message id error\n") ;
		return 0 ;
	}

	switch(minid){
	case SSM_GETVERSION :				//得到版本信息
		__ser_version = param ;
		break ;
	case SSM_GETPKI_DIGEST:				//得到公开密钥的摘要
		if(data_len==16) {
			MD5Crypt16((NEINT8*)&__rsa_contex.publicKey, sizeof(R_RSA_PUBLIC_KEY ) , __rsa_digest);
			if(0==MD5cmp(__rsa_digest,data)) {
				__pki_ok = 1 ;
			}
		}		
		break ;
	case SSM_GETPKI_KEY:				//得到公开密钥

		if(data_len==sizeof(R_RSA_PUBLIC_KEY)) {
			memcpy(&__rsa_contex.publicKey, data, data_len) ;
		}
		else {
			ret = 1 ;
		}
		break ;
	case SSM_EXCH_CRYPTKEY:				//密钥交换
		if(param) {
			ne_connector_set_crypt(nethandle, NULL, 0 ); 
		}
		else {
			//NEINT32 i;
			//neprintf( "send crypt key = %x,%x,%x,%x",
			//	__crypt_key.k[0],__crypt_key.k[1],__crypt_key.k[2],__crypt_key.k[3]) ;
			
			ne_connector_set_crypt(nethandle, &__crypt_key, sizeof(__crypt_key)) ;
		}
		break ;
	case SSM_LOGOUT:					//登出	
		neprintf(_NET("logout !\n")) ;
		ret = -1;
		break ;
	case SSM_PKI_ARITHMETIC:			//得到公开密钥加密算法
	case SSM_SYMMCRYPT_ARITHMETIC:		//得到对称加密算法
	case SSM_LOGIN_GATE:				//得到登陆入口地址(IP:port)
	case SSM_LOGIN_IN:					//登录(发送用户名和密码)
	default :
		ne_logwarn("received message error \n") ;
		
		//ret = _default_cli_msg_func( nethandle,msgid, param,data, data_len) ;
		break;
	}
	return ret ;
}

NEINT32 get_server_version(ne_handle connect_handle)
{
	NEINT32 ret;
	ne_usermsgbuf_t sendmsg;
	if(-1!=__ser_version)
		return __ser_version ;
	else {
//		sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;				
		ne_usermsghdr_init(&sendmsg.msg_hdr);	
		NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
		NE_USERMSG_MINID(&sendmsg) = SSM_GETVERSION ;

		//NETMSG_T msgid ;
		//ne_msgui_buf msg_buf;
		
		//msgid = MAKE_MSGID(MAXID_START_SESSION, SSM_GETVERSION) ;
		//create_netui_msg(&msg_buf, msgid, 0,  NULL, 0) ;
		
		ne_sessionmsg_sendex(connect_handle, &sendmsg, ESF_NORMAL) ;
		ret = ne_connector_waitmsg(connect_handle,(ne_packetbuf_t*) &sendmsg,1000*10);
		if(ret <= 0) {
			ne_logmsg("error on getting rsakey\n") ;
			return -1 ;
		}
		loging_message_handle(connect_handle, &sendmsg,NULL) ;
		//loging_message_handle(connect_handle, msg_buf.msgid,msg_buf.param,
		//		msg_buf._data, msg_buf.data_len);
	}
	return __ser_version;
}

NEINT32 start_encrypt(ne_handle connect_handle)
{
	NEINT32 ret = 0;
	NEINT32 len ;
	ne_usermsgbuf_t msg_buf;
	struct {
		NEINT8  cryptKeymd5[16] ;
		tea_k k ;
	} symm_key;
//	msg_buf.msg_hdr.packet_hdr = NE_PACKHDR_INIT;					
	ne_usermsghdr_init(&msg_buf.msg_hdr);	
//get rsa public key
	NE_USERMSG_MAXID(&msg_buf) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&msg_buf) = SSM_GETPKI_KEY ;
	//msgid = MAKE_MSGID(MAXID_START_SESSION, SSM_GETPKI_KEY) ;
	//create_netui_msg(&msg_buf, msgid, 0,  NULL, 0) ;

	ne_sessionmsg_sendex(connect_handle,&msg_buf, ESF_NORMAL) ;
	ret = ne_connector_waitmsg(connect_handle,(ne_packetbuf_t*) &msg_buf,1000*10);
	if(ret <= 0) {
		ne_logmsg("error on getting rsakey ret=%d\n" AND ret) ;
		return -1 ;
	}
	loging_message_handle(connect_handle, &msg_buf,NULL) ;
	//loging_message_handle(connect_handle, msg_buf.msgid,msg_buf.param,
	//			msg_buf._data, msg_buf.data_len);

	//get rsa md5 check (digest)
	NE_USERMSG_MAXID(&msg_buf) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&msg_buf) = SSM_GETPKI_DIGEST ;
	NE_USERMSG_PARAM(&msg_buf) = 0 ;
	NE_USERMSG_LEN(&msg_buf) = NE_USERMSG_HDRLEN ;
	//msgid = MAKE_MSGID(MAXID_START_SESSION, SSM_GETPKI_DIGEST) ;
	//create_netui_msg(&msg_buf, msgid, 0,  NULL, 0) ;

	ne_sessionmsg_sendex(connect_handle,&msg_buf, ESF_NORMAL) ;
	ret = ne_connector_waitmsg(connect_handle, (ne_packetbuf_t*)&msg_buf,1000*10);
	if(ret <= 0) {
		ne_logmsg("error on getting rsakey ret=%d\n"  AND  ret) ;
		return -1 ;
	}
	loging_message_handle(connect_handle, &msg_buf,NULL) ;
	//loging_message_handle(connect_handle, msg_buf.msgid,msg_buf.param,
	//			msg_buf._data, msg_buf.data_len);

	if(!__pki_ok){
		ne_logmsg("error on getting rsa digest\n") ;
		return -1 ;
	}
	
	//send symm encrypt key
	if(0!=tea_key(&__crypt_key) ){
		ne_logmsg("error on create crypt-key\n") ;
		return -1 ;
	}

	MD5Crypt16((NEINT8*)&__crypt_key,sizeof(__crypt_key),symm_key.cryptKeymd5) ;
	memcpy(&(symm_key.k), &__crypt_key, sizeof(__crypt_key)) ;

	//send symm crypt key
	if(0!=ne_RSAPublicEncrypt(NE_USERMSG_DATA(&msg_buf) , &len, (NEINT8*)&symm_key,sizeof(symm_key),&__rsa_contex )) {
		ne_logmsg("rsa encrypt error data error\n") ;
		return -1 ;
	}
	NE_USERMSG_LEN(&msg_buf) = len + NE_USERMSG_HDRLEN;
	NE_USERMSG_PARAM(&msg_buf) = 0 ;
	NE_USERMSG_MAXID(&msg_buf) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&msg_buf) = SSM_EXCH_CRYPTKEY ;
	//msg_buf.data_len = len ;

	//msg_buf.msgid = MAKE_MSGID(MAXID_START_SESSION, SSM_EXCH_CRYPTKEY);

	ne_sessionmsg_sendex(connect_handle,&msg_buf, ESF_NORMAL) ;
	ret = ne_connector_waitmsg(connect_handle, (ne_packetbuf_t*)&msg_buf,1000*20);
	if(ret <= 0) {
		ne_logmsg("sending crypt key error\n") ;
		return -1 ;
	}
	loging_message_handle(connect_handle, &msg_buf,NULL) ;
	//loging_message_handle(connect_handle, msg_buf.msgid,msg_buf.param,
	//			msg_buf._data, msg_buf.data_len);

	if(!ne_connector_check_crypt(connect_handle)){
		ne_logmsg("exchange PKI error\n") ;
		return -1 ;
	}
	return 0;
}

NEINT32 end_session(ne_handle connect_handle)
{
	NEINT32 ret = 0;
	ne_usermsgbuf_t msg_buf;
//	msg_buf.msg_hdr.packet_hdr = NE_PACKHDR_INIT;						
	ne_usermsghdr_init(&msg_buf.msg_hdr);	
	NE_USERMSG_MAXID(&msg_buf) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&msg_buf) = SSM_LOGOUT ;
	//msgid = MAKE_MSGID(MAXID_START_SESSION, SSM_GETPKI_KEY) ;
	//create_netui_msg(&msg_buf, msgid, 0,  NULL, 0) ;

	ne_sessionmsg_sendex(connect_handle,&msg_buf, ESF_NORMAL) ;
	//ret = ne_connector_waitmsg(connect_handle,(ne_packetbuf_t*) &msg_buf,1000*10);
	//ret = ne_connector_update(connect_handle,1000) ;
	//ne_assert(ret ==0) ;

	return ret ;
}
