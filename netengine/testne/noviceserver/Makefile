TARGET = noviceserver

NELIBDIR = ../../netengine
LUA_CFLAGS = `lua-config --include --libs`
LIB_DIR = -L$(NELIBDIR)/lib/ -L$(LUASRCDIR)

ALL_LIBS =  -lne_app_dbg -lne_srvcore_dbg  -lne_net_dbg -lne_crypt_dbg -lne_common_dbg -lne_quadtree_dbg

CFLAGS = -g -O0 -DNE_DEBUG -D__LINUX__ -DSINGLE_THREAD_MOD  -I$(NELIBDIR)/include -I../include  -lpthread -lrt -lm  -L$(NELIBDIR)/lib/  $(LUA_CFLAGS) -I../../../OurRPG

all: $(TARGET).c 
	gcc  $(CFLAGS) `log4c-config --libs --cflags` -o $(TARGET) $(TARGET).c  $(ALL_LIBS)

clean:
	rm -rf $(TARGET) *.o