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

#endif