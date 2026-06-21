#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#elif _WIN32_WINNT < 0x0602
#error "GetCurrentThreadStackLimits requires _WIN32_WINNT >= 0x0602"
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "allocator.h"
#include "gc_config.h"
#include "gc.h"
#include "gc_internal.h"

#include <stdint.h>

typedef struct {
    bool initialized;
    DWORD owner_thread_id;
    uintptr_t stack_low;
    uintptr_t stack_high;
    size_t page_size;
    size_t memory_limit;
    size_t collection_request_count;
    size_t allocation_count;
    GCAllocation *allocations;
    IntervalNode *allocation_tree;
    GCStats stats;
    GCStatus status;
} GCState;

static GCState gc_state = {
    false,
    0,
    (uintptr_t)0,
    (uintptr_t)0,
    0,
    GC_DEFAULT_MEMORY_LIMIT,
    0,
    0,
    NULL,
    NULL,
    {0, 0, 0, 0},
    GC_STATUS_NOT_INITIALIZED
};

_Static_assert(sizeof(uintptr_t) == 8,
               "the collector currently supports only Windows x86-64");
_Static_assert(offsetof(GCAllocation, interval) == 0,
               "the interval node must be the first allocation field");

static bool gc_is_owner_thread(void)
{
    return gc_state.owner_thread_id == GetCurrentThreadId();
}

static size_t gc_grow_memory_limit(size_t required)
{
    size_t limit = gc_state.memory_limit;

    while (limit < required) {
        if (limit > SIZE_MAX / (size_t)2) {
            return required;
        }
        limit *= (size_t)2;
    }
    return limit;
}

static void gc_request_collection(void)
{
    if (gc_state.collection_request_count < SIZE_MAX) {
        ++gc_state.collection_request_count;
    }
}

static bool gc_prepare_memory_pressure(size_t reserved,
                                       size_t *next_limit)
{
    size_t required;

    if (next_limit == NULL
        || gc_state.stats.bytes_reserved > SIZE_MAX - reserved) {
        return false;
    }

    required = gc_state.stats.bytes_reserved + reserved;
    *next_limit = gc_state.memory_limit;
    if (required <= gc_state.memory_limit) {
        return true;
    }

    gc_request_collection();
    *next_limit = gc_grow_memory_limit(required);
    return true;
}

int gc_init(void)
{
    ULONG_PTR low;
    ULONG_PTR high;
    SYSTEM_INFO system_info;
    DWORD current_thread_id = GetCurrentThreadId();

    if (gc_state.initialized) {
        gc_state.status = gc_state.owner_thread_id == current_thread_id
                          ? GC_STATUS_ALREADY_INITIALIZED
                          : GC_STATUS_WRONG_THREAD;
        return GC_FAILURE;
    }

    GetCurrentThreadStackLimits(&low, &high);
    GetSystemInfo(&system_info);
    if (low >= high || system_info.dwPageSize == 0) {
        gc_state.status = GC_STATUS_INVALID_STACK_LIMITS;
        return GC_FAILURE;
    }

    gc_state.initialized = true;
    gc_state.owner_thread_id = current_thread_id;
    gc_state.stack_low = (uintptr_t)low;
    gc_state.stack_high = (uintptr_t)high;
    gc_state.page_size = (size_t)system_info.dwPageSize;
    gc_state.status = GC_STATUS_OK;
    return GC_SUCCESS;
}

void *gc_malloc(size_t size)
{
    GCAllocation *allocation;
    size_t next_limit;
    size_t reserved;

    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return NULL;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return NULL;
    }
    if (size == 0) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return NULL;
    }
    if (!gc_allocator_reservation_size(size, gc_state.page_size, &reserved)
        || gc_state.stats.bytes_requested > SIZE_MAX - size
        || gc_state.stats.bytes_live > SIZE_MAX - size
        || !gc_prepare_memory_pressure(reserved, &next_limit)) {
        gc_state.status = GC_STATUS_SIZE_OVERFLOW;
        return NULL;
    }

    allocation = gc_allocator_create(size, reserved);
    if (allocation == NULL) {
        gc_state.status = GC_STATUS_OUT_OF_MEMORY;
        return NULL;
    }
    if (!interval_tree_insert(&gc_state.allocation_tree,
                              &allocation->interval)) {
        gc_allocator_destroy_all(allocation);
        gc_state.status = GC_STATUS_INTERNAL_ERROR;
        return NULL;
    }

    allocation->next = gc_state.allocations;
    gc_state.allocations = allocation;
    ++gc_state.allocation_count;
    gc_state.stats.bytes_requested += size;
    gc_state.stats.bytes_live += size;
    gc_state.stats.bytes_reserved += reserved;
    gc_state.memory_limit = next_limit;
    gc_state.status = GC_STATUS_OK;
    return allocation->memory;
}

