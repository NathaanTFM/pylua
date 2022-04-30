#ifndef PYLUA_PYTHON_H
#define PYLUA_PYTHON_H

#include "pylua.h"
#include "pylua_state.h"
#include "pylua_stateinfo.h"

PyObject* pylua_get_as_unicode(lua_State* L, int idx);


#if INT_MIN == LONG_MIN && INT_MAX == LONG_MAX
#   define pylua_pylong_as_int(val) (int)PyLong_AsLong(val)
#else
int pylua_pylong_as_int(PyObject* val);
#endif

int pylua_push_pyobj(lua_State* L, PyObject* obj);
PyObject* pylua_get_as_pyobj(struct LuaStateInfo* info, int idx);

PyObject* pylua_to_tuple(struct LuaStateInfo* info, int argc);
int pylua_push_tuple(lua_State* L, PyObject* obj, int startat);

PyObject* pylua_alloc_luaobject(PyTypeObject* type, LuaStateObject* sobj, int ref);
PyObject* pylua_call(struct LuaStateInfo* info, int funcref, PyObject* args, int startat);

#endif