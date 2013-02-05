#include <gtk/gtk.h>
#include "ne_app/ne_app.h"

//int stream_data_entry(void *node,ne_packhdr_t *msg,void *param)
struct ne_tcp_node g_conn_node ;

int msg_entry(ne_handle  handle, ne_usermsgbuf_t *msg , ne_handle listener)
{
	char *buf;
	neprintf(_NET("received message %d "), NE_USERMSG_MAXID(msg));
	if(NE_USERMSG_LEN(msg)>0) {
		buf = (char *)msg + sizeof(*msg);
			//NENET_DATA(msg) + NENET_DATALEN(msg) = 0 ; 
		//msg->buf[NENET_DATALEN(msg)] = NULL ;
		//neprintf(_NET("%s\n"),NENET_DATA(msg));
	}
	return 0 ;
}
int do_client_msg(struct ne_tcp_node *conn, void *param)
{
		
	int ret;
	ne_usermsgbuf_t msg_recv;

	ne_assert(conn) ;
	ne_assert(check_connect_valid(conn) );

	memset(&msg_recv,0,sizeof(msg_recv)) ;
	ret = ne_connector_waitmsg((ne_handle)conn, (ne_packetbuf_t *)&msg_recv,10000);
	if(ret > 0) {			
		//msg_entry(connect_handle, &msg_recv) ;
		ne_translate_message((ne_handle)conn, (ne_packhdr_t*)&msg_recv) ;
	}		

	else if(-1==ret) {
		neprintf(_NET("closed by remote ret = 0\n")) ;
		return -1 ;
	}
	else {
		neprintf(_NET("wait time out ret = %d\npress any key to continue\n"), ret) ;
		return -1;		
	}

	return ret ;
}

int test_send(char *host, int port)
{

	int i ;
//	struct ndnet_msg msg_buf;
	ne_usermsgbuf_t msg_buf;
	char *paddr ;
	
//  conn_node.msg_entry = msg_entry;
//  i=atoi(argv[2]) ;
	if(-1 == ne_tcpnode_connect(host, port,&g_conn_node )){
//	    if(-1==ne_tcpnode_connect("localhost", 7828,&conn_node )){
		neprintf(_NET("connect error :%s!"), ne_last_error()) ;
		getch();
		exit(1);
	}

	ne_socket_nonblock(g_conn_node.fd,0);
//	_init_ndmsg_buf(&msg_buf);
//	NDNET_MSGID(&msg_buf) = 1 ;

	NE_USERMSG_MAXID(&msg_buf) = MAXID_SYS ;
	NE_USERMSG_MINID(&msg_buf) = SYM_ECHO ;
	paddr = NE_USERMSG_DATA(&msg_buf) ;


	
	for (i=0 ; i<100 ; i++){
		nesprintf(paddr, _NET("send %d time =%lu"), i, ne_time()) ;
//		NENET_DATALEN(&msg_buf) = nestrlen(NENET_DATA(&msg_buf)) ;
		NE_USERMSG_LEN(&msg_buf) = NE_USERMSG_HDRLEN + nestrlen(paddr)+1 ;
		NE_USERMSG_PARAM(&msg_buf) = 3 ;

		ne_tcpnode_send(&g_conn_node,(ne_packhdr_t *)&msg_buf,ESF_URGENCY) ;
		do_client_msg(&g_conn_node,NULL);
	}
	return 0;
}

GtkWidget *image;
GdkPixbuf *pixbuf[2];
GtkWidget *fixed;
int x, y;

gboolean timer_func(gpointer data)
{
	static int index = 0;
	++index;
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf[index % 2]);
	if (index > 300)
		index = 0;
	return NETRUE;
}

gboolean conf_event(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data) 
{
	g_print("type[%d] x[%d] y[%d] width[%d] height[%d]\n", 
		   event->type, event->x, event->y, event->width, event->height);

//	if (event->width > 400)
//		gtk_window_resize(GTK_WINDOW(widget), 400, 300);

	return FALSE;
}

gboolean keypress_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	switch (event->keyval)
	{
	case 65361://left
	case 97://a
		x -= 2;
		gtk_fixed_move(GTK_FIXED(fixed), image, x, y);
		break;
	case 65362://up
	case 119://w
		y -= 2;
		gtk_fixed_move(GTK_FIXED(fixed), image, x, y);
		break;
	case 65363://right
	case 100://d
		x += 2;
		gtk_fixed_move(GTK_FIXED(fixed), image, x, y);
		break;
	case 65364://down
	case 115://s
		y += 2;
		gtk_fixed_move(GTK_FIXED(fixed), image, x, y);
		break;
	}

//	g_print("keyval[%d]\n", event->keyval);
	return FALSE;
}

static int init_gui(char **argv)
{
	GtkWidget *window;
	GError *error = NULL;

	int argc = 1;
    gtk_init (&argc, &argv);
  
	pixbuf[0] = gdk_pixbuf_new_from_file("rc/chinababy001.png", &error);
	if (!pixbuf[0]) {
		g_print("pixbuf NULL, err is %s\n", error->message);
		return (0);
	}
	pixbuf[1] = gdk_pixbuf_new_from_file("rc/chinababy002.png", &error);
	if (!pixbuf[1]) {
		g_print("pixbuf NULL, err is %s\n", error->message);
		return (0);
	}

	image = gtk_image_new_from_pixbuf(pixbuf[0]);
//	gtk_window_set_position(GTK_WINDOW(image), GTK_WIN_POS_NONE);
//	gtk_window_move(GTK_WINDOW(image), 10, 20);
//    g_signal_connect (G_OBJECT(image), "configure-event",
//		      G_CALLBACK(conf_event), NULL);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_resize(GTK_WINDOW(window), 400, 300);
    g_signal_connect (G_OBJECT(window), "configure-event",
					  G_CALLBACK(conf_event), NULL);
	g_signal_connect (G_OBJECT(window), "key-press-event",
					  G_CALLBACK(keypress_event), NULL);


	fixed = gtk_fixed_new();
	gtk_fixed_put(GTK_FIXED(fixed), image, 0, 0);
	gtk_container_add(GTK_CONTAINER(window), fixed);

	gtk_timeout_add(100, timer_func, NULL);

    g_signal_connect (G_OBJECT(window), "destroy",
		      G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
	return (0);
}

int main(int argc, char *argv[] )
{
    char ch;
    int ret;

	if(argc != 3) {
		neprintf("usage: %s host port\n",argv[0]) ;
		getch();
		exit(1) ;
	}

	init_gui(argv);
    ret = init_module();
    ret = create_rsa_key();

	ne_tcpnode_init(&g_conn_node) ;
	ne_msgentry_install((ne_handle)&g_conn_node,msg_entry,MAXID_SYS,SYM_ECHO,0) ;	
	test_send(argv[1],atoi(argv[2])) ;	
    
    gtk_main ();
	destroy_module();	
	destroy_rsa_key();
    
    return 0;
}