int gc_set_memory_limit(size_t bytes)
{
    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return GC_FAILURE;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return GC_FAILURE;
    }
    if (bytes == 0) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return GC_FAILURE;
    }

    gc_state.memory_limit = bytes;
    gc_state.status = GC_STATUS_OK;
    return GC_SUCCESS;
}

int gc_get_stats(GCStats *out)
{
    if (out == NULL) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return GC_FAILURE;
    }
    *out = (GCStats){0, 0, 0, 0};
    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return GC_FAILURE;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return GC_FAILURE;
    }

    *out = gc_state.stats;
    gc_state.status = GC_STATUS_OK;
    return GC_SUCCESS;
}

void gc_shutdown(void)
{
    bool canaries_valid;

    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return;
    }

    canaries_valid = gc_allocator_validate_all(gc_state.allocations);
    gc_allocator_destroy_all(gc_state.allocations);
    gc_state.initialized = false;
    gc_state.owner_thread_id = 0;
    gc_state.stack_low = (uintptr_t)0;
    gc_state.stack_high = (uintptr_t)0;
    gc_state.page_size = 0;
    gc_state.memory_limit = GC_DEFAULT_MEMORY_LIMIT;
    gc_state.collection_request_count = 0;
    gc_state.allocation_count = 0;
    gc_state.allocations = NULL;
    gc_state.allocation_tree = NULL;
    gc_state.stats = (GCStats){0, 0, 0, 0};
    gc_state.status = canaries_valid
                      ? GC_STATUS_OK
                      : GC_STATUS_CORRUPTED_MEMORY;
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

size_t gc_internal_allocation_count(void)
{
    return gc_state.allocation_count;
}

size_t gc_internal_memory_limit(void)
{
    return gc_state.memory_limit;
}

size_t gc_internal_collection_request_count(void)
{
    return gc_state.collection_request_count;
}

bool gc_internal_get_allocation_info(const void *address,
                                     size_t *requested,
                                     size_t *reserved)
{
    IntervalNode *interval;
    GCAllocation *allocation;

    if (requested != NULL) {
        *requested = 0;
    }
    if (reserved != NULL) {
        *reserved = 0;
    }
    if (!gc_state.initialized || !gc_is_owner_thread()
        || address == NULL || requested == NULL || reserved == NULL) {
        return false;
    }

    interval = interval_tree_find(gc_state.allocation_tree,
                                  (uintptr_t)address);
    if (interval == NULL) {
        return false;
    }

    allocation = (GCAllocation *)interval;
    *requested = allocation->requested_size;
    *reserved = allocation->reserved_size;
    return true;
}

static GCAllocation *gc_find_allocation(const void *address)
{
    IntervalNode *interval;

    if (!gc_state.initialized || !gc_is_owner_thread()
        || address == NULL) {
        return NULL;
    }
    interval = interval_tree_find(gc_state.allocation_tree,
                                  (uintptr_t)address);
    return (GCAllocation *)interval;
}

bool gc_internal_validate_canaries(const void *address)
{
    GCAllocation *allocation = gc_find_allocation(address);

    if (allocation == NULL) {
        return false;
    }
    if (!gc_allocator_validate_canaries(allocation)) {
        gc_state.status = GC_STATUS_CORRUPTED_MEMORY;
        return false;
    }
    gc_state.status = GC_STATUS_OK;
    return true;
}

bool gc_internal_corrupt_canary(const void *address, bool after_object)
{
    GCAllocation *allocation = gc_find_allocation(address);

    return gc_allocator_corrupt_canary(allocation, after_object);
}
