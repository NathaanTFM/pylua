#include "pylua_object.h"
#include "pylua_protect.h"

/**
 * Compute an hash using the lua_topointer function,
 * then return it.
 */
Py_hash_t LuaObject_hash(LuaObject* self) {
    PYLUA_CHECK(L, &self->sobj->info, -1);

    lua_rawgeti(L, LUA_REGISTRYINDEX, self->ref);
    const void* ptr = lua_topointer(L, -1);
    lua_pop(L, 1);
    return (Py_hash_t)ptr;
}

/**
 * Implementation of richcompare for LuaObject,
 * allows us to check if two LuaObjects are equals
 * based on their hash.
 *
 * This is only reliable for two LuaObject
 * from a same state
 */
PyObject* LuaObject_richcompare(LuaObject* self, PyObject* other, int op) {
    PYLUA_CHECK(L, &self->sobj->info, NULL);

    // We only work with Py_EQ and Py_NE
    if (op == Py_EQ || op == Py_NE) {
        int eq = Py_NE;
        if (PyObject_TypeCheck(other, &LuaObjectType)) {
            if (LuaObject_hash(self) == LuaObject_hash((LuaObject*)other)) {
                // Both LuaObject with equal hashes ; it is equals.
                eq = Py_EQ;
            }
        }

        // We gotta return the correct value according to the op.
        if (op == eq) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }

    } else {
        // The other operations are not implemented (gt, lt, ge, le)
        return Py_NotImplemented;
    }
}


/**
 * Handle the deallocation of LuaObject
 */
static void LuaObject_dealloc(LuaObject* self) {
    // the unref (with protect)
    if (self->sobj->info.state) {
        //PYLUA_TRY(self->sobj,
            luaL_unref(self->sobj->info.state, LUA_REGISTRYINDEX, self->ref);
        //);
    }
    
    // decref the related stateobj
    Py_DECREF(self->sobj);

    // free ourselves
    Py_TYPE(self)->tp_free((PyObject*)self);
}


PyTypeObject LuaObjectType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pylua.LuaObject",
    .tp_doc = "Lua object",
    .tp_basicsize = sizeof(LuaObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = NULL,
    .tp_dealloc = (destructor)LuaObject_dealloc,
    .tp_richcompare = (richcmpfunc)LuaObject_richcompare,
    .tp_hash = (hashfunc)LuaObject_hash
};