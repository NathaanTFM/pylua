#include "pylua_python.h"
#include "pylua_exceptions.h"
#include "pylua_function.h"
#include "pylua_object.h"
#include "pylua_protect.h"
#include "pylua_table.h"
#include "pylua_thread.h"
#include "pylua_userdata.h"

#include <sys/timeb.h>
#include <setjmp.h>


/**
 * Converts the value at specified index to a python str.
 */
PyObject* pylua_get_as_unicode(lua_State* L, int idx) {
    size_t len;
    const char* str = lua_tolstring(L, idx, &len);
    return PyUnicode_DecodeUTF8(str, len, "replace");
}


#if !(INT_MIN == LONG_MIN && INT_MAX == LONG_MAX)
/**
 * Convert a PyLong to an int.
 * Behaves like PyLong_AsLong.
*/
int pylua_pylong_as_int(PyObject* obj) {
    // values does not fit a long, it won't fit an int
    long val = PyLong_AsLong(obj);
    if (val == -1 && PyErr_Occurred())
        return -1;

    if (val < INT_MIN || val > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "Python int too large to convert to C int");
        return -1;
    }

    return (int)val;
}
#endif


/**
 * Convert a PyObject to an equivalent lua object,
 * then push the result on the stack.
 *
 * Returns 0 if successful,
 *        -1 if an error occured, and sets a python exception
 */
int pylua_push_pyobj(lua_State* L, PyObject* obj) {
    // Is it None?
    if (obj == Py_None) {
        lua_pushnil(L);
        return 0;
    }

    // Is it a bool?
    if (obj == Py_False) {
        lua_pushboolean(L, 0);
        return 0;
    }
    if (obj == Py_True) {
        lua_pushboolean(L, 1);
        return 0;
    }

    // Is it an unicode string?
    if (PyUnicode_CheckExact(obj)) {
        // Converts it to a UTF-8 string, and push the result
        Py_ssize_t len;
        const char* buf = PyUnicode_AsUTF8AndSize(obj, &len);
        lua_pushlstring(L, buf, len);
        return 0;
    }

    // Is it a bytes object?
    if (PyBytes_CheckExact(obj)) {
        Py_ssize_t len;
        char* buf;
        PyBytes_AsStringAndSize(obj, &buf, &len);
        lua_pushlstring(L, buf, len);
        return 0;
    }

    // Is it an int?
    if (PyLong_CheckExact(obj)) {
#if LUA_INT_TYPE == LUA_INT_INT
        lua_Integer val = pylua_pylong_as_int(obj);
#elif LUA_INT_TYPE == LUA_INT_LONG
        lua_Integer val = PyLong_AsLong(obj);
#elif LUA_INT_TYPE == LUA_INT_LONGLONG
        lua_Integer val = PyLong_AsLongLong(obj);
#else
    #error unknown value for LUA_INT_TYPE
#endif

        // Any conversion error?
        if (val == -1 && PyErr_Occurred()) {
            // Maybe it will work with a double
            lua_Number fval = (lua_Number)PyLong_AsDouble(obj);

            if (fval == -1.0 && PyErr_Occurred())
                return -1;

            lua_pushnumber(L, fval);
            return 0;

        } else {
            // No conversion error, let's push the integer
            lua_pushinteger(L, val);
            return 0;
        }
    }

    // Is it a float?
    if (PyFloat_CheckExact(obj)) {
        // Not much error checking, as it uses a double internally
        // and it just gets casted automatically
        lua_Number val = (lua_Number)PyFloat_AS_DOUBLE(obj);
        lua_pushnumber(L, val);
        return 0;
    }

    // Is it a LuaObject?
    if (PyObject_TypeCheck(obj, &LuaObjectType)) {
        LuaObject* lobj = (LuaObject*)obj;

        // TODO: find a way to check if it's in the same state
        
        //if (lobj->sobj->info.state == L) {
            // It uses the same state, we can just push the ref
            lua_rawgeti(L, LUA_REGISTRYINDEX, lobj->ref);
            return 0;

        /*} else {
            // TODO: We do not use threads yet, but when we do,
            // we must add some check to prevent using a LuaObject
            // from a specific state to another state
            
            PyErr_SetString(LuaError, "cannot use a LuaObject from another LuaState");
            return -1;
        }*/
    }

    // Nope, we do not know what this is
    PyErr_Format(LuaError, "cannot convert a %s to a lua object", Py_TYPE(obj)->tp_name);
    return -1;
}


/**
 * Return a PyObject from a lua object on the specified index
 * from the stack. The stack is not modified.
 *
 * If the conversion failed, NULL is returned and a python exception is set
 */
