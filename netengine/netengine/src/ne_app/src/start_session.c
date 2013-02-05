#include "ne_app.h"
#include "ne_crypt/ne_crypt.h"
#include "ne_net/ne_netlib.h"


extern NE_RSA_CONTEX  __rsa_contex ; 
/*
 *ʵ��������Щ���ܺ���
	SSM_GETVERSION = 0,				//�õ��汾��Ϣ
	SSM_GETPKI_DIGEST,				//�õ�������Կ��ժҪ
	SSM_GETPKI_KEY,					//�õ�������Կ
	SSM_EXCH_CRYPTKEY,				//��Կ����
	SSM_PKI_ARITHMETIC,				//�õ�������Կ�����㷨
	SSM_SYMMCRYPT_ARITHMETIC,		//�õ��ԳƼ����㷨
	SSM_LOGIN_GATE,					//�õ���½��ڵ�ַ(IP:port)
	SSM_LOGIN_IN,					//��¼(�����û���������)
	SSM_LOGOUT						//�ǳ�			

  */
//�õ��汾��Ϣ
MSG_ENTRY_INSTANCE(ss_get_version)
{
	ne_usermsgbuf_t sendmsg;

//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_GETVERSION ;
	NE_USERMSG_PARAM(&sendmsg) = NE_CUR_VERSION ;

	//ne_sessionmsg_send(nethandle, &sendmsg) ;
	NE_MSG_SEND(nethandle,&sendmsg,h_listen) ;
	return 0 ;
}

//�õ�������Կ��ժҪ
MSG_ENTRY_INSTANCE(ss_pki_digest)
{
	ne_usermsgbuf_t sendmsg;
//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;	
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_GETPKI_DIGEST ;
	memcpy(NE_USERMSG_DATA(&sendmsg), get_pubkey_digest(), 16) ;

	NE_USERMSG_LEN(&sendmsg) += 16 ;
	//ne_sessionmsg_send(nethandle, &sendmsg) ;
	
	NE_MSG_SEND(nethandle,&sendmsg,h_listen) ;
	return 0 ;

}

//�õ�������Կ
MSG_ENTRY_INSTANCE(ss_public_key)
{
	ne_usermsgbuf_t sendmsg;
//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;		
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_GETPKI_KEY ;
	memcpy(NE_USERMSG_DATA(&sendmsg),(NEUINT8*)&__rsa_contex.publicKey,sizeof(R_RSA_PUBLIC_KEY )) ;

	NE_USERMSG_LEN(&sendmsg) += sizeof(R_RSA_PUBLIC_KEY ) ;
	//ne_sessionmsg_send(nethandle, &sendmsg) ;
	
	NE_MSG_SEND(nethandle,&sendmsg,h_listen) ;
	return 0 ;

}

//��Կ����
MSG_ENTRY_INSTANCE(ss_crypt_key)
{
	ne_usermsgbuf_t sendmsg;
	NEINT32 data_len = NE_USERMSG_LEN(msg) - NE_USERMSG_HDRLEN;
	NEINT32 keylen ;
	NEINT8 *data = NE_USERMSG_DATA(msg) ;
	struct {
		NEINT8  cryptKeymd5[16] ;
		tea_k k ;
		NEINT8 _tmp[16] ;
	} symm_key;
	
//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;		
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_EXCH_CRYPTKEY  ;
	NE_USERMSG_PARAM(&sendmsg) = 1 ;
	//ne_msgui_buf msg_buf ;
		
	//create_netui_msg(&msg_buf,msgid,1, NULL,0) ;

	if(data_len > 512 || data_len<(sizeof(symm_key)-16)) {
		goto C_ERROR ;
	}
	else if(0==ne_RSAPrivateDecrypt((NEINT8*)&symm_key, &keylen, data, data_len, &__rsa_contex)) {	
		NEINT8 keymd5[16] ;
		MD5Crypt16((NEINT8*)&symm_key.k,sizeof(tea_k), keymd5) ;
		if(MD5cmp(keymd5, symm_key.cryptKeymd5)) {
			goto C_ERROR ;
		}
		SET_CRYPT(nethandle, &symm_key.k, sizeof(tea_k),h_listen) ;
		NE_USERMSG_PARAM(&sendmsg) = 0 ;
		//NE_USERMSG_LEN(&sendmsg) = NE_USERMSG_HDRLEN ;
		//create_netui_msg(&msg_buf,msgid,0, NULL,0) ;
	}
C_ERROR:
	NE_MSG_SEND(nethandle,&sendmsg,h_listen);
	
	return 0;
}

