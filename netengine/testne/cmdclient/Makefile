TARGET = cmdclient

NELIBDIR = ../../netengine
LUA_CFLAGS = `lua-config --include --libs`
LIB_DIR = $(NELIBDIR)/lib/

ALL_LIBS = -lne_cliapp_dbg -lne_srvcore_dbg  -lne_net_dbg -lne_crypt_dbg -lne_common_dbg  `log4c-config --libs --cflags`

CFLAGS = -g -O0 -DNE_DEBUG -D__LINUX__ -I$(NELIBDIR)/include/ -I../include -lpthread -lrt -lm  -L$(NELIBDIR)/lib/ $(LUA_CFLAGS)

all: $(TARGET).c 
	gcc  $(CFLAGS) -o $(TARGET) $^  $(ALL_LIBS)

#gui: gui.c
#	gcc gui.c -o gui `pkg-config --cflags --libs gtk+-2.0`

clean:
	rm -rf $(TARGET) *.o