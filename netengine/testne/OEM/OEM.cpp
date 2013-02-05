// OEM.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <assert.h>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
extern "C" {
#include "ne_common/ne_os.h"
#include "ne_quadtree/quadtree.h"
#include "demomsg.h"
}
#include "OEM.h"
#include "map.h"

extern "C" ne_connector_update(ne_handle net_handle, netime_t timeout) ;
extern "C" netime_t ne_time(void) ;
extern "C" NEINT32 ne_msgentry_install(ne_handle  handle, ne_usermsg_func, nemsgid_t maxid, nemsgid_t minid, NEINT32 level) ;
extern "C" void ne_fclose_dbg(FILE *fp);
extern "C" NEINT32 tcpnode_parse_recv_msgex(struct ne_tcp_node *node,NENET_MSGENTRY msg_entry , void *param) ;



#define MAX_LOADSTRING 100
// Global Variables:
HINSTANCE hInst;								// current instance
HWND g_wnd = NULL;
//struct nd_tcp_node conn_node;
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];
// The title bar text

extern MAP maps[20][16];
extern Player players[MAX_OTHER_USER];

#ifdef NE_DEBUG
#pragma comment(lib,"net.lib")
#pragma comment(lib,"crypt.lib")
#pragma comment(lib,"common.lib")
#pragma comment(lib,"cliapp.lib")
#else 
#pragma comment(lib,"net.lib")
#pragma comment(lib,"crypt.lib")
#pragma comment(lib,"common.lib")
#pragma comment(lib,"cliapp.lib")
#endif 

//nd_handle create_connect(char *ip, unsigned short port)
//{
//	nd_handle handle_net = nd_object_create("tcp") ;
//
//	if(!handle_net){		
//		nd_logerror("connect error :%s!" AND nd_last_error()) ;
//		return 0;
//	}
//
//	//set message handle	
//	nd_msgtable_create(handle_net, MSG_CLASS_NUM, MAXID_BASE) ;
//
//	if(-1 == nd_connector_openex( handle_net, ip, port) ) {
//		nd_logerror("connect error :%s!" AND nd_last_error()) ;
//		return 0;
//	}
//
//	return handle_net;
//}

int ClearAllPlayers()
{
	int i;
	for (i = 0; i < MAX_OTHER_USER; ++i) {
		players[i].rolename[0] = NULL;
	}
	return NULL;
}

ne_handle g_connect_handle = NULL;		
DWORD WINAPI netThread(LPVOID lpParameter)
{
	int ret;
	//ne_usermsgbuf_t sendmsg;
	//ne_usermsghdr_init(&sendmsg.msg_hdr);
	for (;;)
	{
		ret = ne_connector_update(g_connect_handle, 10*1000);
		//ret = ne_connector_waitmsg(g_connect_handle, (ne_packetbuf_t *)&sendmsg, 10*1000);

		if (ret < 0 || ret == 0 && g_connect_handle->myerrno != NEERR_SUCCESS) {
			neprintf(_NET("closed by remote ret = 0\n")) ;
			break;
		}
		//if(ret > 0) {
		//	ne_translate_message(g_connect_handle, (ne_packhdr_t*) &sendmsg) ;
		//	//run_cliemsg(connect_handle, &msg_buf) ;
		//	//msg_handle(connect_handle, msg_buf.msgid,msg_buf.param,
		//	//		msg_buf._data, msg_buf.data_len);
		//}
//		else if(-1==ret) {
//			neprintf(_NET("closed by remote ret = 0\n")) ;
//			break ;
//		}
//		else {
//			neprintf(_NET("wait time out ret = %d\npress any key to continue\n"), ret) ;
////			getch() ;
//			
//		}

	}
	return (0);
}

int user_scene_req(ne_handle handle);
DWORD WINAPI sceneThread(LPVOID lpParameter)
{
	for (;;)
	{
		if (g_connect_handle)
			user_scene_req(g_connect_handle);
		Sleep(5000);
	}
}

int login_server(ne_handle handle, char *username, char *password)
{
	//static __INLINE__ int nd_connectmsg_send(nd_handle  connector_handle, nd_usermsgbuf_t *msg ) 
	ne_usermsgbuf_t buf;
	ne_usermsghdr_init(&buf.msg_hdr);

	NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_LOGIN_REQ ;

	login_req_t *data = (login_req_t *)buf.data;
	strncpy((char *)data->username, username, sizeof(data->username));
	data->username[sizeof(data->username) - 1] = NULL;

	strncpy((char *)data->password, password, sizeof(data->password));
	data->password[sizeof(data->password) - 1] = NULL;

	NE_USERMSG_LEN(&buf) += sizeof(login_req_t);

	NE_USERMSG_PARAM(&buf) = ne_time() ;

	ne_connector_send(handle, (ne_packhdr_t *)&buf, ESF_URGENCY/*ESF_NORMAL*/ | ESF_ENCRYPT);
	return (0);
}
int user_action_req(ne_handle handle, act_t act, int x, int y)
{
	ne_usermsgbuf_t buf;
	ne_usermsghdr_init(&buf.msg_hdr);

	NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_ACTION_REQ;

    user_action_t *data = (user_action_t *)buf.data;
	memcpy(data->rolename, hr.rolename, sizeof(hr.rolename));
	data->x = x;
	data->y = y;
	memcpy(&data->action, &act, sizeof(act_t));
	NE_USERMSG_LEN(&buf) += sizeof(user_action_t);
	NE_USERMSG_PARAM(&buf) = ne_time() ;

	ne_connector_send(handle, (ne_packhdr_t *)&buf, ESF_URGENCY | ESF_ENCRYPT);
	return (0);
}

