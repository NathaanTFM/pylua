#include "pylua_state.h"
#include "pylua_exceptions.h"
#include "pylua_hooks.h"
#include "pylua_protect.h"
#include "pylua_python.h"

/**
 * Implement tp_new for our LuaState type
 * which just allocs our type with default NULL values
 */
PyObject* LuaState_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    LuaStateObject* self = (LuaStateObject*)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->info.state = NULL;
    }
    return (PyObject*)self;
}


/**
 * Implement tp_init for our LuaState type ;
 * Create a lua_State, setup panic handlers, etc.
 */
int LuaState_init(LuaStateObject* self, PyObject* args, PyObject* kwds) {
    static char* keywords[] = {"openlibs", NULL};
    int openlibs = 1; // init the libs by default

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|p", keywords, &openlibs)) {
        return -1;
    }

    // memory info
    self->mem = 0;
    self->limit = SIZE_MAX;

    // create the state
    lua_State* L = lua_newstate((lua_Alloc)&pylua_alloc, self);
    
    // initialize the state info
    pylua_set_stateinfo(L, &self->info);
    self->info.root = self;
    
    // sets the panic handler
    lua_atpanic(L, &pylua_panic);

    // open the libs
    if (openlibs)
        luaL_openlibs(L);

    return 0;
}

/**
 * Implements LuaState.load_file, which compiles a script from a file
 * to a callable LuaFunction
 */
PyObject* LuaState_load_file(LuaStateObject* self, PyObject* args) {
    // parse the args
    const char* filename;
    const char* mode = NULL; // defaults to "bt"

    if (!PyArg_ParseTuple(args, "s|z", &filename, &mode)) {
        return NULL;
    }
    
#if LUA_VERSION_NUM <= 501
    if (mode) {
        PyErr_SetObject(LuaError, LUA_VERSION " does not support mode arg");
        return NULL;
    }
#endif

    PYLUA_CHECK(L, &self->info, NULL);

    // attempt to compile
#if LUA_VERSION_NUM >= 502
    int err = luaL_loadfilex(L, filename, mode);
#else
    int err = luaL_loadfile(L, filename);
#endif
    if (err) {
        PyObject* err = pylua_get_as_unicode(L, -1);
        PyErr_SetObject(LuaCompileError, err);
        Py_DECREF(err);
        lua_pop(L, 1);
        return NULL;
    }

    // get it as a luafunction
    PyObject* res = pylua_get_as_pyobj(&self->info, -1);
    lua_pop(L, 1);

    // error handling for pylua_get_as_pyobj is useless
    return res;
}

/**
 * Implements LuaState.load_string, which compiles a string
 * to a callable LuaFunction
 */
PyObject* LuaState_load_string(LuaStateObject* self, PyObject* args) {
    // parse the args
    const char* script;
    Py_ssize_t len;
    
    const char* name = NULL;
    const char* mode = NULL; // defaults to "bt"

    if (!PyArg_ParseTuple(args, "s#|zz", &script, &len, &name, &mode)) {
        return NULL;
    }
    
#if LUA_VERSION_NUM <= 501
    if (mode) {
        PyErr_SetObject(LuaError, LUA_VERSION " does not support mode arg");
        return NULL;
    }
#endif

    // default luaL behaviour
    if (!name)
        name = script;
    
    PYLUA_CHECK(L, &self->info, NULL);

    // attempt to compile
#if LUA_VERSION_NUM >= 502
    int err = luaL_loadbufferx(L, script, len, name, mode);
#else
    int err = luaL_loadbuffer(L, script, len, name);
#endif
    if (err) {
        PyObject* err = pylua_get_as_unicode(L, -1);
        PyErr_SetObject(LuaCompileError, err);
        Py_DECREF(err);
        lua_pop(L, 1);
        return NULL;
    }

    // get it as a luafunction
    PyObject* res = pylua_get_as_pyobj(&self->info, -1);
    lua_pop(L, 1);

    return res;
}

/**
 * Implements LuaState.get_globals, which returns the global table
 */
PyObject* LuaState_get_globals(LuaStateObject *self, void *unused) {
    PYLUA_CHECK(L, &self->info, NULL);

#if LUA_VERSION_NUM >= 502
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
#else
    lua_pushvalue(L, LUA_GLOBALSINDEX);
#endif
    PyObject* globals = pylua_get_as_pyobj(&self->info, -1);
    lua_pop(L, 1);

    return globals;
}

