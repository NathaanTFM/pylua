#ifndef PYLUA_THREAD_H
#define PYLUA_THREAD_H

#include "pylua.h"
#include "pylua_object.h"

PyTypeObject LuaThreadType;

PyObject* LuaThread_call(LuaObject* self, PyObject* args);
PyObject* LuaThread_get_state(LuaObject* self, PyObject* args);

#endif