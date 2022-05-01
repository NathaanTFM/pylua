#ifndef PYLUA_HOOKS_H
#define PYLUA_HOOKS_H

#include "pylua.h"
#include "pylua_state.h"

void pylua_hook_builtin(lua_State* L, lua_Debug* ar);
void pylua_hook_python(lua_State* L, lua_Debug* ar);
int pylua_panic(lua_State* L);
void* pylua_alloc(LuaStateObject* self, void* ptr, size_t osize, size_t nsize);
int pylua_gc(lua_State* L);
int pylua_tostring(lua_State* L);
int pylua_call_python(lua_State* L);

#endif