/**
 * Implements LuaState.new_table, which returns a new thread
 */
PyObject* LuaState_new_thread(LuaStateObject* self, PyObject* args) {
    PYLUA_CHECK(L, &self->info, NULL);
    PYLUA_PROTECT(&self->info, NULL);

    lua_State* thread = lua_newthread(L);
    PyObject* res = pylua_get_as_pyobj(&self->info, -1);
    lua_pop(L, 1);
    
    struct LuaStateInfo* info = pylua_alloc_stateinfo(thread);
    info->root = self;
    
    PYLUA_UNPROTECT(&self->info);
    return res;
}

/**
 * Implements LuaState.new_table, which returns a new empty table
 */
PyObject* LuaState_new_table(LuaStateObject* self, PyObject* args) {
    //if (!PyArg_ParseTuple(args, "")) {
    //    return NULL;
    //}

    PYLUA_CHECK(L, &self->info, NULL);

    // protect from memory allocation errors
    PYLUA_PROTECT(&self->info, NULL);

    lua_newtable(L);
    PyObject* res = pylua_get_as_pyobj(&self->info, -1);
    lua_pop(L, 1);

    // error handling for pylua_get_as_pyobj is useless
    PYLUA_UNPROTECT(&self->info);
    return res;
}

/**
 * Implements LuaState.new_userdata, which converts a Python object
 * to a lua userdata
 */
PyObject* LuaState_new_userdata(LuaStateObject* self, PyObject* args) {
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    
    PYLUA_CHECK(L, &self->info, NULL);
    PYLUA_PROTECT(&self->info, NULL);
    
    Py_INCREF(obj);
    PyObject** userdata = lua_newuserdata(L, sizeof(PyObject*));
    *userdata = obj;
    
    lua_newtable(L);
    lua_pushcclosure(L, &pylua_gc, 0);
    lua_setfield(L, -2, "__gc");
    //lua_pushcclosure(L, &pylua_tostring, 0);
    //lua_setfield(L, -2, "__tostring");
    lua_setmetatable(L, -2);
    
    PyObject* res = pylua_get_as_pyobj(&self->info, -1);
    lua_pop(L, 1);
    
    PYLUA_UNPROTECT(&self->info);
    return res;
}

/**
 * Implements LuaState.new_function.
 * This method takes a single callable parameter, and creates a LuaFunction from it
 */
PyObject* LuaState_new_function(LuaStateObject* self, PyObject* args) {
    PyObject* func;

    if (!PyArg_ParseTuple(args, "O", &func)) {
        return NULL;
    }

    PYLUA_CHECK(L, &self->info, NULL);

    // protect from memory allocation errors
    PYLUA_PROTECT(&self->info, NULL);

    // We can't check the type of our function: it could be an instance,
    // a callable class, or just a regular function ; the user can do whatever he wants

    // Let's push our function as an upvalue
    Py_INCREF(func);
    PyObject** userdata = lua_newuserdata(L, sizeof(PyObject*));
    *userdata = func;

    // we want to add a gc handler, so let's create a metatable
    lua_newtable(L);
    //lua_pushlightuserdata(L, func);
    //lua_pushvalue(L, -2); // let's use the same one instead
    lua_pushcclosure(L, &pylua_gc, 0);
    lua_setfield(L, -2, "__gc");

    // we got our lightuserdata followed by our table
    lua_setmetatable(L, -2);

    // push our new C closure
    lua_pushcclosure(L, &pylua_call_python, 1);

    // Then we get it as a LuaTable
    PyObject* res = pylua_get_as_pyobj(&self->info, -1);
    lua_pop(L, 1);

    // Error handling for get_as_pyobj is useless
    PYLUA_UNPROTECT(&self->info);
    return res;
}

/**
 * Implements LuaState.close(), which closes the lua state
 * Returns True if the state was closed, False otherwise.
 */
PyObject* LuaState_close(LuaStateObject* self, void* unused) {
    if (self->info.state) {
        lua_close(self->info.state);
        self->info.state = NULL;
        Py_RETURN_TRUE;
    }
    
    Py_RETURN_FALSE;
}

/**
 * Getter for LuaState.mem_usage
 * Returns the current memory usage
 */
