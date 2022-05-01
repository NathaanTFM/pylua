#include "pylua_hooks.h"
#include "pylua_exceptions.h"
#include "pylua_protect.h"
#include "pylua_python.h"
#include "pylua_stateinfo.h"

#include <sys/timeb.h>


/**
 * Internal function that calls a Python object,
 * decrease the reference count of the `args` tuple.
 *
 * In case of error, saves the thread and attempt to
 * jump to the nearest panic handler, or abort if not recoverable.
 */
static PyObject* pylua_call_pyobject(struct LuaStateInfo* info, PyObject* func, PyObject* args) {
    // attempt to call the function
    PyObject* res = PyObject_CallObject(func, args);
    Py_DECREF(args);
    
    if (!res) {
        // save the thread back (we're leaving, so we need to do this now)
        info->thstate = PyEval_SaveThread();
    
        // the function might fail due to a lua panic,
        // (py function could be closing the lua state),
        // so we need to check if the state still exists
        // in our LuaStateObject
        if (!info->state) {
            if (info->panic) {
                longjmp(info->panic->buf, 1);
                
            } else {
                // TODO: state is lost, and no panic handler:
                // this might not happen, but in case it does,
                // we're not ready for it. abort.
                fprintf(stderr, "PyLua PANIC: lost state and no panic handler\n");
                abort();
                return NULL; // should not return
            }
        } else {
            // lua state still exists, so we're just dispatching a lua error
            lua_pushstring(info->state, "python error on call");
            lua_error(info->state);
            return NULL; // should not return
        }
    }
    
    return res;
}


/**
 * Builtin debug hook
 */
void pylua_hook_builtin(lua_State* L, lua_Debug* ar) {
    struct LuaStateInfo* info = pylua_get_stateinfo(L, L);
    
    // should we limit execution time?
    if (info->depth && info->timelimit) {
        // get current execution time
        struct timeb time;
        ftime(&time);
        int exectime = (time.time - info->startat.time) * 1000 + (time.millitm - info->startat.millitm);
        
        // compare it
        if (exectime > info->timelimit) {
            lua_pushfstring(L, "exceeded time limit of %d ms (%d ms)", info->timelimit, exectime);
            lua_error(L);
        }
    }
}


/**
 * Debug hook for Python functions
 */
void pylua_hook_python(lua_State* L, lua_Debug* ar) {
    struct LuaStateInfo* info = pylua_get_stateinfo(L, L);
    PyEval_RestoreThread(info->thstate);
    
    PyObject* evt = PyLong_FromLong(ar->event);
    PyObject* args;
    
    if (ar->event == LUA_HOOKLINE) {
        PyObject* line = PyLong_FromLong(ar->currentline);
        args = PyTuple_Pack(2, evt, line);
    } else {
        args = PyTuple_Pack(1, evt);
    }
    
    
    PyObject* res = pylua_call_pyobject(info, info->root->hook, args);
    info->thstate = PyEval_SaveThread();
    
    return NULL;
}


/**
 * Lua panic handler
 */
int pylua_panic(lua_State* L) {
    // set the python error    
    struct LuaStateInfo* info = pylua_get_stateinfo(L, L);
    
    if (!PyErr_Occurred()) {
        PyObject* value = pylua_get_as_unicode(L, -1);
        PyErr_SetObject(LuaFatalError, value);
        Py_DECREF(value);
    }

    lua_close(L);
    info->state = NULL;
    
    if (info->panic) {
        longjmp(info->panic->buf, 0);
    } else {
        fprintf(stderr, "PyLua PANIC: unprotected lua panic!\n");
    }
    return 0;
}


/**
 * Custom alloc function for lua
 * Checks if there's enough available memory before allocating
 */
void* pylua_alloc(LuaStateObject* self, void* ptr, size_t osize, size_t nsize) {
    // if the ptr is NULL, then osize is the type of the allocated buffer
    // (table, string, etc.)
    // we don't care about it, so we set it to zero
    if (ptr == NULL)
        osize = 0;

    // if nsize is 0, we're freeing a buffer.
    // lua knows its size, so we're just gonna free and remove the size
    if (nsize == 0) {
        free(ptr);
        self->mem -= osize;
        return NULL;
    }

    // we're here for allocating ; but do we have enough memory?
    if (self->mem + (nsize - osize) > self->limit)
        return NULL;

    // apparently we do have enough memory, so let's realloc
    ptr = realloc(ptr, nsize);

    if (ptr)
        self->mem += (nsize - osize);

    return ptr;
}


/**
 * Handler for garbage collecting of a python object in metatables
 */
int pylua_gc(lua_State* L) {
    PyObject** obj = (PyObject**)lua_touserdata(L, -1);
    Py_XDECREF(*obj);
    *obj = NULL;
    return 0;
}

/**
 * Handler for tostring (using repr) of a python object in metatables
 */
int pylua_tostring(lua_State* L) {
    PyObject** obj = (PyObject**)lua_touserdata(L, -1);
    PyObject* str = PyObject_Repr(*obj);
    pylua_push_pyobj(L, str);
    return 1;
}


/**
 * Handler for lua -> python calls
 */
int pylua_call_python(lua_State* L) {
    // NOTE: we don't have to clean the stack,
    // lua will do it for us anyway

    // get our stateobj
    struct LuaStateInfo* info = pylua_get_stateinfo(L, L);
    
    // okay, the issue here is that we need the GIL
    PyEval_RestoreThread(info->thstate);
    
    // get argc
    int argc = lua_gettop(L);
    
    // remove args, we don't care anymore about them
    PyObject* args = pylua_to_tuple(info, argc);
    if (!args) {
        lua_pushstring(L, "failed to convert to python args");
        lua_error(L);
        return 0;
    }
    
    PyObject* func = *(PyObject**)lua_touserdata(L, lua_upvalueindex(1));
    
    // attempt to call the function
    PyObject* res = pylua_call_pyobject(info, func, args);

    // arg handling:
    // if it's a tuple, unpack it, otherwise, single arg
    int err = 0;
    argc = 1;
    if (PyTuple_CheckExact(res)) {
        argc = pylua_push_tuple(L, res, 0);
        if (argc < 0)
            err = argc;
        
    } else {
        err = pylua_push_pyobj(L, res);
    }
    
    // save the thread back
    info->thstate = PyEval_SaveThread();
    
    // was there an error in arg handling
    if (err) {
        lua_pushstring(L, "python error on convert");
        lua_error(L);
        return 0;
    }
    
    return argc;
}