#ifndef PYLUA_STATEINFO_H
#define PYLUA_STATEINFO_H

#include "pylua.h"
#include <sys/timeb.h>

struct PanicHandler;
struct _LuaStateObject;

struct LuaStateInfo {
    // Associated lua state
    lua_State* state;
    
    // Panic handler
    struct PanicHandler* panic;
    
    // Time limiter
    struct timeb startat;
    int timelimit;
    
    // Call depth
    int depth;
    
    // Thread state
    PyThreadState* thstate;
    
    // The root state object
    struct _LuaStateObject* root;
};

struct LuaStateInfo* pylua_get_stateinfo(lua_State* L, lua_State* thread);
void pylua_init_stateinfo(struct LuaStateInfo* info, lua_State* L);
void pylua_set_stateinfo(lua_State* L, struct LuaStateInfo* info);
struct LuaStateInfo* pylua_alloc_stateinfo(lua_State* L);

#endif