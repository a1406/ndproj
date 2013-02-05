TARGET = gui

NDLIBDIR = ../../ndnetengine-read-only/
LIB_DIR = $(NDLIBDIR)/lib/

ALL_LIBS =  -lnd_app_dbg -lnd_srvcore_dbg  -lnd_net_dbg -lnd_crypt_dbg -lnd_common_dbg  

#ALL_LIBS = $(LIB_DIR)/libnd_app_dbg.a \
           $(LIB_DIR)/libnd_cliapp_dbg.a \
           $(LIB_DIR)/libnd_common_dbg.a \
           $(LIB_DIR)/libnd_crypt_dbg.a \
           $(LIB_DIR)/libnd_srvcore_dbg.a \
           $(LIB_DIR)/libnd_net_dbg.a 

CFLAGS = -g -O0 -DND_DEBUG -D__LINUX__ -I$(NDLIBDIR)/include/  -lpthread -lrt -lm  -L$(NDLIBDIR)/lib/ `pkg-config --cflags --libs gtk+-2.0`

all: $(TARGET).c 
	gcc  $(CFLAGS) -o $(TARGET) $(TARGET).c  $(ALL_LIBS)

#gui: gui.c
#	gcc gui.c -o gui `pkg-config --cflags --libs gtk+-2.0`

clean:
	rm -rf $(TARGET) *.o