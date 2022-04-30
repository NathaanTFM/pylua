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

} LuaStateObject;

PyObject* LuaState_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
int LuaState_init(LuaStateObject* self, PyObject* args, PyObject* kwds);
PyObject* LuaState_load_file(LuaStateObject* self, PyObject* args);
PyObject* LuaState_load_string(LuaStateObject* self, PyObject* args);
PyObject* LuaState_get_globals(LuaStateObject *self, void *unused);
PyObject* LuaState_new_thread(LuaStateObject* self, PyObject* args);
PyObject* LuaState_new_table(LuaStateObject* self, PyObject* args);
PyObject* LuaState_new_userdata(LuaStateObject* self, PyObject* args);
PyObject* LuaState_new_function(LuaStateObject* self, PyObject* args);
PyObject* LuaState_close(LuaStateObject* self, void* unused);
PyObject* LuaState_get_mem_usage(LuaStateObject* self, void* unused);
PyObject* LuaState_get_mem_limit(LuaStateObject* self, void* unused);
int LuaState_set_mem_limit(LuaStateObject* self, PyObject* value, void* unused);
PyObject* LuaState_get_time_limit(LuaStateObject* self, void* unused);
int LuaState_set_time_limit(LuaStateObject* self, PyObject* value, void* unused);
void LuaState_dealloc(LuaStateObject* self);

PyTypeObject LuaStateType;

#endif