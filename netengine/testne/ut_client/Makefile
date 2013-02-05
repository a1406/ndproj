LIB_DIR = ../ndnetengine-read-only/lib/

ALL_LIBS = -lnd_app_dbg -lnd_srvcore_dbg  -lnd_net_dbg -lnd_crypt_dbg -lnd_common_dbg  

#ALL_LIBS = $(LIB_DIR)/libnd_app_dbg.a \
           $(LIB_DIR)/libnd_cliapp_dbg.a \
           $(LIB_DIR)/libnd_common_dbg.a \
           $(LIB_DIR)/libnd_crypt_dbg.a \
           $(LIB_DIR)/libnd_srvcore_dbg.a \
           $(LIB_DIR)/libnd_net_dbg.a 

CFLAGS = -g -O0 -D__LINUX__ -I../../ndnetengine-read-only/include/ -I../include/  -lpthread -lrt -lm  -L../../ndnetengine-read-only/lib/

all: test_srv.c 
	gcc  $(CFLAGS) -o test_srv test_srv.c  $(ALL_LIBS)

clean:
	rm test_srv

