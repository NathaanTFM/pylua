#ifndef PYLUA_PROTECT_H
#define PYLUA_PROTECT_H

#include "pylua.h"
#include "pylua_exceptions.h"
#include "pylua_stateinfo.h"

#include <setjmp.h>

struct PanicHandler {
    jmp_buf buf;
    struct PanicHandler* next;
}; 

int pylua_get_panichandler_depth(struct LuaStateInfo* info);
void pylua_pop_panichandler(struct LuaStateInfo* info);
struct PanicHandler* pylua_push_panichandler(struct LuaStateInfo* info);

// Protect from lua panic errors
// We are going to allow only a single panic handler,
// which should make everything easier, but reduce traceback info.

#define PYLUA_PROTECT(s, r) \
    do { \
        if (setjmp(pylua_push_panichandler(s)->buf)) { \
            pylua_pop_panichandler(s); \
            return r; \
        } \
    } while (0)

#define PYLUA_UNPROTECT(s) \
    pylua_pop_panichandler(s);
    
/*
    NOTE: This only works if there's no protection ;
    otherwise, it causes the panic function to run.
*/
    
#define PYLUA_TRY(s, c) \
    do { \
        if (!setjmp(pylua_push_panichandler(s)->buf)) { \
            c \
        } else { \
            PyErr_Clear(); \
        } \
        pylua_pop_panichandler(s); \
    } while (0)

#define PYLUA_CHECK(L, s, r) \
    lua_State* L; \
    do { \
        L = (s)->state; \
        if (!L) { \
            PyErr_SetString(LuaFatalError, "lua state is dead"); \
            return r; \
        } \
    } while (0)
        
#define PYLUA_ERROR(s, m) \
    do { \
        if ((s)->panic) { \
            longjmp((s)->buf, 1); \
        } else { \
            lua_pushstring((s)->state, m); \
            lua_error((s)->state); \
        } \
    } while (0)

#endif