#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#ifdef LUA_JIT
#include "lj_obj.h"
#else
#include "lstate.h"
#endif
#include <stdio.h>

static void dump_stack(lua_State *l)
{
	int i, top, type;	
	top = lua_gettop(l);
	printf("==== dump stack[%d] =====\n", top);
//	printf("base[%p] top[%p]\n", (l->base), l->top);
	for (i = 0; i <= top; ++i) {
		type = lua_type(l, i);
		if (type == 6)
			printf("%d %d: %s, %p\n", i, type, lua_typename(l, type), lua_tocfunction(l, i));
		else
			printf("%d %d: %s\n", i, type, lua_typename(l, type));			
	}
	printf("==== dump end =====\n");	
}
#define api_incr_top(L)   {api_check(L, L->top < L->ci->top); L->top++;}
#define incr_top(L) \
  (++L->top >= L->maxstack && (lj_state_growstack1(L), 0))

static lua_State *L = NULL;
static int lua_init(char *filename)
{
	TValue *o, func;
	L = lua_open();
    luaopen_base(L);
//    luaopen_io(L);
    luaopen_string(L);
    luaopen_math(L);

//	lua_pop(L, 4);
	
	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) {
		error(L, "cannot run configuration file: %s", lua_tostring(L, -1));
		return (-1);
	}
//	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_getglobal(L, "lua_func");
	lua_pushstring(L, "tangpeilei");

	o = (TValue *)index2adr(L, -2);
#ifdef LUA_JIT
	if (isluafunc(funcV(o))) {
		copyTV(L, &func, o);
	} else
		assert(0);
#else
	if (isLfunction(o)) {
//		setclvalue(L, &func, o->value.gc);
		setobj(L, &func, o);
	} else
		assert(0);
#endif
	lua_pcall(L, 1, 0, 0);
	lua_gc(L, LUA_GCCOLLECT, 0);
#ifdef LUA_JIT
	copyTV(L, L->top, &func);
	incr_top(L);
#else
	lua_lock(L);
	setobj(L, L->top, &func);
	api_incr_top(L);
//	luaD_call(L, &func, 0);
	lua_unlock(L);
#endif	
	lua_pushstring(L, "tangpl");
	lua_pcall(L, 1, 0, 0);	
/*
	dump_stack(L);		
	lua_getglobal(L, "lua_func");
	dump_stack(L);	
	printf("%p\n", lua_tocfunction(L, 0));		
	printf("%p\n", lua_tocfunction(L, 1));
	printf("%p\n", lua_tocfunction(L, -1));
	printf("%p\n", lua_tocfunction(L, -2));		
	lua_pushnil(L);
	lua_pushnil(L);
	lua_pushnil(L);

	lua_pushcfunction(L, 0);
	lua_pushcfunction(L, 0);
	lua_pushcfunction(L, 0);	
	dump_stack(L);
	
	lua_getglobal(L, "lua_func");
	printf("%p\n", lua_tocfunction(L, 0));
	printf("%p\n", lua_tocfunction(L, 1));
	printf("%p\n", lua_tocfunction(L, -1));	
	dump_stack(L);
*/
	return (0);
}

void testlua(int i)
{
	int n, ret;
	for (n = 0; n < i; ++n) {
		lua_getglobal(L, "test_func");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);
		ret = lua_tonumber(L, -1);
	}
}
#define itype(o)	((o)->it)
#define uitype(o)	((uint32_t)itype(o))

void testlua2(int i)
{
	int n, ret;
	TValue *o, func;

	lua_getglobal(L, "test_func");
	o = (TValue *)index2adr(L, -1);	
#ifdef LUA_JIT
	if (isluafunc(funcV(o))) {
		copyTV(L, &func, o);
	} else
		assert(0);
#else
	if (isLfunction(o)) {
//		setclvalue(L, &func, o->value.gc);
		setobj(L, &func, o);
	} else
		assert(0);
#endif
	for (n = 0; n < i; ++n) {
#ifdef LUA_JIT
		copyTV(L, L->top, &func);
		incr_top(L);
#else
		lua_lock(L);
		setobj(L, L->top, &func);
		api_incr_top(L);
		lua_unlock(L);
#endif	
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);
		ret = lua_tonumber(L, -1);
	}
}

int cfunc(int i)
{
	int n;
	int ret = 0;
	for (n = 0; n < i; ++n) {
		ret += i;
	}
	return ret;
}

void testlua3(int i)
{
	int n, ret;
	for (n = 0; n < i; ++n) {
		ret = cfunc(i);
	}
}

int main(int argc, char *argv[])
{
	if (argc == 2) {
		lua_init(argv[1]);
//		testlua(8000);
//		printf("222\n");
//		testlua(25000);
		testlua2(25000);
	}

	return (0);
}
