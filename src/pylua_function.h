#ifndef PYLUA_FUNCTION_H
#define PYLUA_FUNCTION_H

#include "pylua.h"
#include "pylua_object.h"

PyTypeObject LuaFunctionType;

PyObject* LuaFunction_call(LuaObject* self, PyObject* args, PyObject* kwargs);
PyObject* LuaFunction_getfenv(LuaObject* self, PyObject* args);
PyObject* LuaFunction_setfenv(LuaObject* self, PyObject* args);

#endif