int user_scene_req(ne_handle handle)
{
	ne_usermsgbuf_t buf;
	ne_usermsghdr_init(&buf.msg_hdr);

	NE_USERMSG_MAXID(&buf) = MAXID_USER_SERVER ;
	NE_USERMSG_MINID(&buf) = MSG_USER_SCENE_REQ;

	NE_USERMSG_PARAM(&buf) = ne_time() ;

	ne_connector_send(handle, (ne_packhdr_t *)&buf, ESF_URGENCY | ESF_ENCRYPT);
	return (0);
}

int main(LPSTR command_line)
{
	int    argc;
	char** argv;
	char*  arg;
	int    index;
	int    result;

	// count the arguments
	argc = 1;
	arg  = command_line;

	while (arg[0] != 0) {
		while (arg[0] != 0 && arg[0] == ' ') {
			arg++;
		}
		if (arg[0] != 0) {
			argc++;
			while (arg[0] != 0 && arg[0] != ' ') {
				arg++;
			}
		}
	}    
	// tokenize the arguments
	argv = (char**)malloc(argc * sizeof(char*));
	arg = command_line;
	index = 1;

	while (arg[0] != 0) {
		while (arg[0] != 0 && arg[0] == ' ') {
			arg++;
		}

		if (arg[0] != 0) {
			argv[index] = arg;
			index++;

			while (arg[0] != 0 && arg[0] != ' ') {
				arg++;
			}

			if (arg[0] != 0) {
				arg[0] = 0;    
				arg++;
			}
		}
	}    

	// put the program name into argv[0]
	char filename[_MAX_PATH];
	GetModuleFileName(NULL, filename, _MAX_PATH);
	argv[0] = filename;

	result = ne_cliapp_init(argc, (NEINT8 **)argv);
	// call the user specified main function    
//	result = main(argc, argv);
	free(argv);
	return result;
}

static int msg_entry(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	assert(h_listen == NULL);
	return (0);
}

static int login_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_USER_LOGIN_ACK)
		return (-1);
	login_ack_t *data = (login_ack_t *)msg->data;
	if (data->result != 0)
	{
		;//todo
		return (0);
	}

	if (NE_USERMSG_DATALEN(msg) != sizeof(login_ack_t))
	{
		;//todo
		return (0);			
	}
	hr.Life = data->user_info.hp;
	hr.attack = data->user_info.attack;
	hr.recover = data->user_info.recover;
	hr.Money = data->user_info.money;
	hr.Experience = data->user_info.experience;
	hr.position.x = data->user_info.base.pos_x;
	hr.position.y = data->user_info.base.pos_y;	
	strncpy(hr.rolename, (char *)data->user_info.base.rolename, sizeof(hr.rolename));
	hr.rolename[sizeof(hr.rolename) - 1] = NULL;
	return (0);
}

static int action_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	int i;
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_USER_ACTION_ACK)
		return (-1);
	user_action_t *ack = (user_action_t *)msg->data;
	if (strcmp((char *)ack->rolename, (char *)hr.rolename) == 0) {
		memcpy(&hr.act, &ack->action, sizeof(hr.act));
		hr.position.x = ack->x;
		hr.position.y = ack->y;
		if (g_wnd)
			InvalidateRect(g_wnd, NULL, TRUE);
		return (0);
	} else {
		for (i = 0; i < MAX_OTHER_USER; ++i) {
			if (players[i].rolename[0] == NULL) {
					//todo insert a new player
				strncpy(players[i].rolename, (const char *)ack->rolename, sizeof(players[i].rolename));
				players[i].rolename[sizeof(players[i].rolename) - 1] = NULL;
				memcpy(&players[i].act, &ack->action, sizeof(players[i].act));
				players[i].position.x = ack->x;
				players[i].position.y = ack->y;
				break;
			}
			if (strcmp(players[i].rolename, (const char *)ack->rolename) == 0) {
					//todo update the player's info
				players[i].position.x = ack->x;
				players[i].position.y = ack->y;
				memcpy(&players[i].act, &ack->action, sizeof(players[i].act));
				break;
			}
		}
	}
	if (g_wnd)
		InvalidateRect(g_wnd, NULL, TRUE);
	return (0);
}

