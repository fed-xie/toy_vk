#include "include/toy_lua.h"

#include "toy_assert.h"
#include "include/toy_log.h"
#include "include/toy_file.h"
#include <string.h>

#pragma comment(lib, "libs/lua.lib")


lua_State* toy_lua_new_vm ()
{
	lua_State* vm = luaL_newstate();
	if (NULL == vm)
		return NULL;

	luaL_checkversion(vm);
	luaL_openlibs(vm);

	return vm;
}


static int toy_luacf_callva(lua_State* lua_vm)
{
	int argc = lua_gettop(lua_vm);
	const char* fname = (const char*)lua_touserdata(lua_vm, 1);
	lua_getglobal(lua_vm, fname);
	int inputn = 0;
	if (argc > 1)
	{
		const char* fmt = (const char*)lua_touserdata(lua_vm, 2);
		va_list ap = *((va_list*)lua_touserdata(lua_vm, 3));

		luaL_checkstack(lua_vm, strlen(fmt), "Check stack failed");
		while ('\0' != fmt[inputn])
		{
			switch (fmt[inputn])
			{
			case 's': lua_pushstring(lua_vm, va_arg(ap, const char*)); break;
			case 'd': lua_pushnumber(lua_vm, va_arg(ap, double)); break;
			case 'i': lua_pushinteger(lua_vm, va_arg(ap, int)); break;
			case 'l': lua_pushlightuserdata(lua_vm, va_arg(ap, void*)); break;
			case 'g': lua_getglobal(lua_vm, va_arg(ap, const char*)); break;
			default:
				lua_pushstring(lua_vm, "luacf_call input format syntax error");
				lua_error(lua_vm);
			}
			++inputn;
		}
	}
	lua_call(lua_vm, inputn, 0);
	return 0;
}

int toy_lua_host_call (lua_State* lua_vm, const char* funcName, const char* fmt, ...)
{
	assert(funcName);
	int err = LUA_OK;
	lua_pushcfunction(lua_vm, toy_luacf_callva);
	lua_pushlightuserdata(lua_vm, (void*)funcName);
	if (fmt)
	{
		va_list ap;
		va_start(ap, fmt);
		lua_pushlightuserdata(lua_vm, (void*)fmt);
		lua_pushlightuserdata(lua_vm, &ap);
		err = lua_pcall(lua_vm, 3, 0, 0);
		va_end(ap);
	}
	else
	{
		err = lua_pcall(lua_vm, 1, 0, 0);
	}

	if (LUA_OK != err && LUA_TSTRING == lua_type(lua_vm, -1))
	{
		const char* msg = lua_tostring(lua_vm, -1);
		toy_log_e("toy_lua_host_call(%d): %s", __LINE__, msg);
		lua_pop(lua_vm, 1);
	}
	return 0;
}


int toy_lua_vm_call (lua_State* lua_vm, lua_CFunction func, ...)
{
	va_list ap;
	va_start(ap, func);
	lua_pushcfunction(lua_vm, func);
	lua_pushlightuserdata(lua_vm, &ap);
	int err = lua_pcall(lua_vm, 1, 0, 0);
	va_end(ap);

	if (LUA_OK != err && LUA_TSTRING == lua_type(lua_vm, -1))
	{
		const char* msg = lua_tostring(lua_vm, -1);
		toy_log_e("toy_lua_vm_call(%d): %s", __LINE__, msg);
		lua_pop(lua_vm, 1);
	}
	return 0;
}


static int luacf_reqLib(lua_State* L)
{
	luaL_Reg* lib = (luaL_Reg*)lua_touserdata(L, 1);
	lua_Integer glb = lua_tointeger(L, 2);
	for (; lib->func; ++lib)
	{
		luaL_requiref(L, lib->name, lib->func, glb);
		lua_pop(L, 1);  /* remove lib */
	}
	return 0;
}

