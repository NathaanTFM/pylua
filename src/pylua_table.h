#ifndef PYLUA_TABLE_H
#define PYLUA_TABLE_H

#include "pylua.h"
#include "pylua_object.h"

Py_ssize_t LuaTable_length(LuaObject* self);
PyObject* LuaTable_getattr(LuaObject* self, PyObject* attr);
int LuaTable_setattr(LuaObject* self, PyObject* attr, PyObject* value);

PyTypeObject LuaTableType;

#endif