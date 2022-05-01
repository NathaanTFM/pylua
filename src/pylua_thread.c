#include "pylua_thread.h"
#include "pylua_exceptions.h"
#include "pylua_protect.h"
#include "pylua_python.h"
#include "pylua_stateinfo.h"

static PyObject* LuaThread_call(LuaObject* self, PyObject* args) {
    if (!args || PyTuple_GET_SIZE(args) < 0) {
        PyErr_SetString(PyExc_TypeError, "function takes at least 1 argument (0 given)");
        return NULL;
    }
    
    LuaObject* func = (LuaObject*)PyTuple_GET_ITEM(args, 0);
    if (!PyObject_TypeCheck(func, &LuaObjectType)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be pylua.LuaObject");
        return NULL;
    }
    
    if (self->sobj->info.thstate) {
        PyErr_SetString(LuaFatalError, "not thread safe");
        return NULL;
    }
    
    PYLUA_CHECK(L, &self->sobj->info, NULL);
    PYLUA_PROTECT(&self->sobj->info, NULL);
    
    // first we need to get our thread object
    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);
    lua_State* thread = lua_tothread(L, -1);
    lua_pop(L, -1);
    
    // we need our info object
    struct LuaStateInfo* info = pylua_get_stateinfo(L, thread);
    
    PYLUA_UNPROTECT(&self->sobj->info);
    
    if (info->thstate) {
        PyErr_SetString(LuaFatalError, "not thread safe");
        return NULL;
    }
    return pylua_call(info, func->ref, args, 1);
}


static PyObject* LuaThread_get_state(LuaObject* self, PyObject* args) {
    PYLUA_CHECK(L, &self->sobj->info, NULL);
    PYLUA_PROTECT(&self->sobj->info, NULL);
    
    // first we need to get our thread object
    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);
    lua_State* thread = lua_tothread(L, -1);
    lua_pop(L, -1);
    
    // we need our info object
    struct LuaStateInfo* info = pylua_get_stateinfo(L, thread);
    PYLUA_UNPROTECT(&self->sobj->info);
    
    // there it is
    Py_INCREF(info->root);
    return (PyObject*)info->root;
}


static PyMethodDef LuaThread_methods[] = {
    {"call", (PyCFunction)LuaThread_call, METH_VARARGS, "call a lua function using the thread"},
    {"get_state", (PyCFunction)LuaThread_get_state, METH_NOARGS, "get the state from a thread"},
    {NULL}
};
    
PyTypeObject LuaThreadType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pylua.LuaThread",
    .tp_doc = "Lua thread",
    .tp_base = &LuaObjectType,
    .tp_methods = LuaThread_methods
};