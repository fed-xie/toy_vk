#pragma once

#include "toy_platform.h"
#include "toy_error.h"
#include "toy_allocator.h"

#if TOY_CPP
#include <lua.hpp>
#else
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif

TOY_EXTERN_C_START


lua_State* toy_lua_new_vm ();


/* call a lua function, fmt only accept:
   's' as a const char* for a string
   'd' as a double
   'i' as a int
   'g' as a const char* for a global variable name
   'l' as a void* for a light user data
   eg.
	int n = 42;
	double d = .33;
	char* str = "arg string";
	call("foo", "isd", n, str, d);
 */
int toy_lua_host_call (lua_State* lua_vm, const char* funcName, const char* fmt, ...);

/* call a lua_CFunction,
   pass a va_list* to the func (by light user data)
 */
int toy_lua_vm_call (lua_State* lua_vm, lua_CFunction func, ...);

void toy_lua_run (lua_State* lua_vm, const toy_allocator_t* alc, const char* script_path, toy_error_t* error);

//wrapper of luaL_requiref(), working similar to luaL_openlibs()
int toy_lua_require (lua_State* lua_vm, luaL_Reg mod[], int global /*TRUE*/);

//wrapper of lua_gc()
int toy_lua_gc(lua_State* lua_vm, int what, int data);


toy_inline void toy_lua_rawget_table (lua_State* lua_vm, const char* key) {
	lua_pushstring(lua_vm, key);
	int value_type = lua_rawget(lua_vm, -2);
	luaL_checktype(lua_vm, -1, LUA_TTABLE);
}
void toy_lua_rawget_string (lua_State* lua_vm, const char* key, char* dst, size_t dst_size);
lua_Integer toy_lua_rawget_integer (lua_State* lua_vm, const char* key);

TOY_EXTERN_C_END
