#include "pylua_table.h"
#include "pylua_protect.h"
#include "pylua_python.h"

/**
 * Implements `len` for a LuaTable
 */
Py_ssize_t LuaTable_length(LuaObject* self) {
    PYLUA_CHECK(L, &self->sobj->info, -1);

    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);
#if LUA_VERSION_NUM >= 502
    size_t length = lua_rawlen(L, -1);
#else
    size_t length = lua_objlen(L, -1);
#endif
    lua_pop(L, 1);
    return length;
}

/**
 * Implements `getattr` AND `getitem` for a LuaTable
 */
PyObject* LuaTable_getattr(LuaObject* self, PyObject* attr) {
    PYLUA_CHECK(L, &self->sobj->info, NULL);
    PYLUA_PROTECT(&self->sobj->info, NULL);

    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);

    if (pylua_push_pyobj(L, attr)) {
        lua_pop(L, 1);
        PYLUA_UNPROTECT(&self->sobj->info);
        return NULL;
    }

    lua_rawget(L, -2);
    PyObject* val = pylua_get_as_pyobj(&self->sobj->info, -1);
    lua_pop(L, 2);

    // error handling for pylua_get_as_pyobj is useless
    PYLUA_UNPROTECT(&self->sobj->info);
    return val;
}


/**
 * Implements `setattr` AND `setitem` for a LuaTable
 */
int LuaTable_setattr(LuaObject* self, PyObject* attr, PyObject* value) {
    PYLUA_CHECK(L, &self->sobj->info, -1);
    PYLUA_PROTECT(&self->sobj->info, -1);

    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);
    if (pylua_push_pyobj(L, attr)) {
        lua_pop(L, 1);
        PYLUA_UNPROTECT(&self->sobj->info);
        return -1;
    }
    if (pylua_push_pyobj(L, value)) {
        lua_pop(L, 2);
        PYLUA_UNPROTECT(&self->sobj->info);
        return -1;
    }
    lua_rawset(L, -3);
    lua_pop(L, 1);

    PYLUA_UNPROTECT(&self->sobj->info);
    return 0;
}


static PyMappingMethods LuaTableMappingMethods = {
    .mp_length = (lenfunc)LuaTable_length,
    .mp_subscript = (binaryfunc)LuaTable_getattr,
    .mp_ass_subscript = (objobjargproc)LuaTable_setattr
};

PyTypeObject LuaTableType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pylua.LuaTable",
    .tp_doc = "Lua table",
    .tp_base = &LuaObjectType,
    .tp_getattro = (getattrofunc)LuaTable_getattr,
    .tp_setattro = (setattrofunc)LuaTable_setattr,
    .tp_as_mapping = &LuaTableMappingMethods
};