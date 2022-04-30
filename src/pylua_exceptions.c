#include "pylua_exceptions.h"

int pylua_init_exceptions() {
    // create the exceptions
    LuaError = PyErr_NewException("pylua.LuaError", NULL, NULL);
    LuaCompileError = PyErr_NewException("pylua.LuaCompileError", LuaError, NULL);
    LuaRuntimeError = PyErr_NewException("pylua.LuaRuntimeError", LuaError, NULL);
    LuaFatalError = PyErr_NewException("pylua.LuaFatalError", LuaError, NULL);
    if (!LuaError || !LuaCompileError || !LuaRuntimeError || !LuaFatalError) {
        Py_XDECREF(LuaError);
        Py_XDECREF(LuaCompileError);
        Py_XDECREF(LuaRuntimeError);
        Py_XDECREF(LuaFatalError);
        return -1;
    }
    return 0;
}