//�õ�������Կ�����㷨
MSG_ENTRY_INSTANCE(ss_public_crypt_arith)
{
	ne_usermsgbuf_t sendmsg;
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_PKI_ARITHMETIC ;
	memcpy(NE_USERMSG_DATA(&sendmsg),"RSA",4) ;

	NE_USERMSG_LEN(&sendmsg) += 4 ;
	NE_MSG_SEND(nethandle, &sendmsg,h_listen) ;
	return 0 ;
}

//�õ��ԳƼ����㷨
MSG_ENTRY_INSTANCE(ss_symm_crypt_arith)
{
	ne_usermsgbuf_t sendmsg;
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_SYMMCRYPT_ARITHMETIC ;
	memcpy(NE_USERMSG_DATA(&sendmsg),"TEA",4) ;

	NE_USERMSG_LEN(&sendmsg) += 4;
	NE_MSG_SEND(nethandle, &sendmsg,h_listen) ;
	return 0 ;

}

//�õ���½��ڵ�ַ(IP:port)
MSG_ENTRY_INSTANCE(ss_netgate)
{
	ne_usermsgbuf_t sendmsg;
//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_LOGIN_GATE ;
	NE_USERMSG_PARAM(&sendmsg) = 0 ;
	NE_MSG_SEND(nethandle, &sendmsg,h_listen) ;
	return 0 ;
}

//��¼(�����û���������)
MSG_ENTRY_INSTANCE(ss_login)
{
	ne_usermsgbuf_t sendmsg;
//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;	
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_LOGIN_IN ;
	NE_USERMSG_PARAM(&sendmsg) = 1 ;
	NE_MSG_SEND(nethandle, &sendmsg,h_listen) ;
	return 0 ;
}

//�ǳ�
MSG_ENTRY_INSTANCE(ss_logout)
{
	ne_usermsgbuf_t sendmsg;
//	sendmsg.msg_hdr.packet_hdr = NE_PACKHDR_INIT;		
	ne_usermsghdr_init(&sendmsg.msg_hdr);	
	NE_USERMSG_MAXID(&sendmsg) = MAXID_START_SESSION ;
	NE_USERMSG_MINID(&sendmsg) = SSM_LOGOUT ;
	NE_USERMSG_PARAM(&sendmsg) = 1 ;
	NE_MSG_SEND(nethandle, &sendmsg,h_listen) ;
	
	neprintf(_NET("received close message, client closed by server\n")) ;
	NE_CLOSE(nethandle, 0,h_listen) ;
	//NE_CLOSE
//#error in here ,need to close and send fin packet
	return 0 ;
}

void install_start_session(ne_handle listen_handle)
{		
	//�õ��汾��Ϣ
	NE_INSTALL_HANDLER(listen_handle,ss_get_version,MAXID_START_SESSION,SSM_GETVERSION,EPL_CONNECT);

	//�õ�������Կ��ժҪ
	NE_INSTALL_HANDLER(listen_handle,ss_pki_digest,MAXID_START_SESSION,SSM_GETPKI_DIGEST,EPL_CONNECT);
	//�õ�������Կ
	NE_INSTALL_HANDLER(listen_handle,ss_public_key,MAXID_START_SESSION,SSM_GETPKI_KEY,EPL_CONNECT);
	//��Կ����
	NE_INSTALL_HANDLER(listen_handle,ss_crypt_key,MAXID_START_SESSION,SSM_EXCH_CRYPTKEY,EPL_CONNECT);
	//�õ�������Կ�����㷨
	NE_INSTALL_HANDLER(listen_handle,ss_public_crypt_arith,MAXID_START_SESSION,SSM_PKI_ARITHMETIC,EPL_CONNECT);

	//�õ��ԳƼ����㷨
	NE_INSTALL_HANDLER(listen_handle,ss_symm_crypt_arith,MAXID_START_SESSION,SSM_SYMMCRYPT_ARITHMETIC,EPL_CONNECT);
	//�õ���½��ڵ�ַ(IP:port)
	NE_INSTALL_HANDLER(listen_handle,ss_netgate,MAXID_START_SESSION,SSM_LOGIN_GATE,EPL_CONNECT);

	//��¼(�����û���������)
	NE_INSTALL_HANDLER(listen_handle,ss_login,MAXID_START_SESSION,SSM_LOGIN_IN,EPL_CONNECT);
	//�ǳ�		
	NE_INSTALL_HANDLER(listen_handle,ss_logout,MAXID_START_SESSION,SSM_LOGOUT,EPL_CONNECT);
}
