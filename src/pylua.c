#include "pylua.h"
#include "pylua_exceptions.h"
#include "pylua_function.h"
#include "pylua_object.h"
#include "pylua_state.h"
#include "pylua_table.h"
#include "pylua_thread.h"
#include "pylua_userdata.h"


static PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    
    .m_name = "pylua",
    .m_doc = "Implements a lua environment for use with Python.",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit_pylua(void) {
    // init exceptions
    if (pylua_init_exceptions() < 0)
        return NULL;
    
    // init types
    if (PyType_Ready(&LuaStateType) < 0)
        return NULL;
    if (PyType_Ready(&LuaObjectType) < 0)
        return NULL;
    if (PyType_Ready(&LuaFunctionType) < 0)
        return NULL;
    if (PyType_Ready(&LuaTableType) < 0)
        return NULL;
    if (PyType_Ready(&LuaThreadType) < 0)
        return NULL;
    if (PyType_Ready(&LuaUserDataType) < 0)
        return NULL;
    
    // create module
    PyObject* mod = PyModule_Create(&module);
    if (!mod)
        return NULL;
    
    // those might not be necessary, as NewException returns a new reference
    /*Py_INCREF(LuaError);
    Py_INCREF(LuaCompileError);
    Py_INCREF(LuaRuntimeError);
    Py_INCREF(LuaFatalError);*/
    PyModule_AddObject(mod, "LuaError", LuaError);
    PyModule_AddObject(mod, "LuaCompileError", LuaCompileError);
    PyModule_AddObject(mod, "LuaRuntimeError", LuaRuntimeError);
    PyModule_AddObject(mod, "LuaFatalError", LuaFatalError);
    
    // those might not be necessary either
    /*Py_INCREF(&LuaObjectType);
    Py_INCREF(&LuaStateType);
    Py_INCREF(&LuaTableType);
    Py_INCREF(&LuaFunctionType);
    Py_INCREF(&LuaUserDataType);
    Py_INCREF(&LuaThreadType);*/
    PyModule_AddObject(mod, "LuaObject", (PyObject *) &LuaObjectType);
    PyModule_AddObject(mod, "LuaState", (PyObject *) &LuaStateType);
    PyModule_AddObject(mod, "LuaTable", (PyObject *) &LuaTableType);
    PyModule_AddObject(mod, "LuaFunction", (PyObject *) &LuaFunctionType);
    PyModule_AddObject(mod, "LuaUserData", (PyObject *) &LuaUserDataType);
    PyModule_AddObject(mod, "LuaThread", (PyObject *) &LuaThreadType);
    
    // also some useful consts
    PyModule_AddIntConstant(mod, "LUA_HOOKCALL", LUA_HOOKCALL);
    PyModule_AddIntConstant(mod, "LUA_HOOKRET", LUA_HOOKRET);
    PyModule_AddIntConstant(mod, "LUA_HOOKLINE", LUA_HOOKLINE);
    PyModule_AddIntConstant(mod, "LUA_HOOKCOUNT", LUA_HOOKCOUNT);
    PyModule_AddIntConstant(mod, "LUA_HOOKTAILCALL", LUA_HOOKTAILCALL);
    
    PyModule_AddIntConstant(mod, "LUA_MASKCALL", LUA_MASKCALL);
    PyModule_AddIntConstant(mod, "LUA_MASKRET", LUA_MASKRET);
    PyModule_AddIntConstant(mod, "LUA_MASKLINE", LUA_MASKLINE);
    PyModule_AddIntConstant(mod, "LUA_MASKCOUNT", LUA_MASKCOUNT);
    return mod;
}