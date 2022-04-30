#include "pylua_userdata.h"
#include "pylua_object.h"

PyTypeObject LuaUserDataType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pylua.LuaUserData",
    .tp_doc = "Lua userdata",
    .tp_base = &LuaObjectType
};