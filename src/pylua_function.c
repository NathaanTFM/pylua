#include "pylua_function.h"
#include "pylua_exceptions.h"
#include "pylua_protect.h"
#include "pylua_python.h"
#include "pylua_table.h"

/**
 * Implementation for the tp_call attribute of our LuaFunction type,
 * which allows us to call the lua function with python arguments,
 * and return a python object.
 */
static PyObject* LuaFunction_call(LuaObject* self, PyObject* args, PyObject* kwargs) {
    // We do not allow keyword arguments
    if (kwargs && PyDict_Size(kwargs)) {
        PyErr_SetString(PyExc_TypeError, "unexpected keyword argument");
        return NULL;
    }
    
    if (!self->sobj->info.state) {
        PyErr_SetString(LuaFatalError, "lua state is dead");
        return NULL;
    }
    
    if (self->sobj->info.thstate) {
        PyErr_SetString(LuaFatalError, "not thread safe");
        return NULL;
    }
    
    return pylua_call(&self->sobj->info, self->ref, args, 0);
}

/**
 * Implements LuaState.get_fenv, which returns the function environment (as a LuaTable)
 * from a given LuaFunction.
 *
 * For Lua >= 5.2, it uses the upvalue 1 which should contain the global table,
 * For Lua <= 5.1, it just uses getfenv
 */
static PyObject* LuaFunction_getfenv(LuaObject* self, PyObject* args) {
    PYLUA_CHECK(L, &self->sobj->info, NULL);

    // Protect from memory allocation errors
    PYLUA_PROTECT(&self->sobj->info, NULL);

    // Let's push the function first
    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);

    // Get the function env
#if LUA_VERSION_NUM >= 502
    lua_getupvalue(L, -1, 1);
#else
    lua_getfenv(L, -1);
#endif

    // Converts it to a pyobject
    PyObject* res = pylua_get_as_pyobj(&self->sobj->info, -1);
    lua_pop(L, 2);

    // Returns it (error handling is useless for res)
    PYLUA_UNPROTECT(&self->sobj->info);
    return res;
}

/**
 * Implements LuaState.get_fenv, which sets the function environment
 * of a given LuaFunction to a given LuaFunction.
 *
 * For Lua >= 5.2, it uses the upvalue 1 which should contain the global table,
 * For Lua <= 5.1, it just uses setfenv
 */
static PyObject* LuaFunction_setfenv(LuaObject* self, PyObject* args) {
    PyObject* env;

    if (!PyArg_ParseTuple(args, "O!", &LuaTableType, &env)) {
        return NULL;
    }

    PYLUA_CHECK(L, &self->sobj->info, NULL);

    // Protect from memory allocation
    PYLUA_PROTECT(&self->sobj->info, NULL);

    // Push the function
    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);

    // Push the env
    if (pylua_push_pyobj(L, env)) {
        PYLUA_UNPROTECT(&self->sobj->info);
        return NULL;
    }

    // Set the function env
#if LUA_VERSION_NUM >= 502
    lua_setupvalue(L, -2, 1);
#else
    lua_setfenv(L, -2);
#endif

    // Clean the stack
    lua_pop(L, 1);

    PYLUA_UNPROTECT(&self->sobj->info);
    Py_RETURN_NONE;
}


static PyMethodDef LuaFunction_methods[] = {
    {"getfenv", (PyCFunction)LuaFunction_getfenv, METH_VARARGS, "return a function environment"},
    {"setfenv", (PyCFunction)LuaFunction_setfenv, METH_VARARGS, "define a function environment"},
    {NULL}
};

PyTypeObject LuaFunctionType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pylua.LuaFunction",
    .tp_doc = "Lua function",
    .tp_base = &LuaObjectType,
    .tp_call = (ternaryfunc)LuaFunction_call,
    .tp_methods = LuaFunction_methods
};