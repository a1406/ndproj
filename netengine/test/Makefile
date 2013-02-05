TARGET = testmove
LIB_DIR = ../netengine/lib/

ALL_LIBS = -lne_app_dbg -lne_srvcore_dbg  -lne_net_dbg -lne_crypt_dbg -lne_common_dbg  -lne_quadtree_dbg

CFLAGS = -g -O0 -pg -D__LINUX__ -DSINGLE_THREAD_MOD -I../netengine/include/ -Iinclude  -lpthread -lrt -lm  -L../netengine/lib/

all: $(TARGET)

$(TARGET): testmove.c 
	gcc  $(CFLAGS) -o $@ $^  $(ALL_LIBS)	

clean:
	rm -rf *.o $(TARGET)