#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#elif _WIN32_WINNT < 0x0602
#error "GetCurrentThreadStackLimits requires _WIN32_WINNT >= 0x0602"
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "gc.h"
#include "gc_internal.h"

#include <stdint.h>

typedef struct {
    bool initialized;
    DWORD owner_thread_id;
    uintptr_t stack_low;
    uintptr_t stack_high;
    GCStatus status;
} GCState;

static GCState gc_state = {
    false,
    0,
    (uintptr_t)0,
    (uintptr_t)0,
    GC_STATUS_NOT_INITIALIZED
};

_Static_assert(sizeof(uintptr_t) == 8,
               "the collector currently supports only Windows x86-64");

static bool gc_is_owner_thread(void)
{
    return gc_state.owner_thread_id == GetCurrentThreadId();
}

int gc_init(void)
{
    ULONG_PTR low;
    ULONG_PTR high;
    DWORD current_thread_id = GetCurrentThreadId();

    if (gc_state.initialized) {
        gc_state.status = gc_state.owner_thread_id == current_thread_id
                          ? GC_STATUS_ALREADY_INITIALIZED
                          : GC_STATUS_WRONG_THREAD;
        return GC_FAILURE;
    }

    GetCurrentThreadStackLimits(&low, &high);
    if (low >= high) {
        gc_state.status = GC_STATUS_INVALID_STACK_LIMITS;
        return GC_FAILURE;
    }

    gc_state.initialized = true;
    gc_state.owner_thread_id = current_thread_id;
    gc_state.stack_low = (uintptr_t)low;
    gc_state.stack_high = (uintptr_t)high;
    gc_state.status = GC_STATUS_OK;
    return GC_SUCCESS;
}

void gc_shutdown(void)
{
    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return;
    }

    gc_state.initialized = false;
    gc_state.owner_thread_id = 0;
    gc_state.stack_low = (uintptr_t)0;
    gc_state.stack_high = (uintptr_t)0;
    gc_state.status = GC_STATUS_OK;
}

bool gc_is_initialized(void)
{
    return gc_state.initialized;
}

GCStatus gc_get_status(void)
{
    return gc_state.status;
}

bool gc_internal_get_stack_limits(uintptr_t *low, uintptr_t *high)
{
    if (low != NULL) {
        *low = (uintptr_t)0;
    }
    if (high != NULL) {
        *high = (uintptr_t)0;
    }
    if (low == NULL || high == NULL) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return false;
    }
    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return false;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return false;
    }

    *low = gc_state.stack_low;
    *high = gc_state.stack_high;
    gc_state.status = GC_STATUS_OK;
    return true;
}