PyObject* LuaState_get_mem_usage(LuaStateObject* self, void* unused) {
    return PyLong_FromSize_t(self->mem);
}

/**
 * Getter for LuaState.mem_limit
 * Returns the memory limit
 */
PyObject* LuaState_get_mem_limit(LuaStateObject* self, void* unused) {
    return PyLong_FromSize_t(self->limit);
}

/**
 * Setter for LuaState.mem_limit
 * Sets the memory limit
 */
int LuaState_set_mem_limit(LuaStateObject* self, PyObject* value, void* unused) {
    size_t limit = PyLong_AsSize_t(value);
    if (limit == (size_t)-1 && PyErr_Occurred()) {
        return -1;
    }

    self->limit = limit;
    return 0;
}

/**
 * Getter for LuaState.time_limit
 * Returns the execution time limit
 */
PyObject* LuaState_get_time_limit(LuaStateObject* self, void* unused) {
    return PyLong_FromLong(self->info.timelimit);
}

/**
 * Setter for LuaState.time_limit
 * Sets the execution time limit
 */
int LuaState_set_time_limit(LuaStateObject* self, PyObject* value, void* unused) {
    int limit = pylua_pylong_as_int(value);
    if (limit == -1 && PyErr_Occurred()) {
        return -1;
    }

    self->info.timelimit = limit;
    if (limit == 0) {
        lua_sethook(self->info.state, NULL, 0, 0); 
    } else {
        lua_sethook(self->info.state, &pylua_hook, LUA_MASKCOUNT, limit * 250); 
    }
    return 0;
}

/**
 * Handle deallocation of LuaState
 */
void LuaState_dealloc(LuaStateObject* self) {
    if (self->info.state) {
        lua_close(self->info.state);
        self->info.state = NULL;
    }
    
    // TODO: move that to lua gc
    if (self->info.panic) {
        fprintf(stderr, "PyLua PANIC: panic handler exists (depth %d) on close\n", pylua_get_panichandler_depth(&self->info));
        
        while (self->info.panic) {
            pylua_pop_panichandler(&self->info);
        }
    } else {
        //PYLUA_DEBUG_2("panic handler is clean");
    }
    
    // delete ourselves
    Py_TYPE(self)->tp_free((PyObject *) self);
}


static PyMethodDef LuaState_methods[] = {
    {"get_globals", (PyCFunction)LuaState_get_globals, METH_NOARGS, "return the global table"},
    {"load_string", (PyCFunction)LuaState_load_string, METH_VARARGS, "compile a string to a LuaFunction"},
    {"load_file", (PyCFunction)LuaState_load_file, METH_VARARGS, "compile a file to a LuaFunction"},
    {"new_thread", (PyCFunction)LuaState_new_thread, METH_VARARGS, "create a new state for threading"},
    {"new_table", (PyCFunction)LuaState_new_table, METH_NOARGS, "create a new table"},
    {"new_userdata", (PyCFunction)LuaState_new_userdata, METH_VARARGS, "create a new userdata"},
    {"new_function", (PyCFunction)LuaState_new_function, METH_VARARGS, "create a LuaFunction bound to a Python callable"},
    //{"sethook", (PyCFunction)LuaState_sethook, METH_VARARGS, "set a lua debug hook"},
    {"close", (PyCFunction)LuaState_close, METH_NOARGS, "close the lua state"},
    {NULL}
};

static PyGetSetDef LuaState_getset[] = {
    {"mem_usage", (getter)LuaState_get_mem_usage, NULL, "current memory usage", NULL},
    {"mem_limit", (getter)LuaState_get_mem_limit, (setter)LuaState_set_mem_limit, "current memory limit", NULL},
    {"time_limit", (getter)LuaState_get_time_limit, (setter)LuaState_set_time_limit, "current memory limit", NULL},
    //{"globals", (getter)LuaState_get_globals, NULL, "globals", NULL},
    {NULL}
};

PyTypeObject LuaStateType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pylua.LuaState",
    //.tp_doc = "",
    .tp_basicsize = sizeof(LuaStateObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = LuaState_new,
    .tp_init = (initproc)LuaState_init,
    .tp_dealloc = (destructor)LuaState_dealloc,
    .tp_methods = LuaState_methods,
    .tp_getset = LuaState_getset
};