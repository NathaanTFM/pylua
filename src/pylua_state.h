#ifndef PYLUA_STATE_H
#define PYLUA_STATE_H

#include "pylua.h"
#include "pylua_stateinfo.h"

typedef struct _LuaStateObject {
    PyObject_HEAD
    
    // Lua state info
    struct LuaStateInfo info;

    // Memory usage and limit
    size_t mem;
    size_t limit;
    
    // Debug hook
    PyObject* hook;

} LuaStateObject;

PyTypeObject LuaStateType;

#endif