int toy_lua_require (lua_State* lua_vm, luaL_Reg mod[], int global /*TRUE*/)
{
	lua_pushcfunction(lua_vm, luacf_reqLib);
	lua_pushlightuserdata(lua_vm, mod);
	lua_pushinteger(lua_vm, global);
	int err = lua_pcall(lua_vm, 2, 0, 0);
	if (LUA_OK != err && LUA_TSTRING == lua_type(lua_vm, -1))
	{
		const char* msg = lua_tostring(lua_vm, -1);
		toy_log_e("toy_lua_require: %s", msg);
		lua_pop(lua_vm, 1);
	}
	return 0;
}


void toy_lua_run (lua_State* lua_vm, const toy_allocator_t* alc, const char* script_path, toy_error_t* error)
{
	size_t script_size = 0;
	const char* file_content = toy_std_load_whole_file(script_path, alc, &script_size, error);
	if (toy_is_failed(*error)) {
		if (TOY_ERROR_FILE_OPEN_FAILED == error->err_code)
			toy_log_e("Open file %s failed", script_path);
		return;
	}

	int err = luaL_loadbuffer(lua_vm, file_content, script_size, script_path);
	toy_free_aligned(alc, (toy_aligned_p)file_content);
	if (LUA_OK != err)
	{
		const char* msg = lua_tostring(lua_vm, -1);
		toy_log_e("toy_lua_run: %s", msg);
		lua_pop(lua_vm, 1);
		toy_err(TOY_ERROR_OPERATION_FAILED, msg, error);
		return;
	}

	err = lua_pcall(lua_vm, 0, LUA_MULTRET, 0);
	if (LUA_OK != err)
	{
		if (LUA_TSTRING == lua_type(lua_vm, -1)) {
			const char* msg = lua_tostring(lua_vm, -1);
			toy_log_e("toy_lua_run: %s", msg);
			toy_err(TOY_ERROR_OPERATION_FAILED, "Run lua script failed", error);
		}
		lua_settop(lua_vm, 0);
	}
	else {
		toy_ok(error);
	}
	return;
}


static int toy_luacf_gc(lua_State* L)
{
	lua_Integer what = lua_tointeger(L, 1);
	lua_Integer data = lua_tointeger(L, 2);
	lua_Integer ret = lua_gc(L, what, data);
	lua_pushinteger(L, ret);
	return 1;
}

int toy_lua_gc(lua_State* lua_vm, int what, int data)
{
	toy_log_d("before gc, lua_gettop() = %d", lua_gettop(lua_vm));
	lua_pushcfunction(lua_vm, toy_luacf_gc);
	lua_pushinteger(lua_vm, what);
	lua_pushinteger(lua_vm, data);
	int err = lua_pcall(lua_vm, 2, 1, 0);
	if (LUA_OK != err && LUA_TSTRING == lua_type(lua_vm, -1))
	{
		const char* msg = lua_tostring(lua_vm, -1);
		toy_log_e("toy_lua_gc: %s", msg);
		lua_pop(lua_vm, 1);
		return -1;
	}
	lua_Integer res = lua_tointeger(lua_vm, 1);
	lua_pop(lua_vm, 1);
	toy_log_d("after gc, lua_gettop() = %d", lua_gettop(lua_vm));
	return res;
}


lua_Integer toy_lua_rawget_integer (lua_State* lua_vm, const char* key) {
	lua_pushstring(lua_vm, key);
	int value_type = lua_rawget(lua_vm, -2);
#if TOY_DEBUG
	luaL_checktype(lua_vm, -1, LUA_TNUMBER);
#endif
	lua_Integer ret = lua_tointeger(lua_vm, -1);
	lua_pop(lua_vm, 1);
	return ret;
}


void toy_lua_rawget_string (lua_State* lua_vm, const char* key, char* dst, size_t dst_size) {
	lua_pushstring(lua_vm, key);
	int value_type = lua_rawget(lua_vm, -2);
#if TOY_DEBUG
	luaL_checktype(lua_vm, -1, LUA_TSTRING);
#endif
	const char* val = lua_tostring(lua_vm, -1);
	strncpy_s(dst, dst_size, val, dst_size);
	dst[dst_size - 1] = '\0';
	lua_pop(lua_vm, 1);
}
