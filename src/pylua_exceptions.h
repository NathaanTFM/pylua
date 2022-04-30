#ifndef PYLUA_EXCEPTIONS_H
#define PYLUA_EXCEPTIONS_H

#include "pylua.h"

PyObject* LuaError;
PyObject* LuaCompileError;
PyObject* LuaFatalError;
PyObject* LuaRuntimeError;

int pylua_init_exceptions();

#endif