#include "pylua_stateinfo.h"

/**
 * Returns the address of the struct LuaStateInfo associated with a lua_State,
 */
struct LuaStateInfo* pylua_get_stateinfo(lua_State* L, lua_State* thread) {
    lua_pushlightuserdata(L, thread);
    lua_rawget(L, LUA_REGISTRYINDEX);
    struct LuaStateInfo* res = (struct LuaStateInfo*)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return res;
}

/**
 * Initialize a struct LuaStateInfo*
 */
void pylua_init_stateinfo(struct LuaStateInfo* info, lua_State* L) {
    // init 
    info->state = L;
    info->panic = NULL;
    info->startat.time = 0;
    info->timelimit = 0;
    info->depth = 0;
    info->thstate = NULL;
}

/**
 * Sets a struct LuaStateInfo* to a lua_State
 */
void pylua_set_stateinfo(lua_State* L, struct LuaStateInfo* info) {
    lua_pushlightuserdata(L, L); // key
    lua_pushlightuserdata(L, info); // value
    lua_rawset(L, LUA_REGISTRYINDEX);
    
    pylua_init_stateinfo(info, L);
}

/**
 * Allocates a struct LuaStateInfo* to a lua_State
 */
struct LuaStateInfo* pylua_alloc_stateinfo(lua_State* L) {
    lua_pushlightuserdata(L, L); // key
    struct LuaStateInfo* info = lua_newuserdata(L, sizeof *info); // value
    lua_rawset(L, LUA_REGISTRYINDEX);
    
    pylua_init_stateinfo(info, L);
    return info;
}