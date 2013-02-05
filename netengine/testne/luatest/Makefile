TARGET = luatest

LUA_PATH = ./lua51/lua5.1-5.1.4
LUAJIT_PATH = ./luajit-2.0.0~beta2+dfsg2
LUA_CFLAGS = -I$(LUA_PATH)/src -L$(LUA_PATH)/src/ 
LUA_JITCFLAGS = -I$(LUAJIT_PATH)/src -L$(LUAJIT_PATH)/src/ 
# -llua5.1
ALL_LIBS = `log4c-config --libs --cflags` $(LUA_PATH)/src/liblua.a
ALL_JITLIBS = `log4c-config --libs --cflags` $(LUAJIT_PATH)/src/libluajit.a

CFLAGS = -g -O0 -D__LINUX__  -lpthread -lrt -lm  $(LUA_CFLAGS)
CJITFLAGS = -g -O0 -D__LINUX__  -lpthread -lrt -lm -DLUA_JIT $(LUA_JITCFLAGS)

all: $(TARGET).c 
	gcc  $(CFLAGS) -o $(TARGET) $^  $(ALL_LIBS)  -pg

jit: $(TARGET).c 
	gcc  $(CJITFLAGS) -o $(TARGET) $^  $(ALL_JITLIBS) -pg

clean:
	rm -rf $(TARGET) *.o