#ifndef PYLUA_OBJECT_H
#define PYLUA_OBJECT_H

#include "pylua.h"
#include "pylua_state.h"

typedef struct {
    PyObject_HEAD

    // The parent state
    LuaStateObject* sobj;

    // Reference number
    int ref;

} LuaObject;

PyTypeObject LuaObjectType;

Py_hash_t LuaObject_hash(LuaObject* self);
PyObject* LuaObject_richcompare(LuaObject* self, PyObject* other, int op);
void LuaObject_dealloc(LuaObject* self);

#endif