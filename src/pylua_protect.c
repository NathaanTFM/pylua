#include "pylua_protect.h"

/**
 * Get panic handler depth
 */
int pylua_get_panichandler_depth(struct LuaStateInfo* info) {
    int count = 0;
    struct PanicHandler* handler = info->panic;
    while (handler) {
        count++;
        handler = handler->next;
    }
    return count;
}

/**
 * Push a new panic handler
 */
struct PanicHandler* pylua_push_panichandler(struct LuaStateInfo* info) {
    struct PanicHandler* handler = malloc(sizeof *handler);
    handler->next = info->panic;
    info->panic = handler;
    return handler;
}

/**
 * Pop panic handler
 */
void pylua_pop_panichandler(struct LuaStateInfo* info) {
    struct PanicHandler* handler = info->panic->next;
    free(info->panic);
    info->panic = handler;
}