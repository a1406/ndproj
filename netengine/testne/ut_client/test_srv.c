#include "ne_app/ne_app.h"


static int test_accept(NEUINT16 sessionid, SOCKADDR_IN *addr, ne_handle listener)
{
    printf("%s %d: sessonid[%hd] addr[%s] listener[]\n", __FUNCTION__, __LINE__, sessionid, (char *)(inet_ntoa(addr->sin_addr)));
    return (0);
}

static void test_deaccept(NEUINT16 sessionid, ne_handle listener)
{
    printf("%s %d: sessonid[%hd] listener[]\n", __FUNCTION__, __LINE__, sessionid);
}




int main(int argc, char *argv[])
{
    char ch;
    int ret;
    ne_handle listen_handle;
    
    ret = init_module();
    ret = create_rsa_key();
	listen_handle = ne_object_create("listen-ext") ;

	ne_listensrv_set_single_thread(listen_handle) ;
    ret = ne_listensrv_session_info(listen_handle, 1024, 0);

	NE_SET_ONCONNECT_ENTRY(listen_handle, test_accept, NULL, test_deaccept) ;
    
	ret = ne_listensrv_open(12345, listen_handle);

	while (1) {
		if(kbhit()) {
			ch = getch() ;
			if(NE_ESC==ch){
				printf_dbg("you are hit ESC, program eixt\n") ;
				break ;
			}
		}
		else {
			ne_sleep(1000) ;
		}
	}
    
    return 0;
}