static int scene_ack(ne_handle connect_handle , ne_usermsgbuf_t *msg, ne_handle h_listen)
{
	int i;
	int index;
	user_scene_ack_t *t;
	assert(h_listen == NULL);
	if (msg->msg_hdr.maxid != MAXID_USER_SERVER || msg->msg_hdr.minid != MSG_USER_SCENE_ACK)
		return (-1);
	t = (user_scene_ack_t *)msg->data;
	ClearAllPlayers();
		//todo
	for (i = 0, index = 0; i < t->num; ++i)
	{
		if (strcmp((const char *)t->player[i].rolename, hr.rolename) == 0)
			continue;
		players[index].position.x = t->player[i].pos_x;
		players[index].position.y = t->player[i].pos_y;
		memcpy(players[index].rolename, t->player[i].rolename, NORMAL_SHORT_STR_LEN);
		//memcpy(&players[index].act, &t->player[i].action, sizeof(act_t));
		++index;
	}
	if (g_wnd)
		InvalidateRect(g_wnd, NULL, TRUE);
	return (0);
}

int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPSTR     lpCmdLine,
					 int       nCmdShow)
{
	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_OEM, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	//int ret = nd_cliapp_init(0, NULL);

	//nd_handle h_connect = create_connect("127.0.0.1", 9500);
	ClearAllPlayers();
	int ret = main(lpCmdLine);
	g_connect_handle = create_connector() ;
	if (!g_connect_handle)
	{
//		::AfxMessageBox("can not connect");
		goto nonet;
	}
	if(0 != start_encrypt(/*get_connect_handle()*/g_connect_handle) ) {
		;//todo
	}
	
	ne_msgentry_install(g_connect_handle, msg_entry, MAXID_SYS,SYM_BROADCAST,0);
	ne_msgentry_install(g_connect_handle, login_ack, MAXID_USER_SERVER, MSG_USER_LOGIN_ACK, 0);
	ne_msgentry_install(g_connect_handle, action_ack, MAXID_USER_SERVER, MSG_USER_ACTION_ACK, 0);
	ne_msgentry_install(g_connect_handle, scene_ack, MAXID_USER_SERVER, MSG_USER_SCENE_ACK, 0);	

	DWORD id;
	CreateThread(NULL, 0, netThread, NULL, 0, &id);
	CreateThread(NULL, 0, sceneThread, NULL, 0, &id);	

	login_server(g_connect_handle, "username", "testpwd");
nonet:
	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_OEM);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_OEM);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;//(LPCSTR)IDC_OEM;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, TEXT("Œ¸—™πÌ¡‘»À"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
		250, 150, 646, 512, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	g_wnd = hWnd;

	//todo
//nd_connector_set_crypt
	//int ret = create_rsa_key();
//	int len ,outlen, inlen;
//	char buf[128],en_buf[128], de_buf[128] ;
//		
//	ND_RSA_CONTEX  __rsa_contex ;
//
//	ndprintf("start test RSA \n input data\n") ;
//	
//	if(nd_RSAInit(&__rsa_contex))
//		abort()  ;
//
//
//	fgets( buf, 128, stdin );
//	
//	len = strlen(buf) ;
//
//	printf("input buf=%s\n",buf);
//	
//	if(0!=nd_RSAPublicEncrypt(en_buf, &outlen, buf, len, &__rsa_contex)) {
//		ndprintf("generate rsa encrypt error\n") ;
//		abort() ;
//	}
//	
//	if(0!=nd_RSAPrivateDecrypt(de_buf, &inlen, en_buf, outlen,&__rsa_contex)) {
//		ndprintf("generate rsa encrypt error\n") ;
//		abort() ;
//	}
///*
//	if(0!=nd_RSAPrivateEncrypt(en_buf, &outlen, de_buf, inlen, &__rsa_contex)) {
//		ndprintf("generate rsa encrypt error\n") ;
//		abort() ;
//	}
//	
//	if(0!=nd_RSAPublicDecrypt(de_buf, &inlen, en_buf, outlen,&__rsa_contex)) {
//		ndprintf("generate rsa encrypt error\n") ;
//		abort() ;
//	}
//
//*/
//	de_buf[len] = 0 ;
//	ndprintf("after encrypt and decrypt buf=%s\n", de_buf) ;
//	nd_RSAdestroy(&__rsa_contex);

/*
	nd_tcpnode_init(&conn_node);
	if(-1 == nd_tcpnode_connect("127.0.0.1", 12345, &conn_node)) {
		ndprintf(_NDT("%s %d: connect error :%s!"), __FUNC__, __LINE__, nd_last_error()) ;
	}
*/
	//FILE *fp;
	//fp=fopen((const NEINT8 *)"1.map", (const NEINT8 *)"r+");
	//fread(maps,sizeof(MAP),320,fp);
	//fclose(fp);

	MAP *p = &maps[0][0];
	for (int i = 0; i < sizeof(maps) / sizeof(maps[0][0]); ++i) {
		p[i].Expect = 0;
		p[i].Block = FALSE;
		p[i].Events = 0;
		p[i].Global = 0;
	}

	InitHeroEnemyInfo(hWnd);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	TCHAR szHello[MAX_LOADSTRING];
	LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);

	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
		HANDLE_MSG(hWnd,WM_PAINT, Main_OnPaint);
		HANDLE_MSG(hWnd,WM_KEYDOWN, Main_OnKey);
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}



// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