PyObject* pylua_get_as_pyobj(struct LuaStateInfo* info, int idx) {
    // Let's get the type
    lua_State* L = info->state;
    int type = lua_type(L, idx);

    switch (type) {
        case LUA_TNIL:
            Py_RETURN_NONE;

        case LUA_TBOOLEAN:
            return PyBool_FromLong(lua_toboolean(L, idx));

        case LUA_TNUMBER:
#if LUA_VERSION_NUM >= 503
            // Lua 5.3 supports integers, so we're gonna check if we're dealing with one
            if (lua_isinteger(L, idx)) {
#   if LUA_INT_TYPE == LUA_INT_LONGLONG
                return PyLong_FromLongLong(lua_tointeger(L, idx));
#   elif LUA_INT_TYPE == LUA_INT_LONG || LUA_INT_TYPE = LUA_INT_INT
                return PyLong_FromLong((long)lua_tointeger(L, idx));
#   else
#       error unknown value for LUA_INT_TYPE
#   endif
            }
#endif
            // It's not an integer, so it must be a float, double or long double.
            // Python does not support long doubles or floats, so we're dealing with doubles no matter what
            return PyFloat_FromDouble((double)lua_tonumber(L, idx));

        case LUA_TSTRING: ;
            // We are using bytes, because lua strings does not have any kind of encoding
            size_t len;
            const char* val = lua_tolstring(L, idx, &len);
            return PyBytes_FromStringAndSize(val, len);

        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TLIGHTUSERDATA:
        case LUA_TUSERDATA:
        case LUA_TTHREAD:
            // Copy the value to the top of the stack
            lua_pushvalue(L, idx);

            // Creates a reference (which removes the element on the top)
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);

            PyObject* obj = pylua_alloc_luaobject(
                type == LUA_TTABLE          ?   &LuaTableType :
                type == LUA_TFUNCTION       ?   &LuaFunctionType :
                type == LUA_TLIGHTUSERDATA  ?   &LuaUserDataType :
                type == LUA_TUSERDATA       ?   &LuaUserDataType :
              /*type == LUA_TTHREAD         ?*/ &LuaThreadType,

                info->root, ref);

            return obj;

        default:
            PyErr_Format(LuaError, "cannot convert %s", lua_typename(L, type));
            return NULL;
    }
}

/**
 * Create a tuple from the last n element in the stack,
 * then pop those values.
 * If an error occured, the stack is untouched.
 */
PyObject* pylua_to_tuple(struct LuaStateInfo* info, int argc) {
    PyObject* res = PyTuple_New(argc);
    if (!res)
        return NULL;
    
    for (int i = 0; i < argc; i++) {
        PyObject* arg = pylua_get_as_pyobj(info, i-argc); 
        if (!arg) {
            Py_DECREF(res);
            return NULL;
        }
        
        if (PyTuple_SetItem(res, i, arg) < 0) {
            Py_DECREF(res);
            return NULL;
        }
    }
    
    lua_pop(info->state, argc);
    return res;
}


/**
 * Push the values from the tuple to the stack.
 * If an error occured, the stack is restored.
 * Returns -1 if an error occured ; else, returns the number of pushed values
 */
int pylua_push_tuple(lua_State* L, PyObject* obj, int startat) {
    if (obj == NULL) {
        // if the tuple is null, behave as if it was an empty tuple: it worked.
        return 0;
    }
    
    if (!PyTuple_CheckExact(obj)) {
        return -1;
    }
    
    int top = lua_gettop(L);
    
    // get the size of the tuple
    Py_ssize_t size = PyTuple_GET_SIZE(obj) - startat;
    
    // element count
    int count = (int)size;
    if (size > INT_MAX) {
        count = INT_MAX;
    }
    
    // push the elements
    for (int i = 0; i < count; i++) {
        int err = pylua_push_pyobj(L, PyTuple_GET_ITEM(obj, startat + i));
        if (err) {
            // if it fails, then give up on what we added to the stack so far
            lua_settop(L, top);
            return -1;
        }
    }
    
    return count;
}

/**
 * Allocate a LuaObject with the specified type,
 * lua state and ref
 */
PyObject* pylua_alloc_luaobject(PyTypeObject* type, LuaStateObject* sobj, int ref) {
    LuaObject* obj = (LuaObject*)type->tp_alloc(type, 0);
    obj->sobj = sobj;
    obj->ref = ref;
    Py_INCREF(obj->sobj);
    return (PyObject*)obj;
}


/**
 * Internal function for running lua code from python
 * Used by LuaFunction_call and LuaThread_call_function
 */
PyObject* pylua_call(struct LuaStateInfo* info, int funcref, PyObject* args, int startat) {
    int err;

    lua_State* L = info->state;
    PYLUA_PROTECT(info, NULL);
    
    int top = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, funcref); // push the function

    int argc = pylua_push_tuple(L, args, startat);
    if (argc < 0) { // error occured?
        PYLUA_UNPROTECT(info);
        return NULL;
    }
    
    // set the startat time if needed
    if (info->depth++ == 0)
        ftime(&info->startat);
    
    PYLUA_UNPROTECT(info);
    info->thstate = PyEval_SaveThread();
    
    // we need another kind of protection for this,
    // because we stole the python GIL,
    // and an unexpected panic error can happen
    struct PanicHandler* panic = pylua_push_panichandler(info);
    int fatal = setjmp(panic->buf);
    if (!fatal) {
        // if no error occured yet
        err = lua_pcall(L, argc, LUA_MULTRET, 0);
    }
    
    pylua_pop_panichandler(info);
    
    // restore the thread
    PyEval_RestoreThread(info->thstate);
    info->thstate = NULL;
    
    // now we're good 
    info->depth--;
    
    // we can stop here if a fatal error happened
    if (fatal)
        return NULL;
    
    // we can get back to a regular protection
    PYLUA_PROTECT(info, NULL);
    
    // error handling
    if (err) {
        if (!PyErr_Occurred()) {
            PyObject* err = pylua_get_as_unicode(L, -1);
            PyErr_SetObject(LuaRuntimeError, err);
            Py_DECREF(err);
        }
        // remove the error from the stack
        lua_pop(L, 1);
        
        PYLUA_UNPROTECT(info);
        return NULL;
    }


    // Now we need to get our arguments
    int len = lua_gettop(L) - top;
    PyObject* res = pylua_to_tuple(info, len);     // no need to error check this

    // remove everything, restore the stack
    PYLUA_UNPROTECT(info);
    
    return res;
}