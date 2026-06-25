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
#include "marker.h"
#include "old_pages.h"
#include "register_roots.h"
#include "roots.h"
#include "stack_roots.h"
#include "sweeper.h"

#include <stdint.h>

typedef struct {
    bool initialized;
    DWORD owner_thread_id;
    PVOID barrier_handler;
    uintptr_t stack_low;
    uintptr_t stack_high;
    size_t page_size;
    size_t memory_limit;
    size_t promotion_threshold;
    size_t minor_collections_since_major;
    size_t collection_request_count;
    size_t allocation_count;
    size_t root_count;
    GCAllocation *allocations;
    IntervalNode *allocation_tree;
    GCOldPage *old_pages;
    GCRoot *roots;
    GCStats stats;
    GCStatus status;
} GCState;

static GCState gc_state = {
    false,
    0,
    NULL,
    (uintptr_t)0,
    (uintptr_t)0,
    0,
    GC_DEFAULT_MEMORY_LIMIT,
    GC_DEFAULT_PROMOTION_THRESHOLD,
    0,
    0,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    {0},
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

#if GC_OLD_PAGES_PROTECTION_SUPPORTED
static LONG CALLBACK gc_write_barrier_handler(
    EXCEPTION_POINTERS *exception_info)
{
    EXCEPTION_RECORD *record;

    if (exception_info == NULL) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    record = exception_info->ExceptionRecord;
    if (record == NULL
        || record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION
        || record->NumberParameters < 2
        || record->ExceptionInformation[0] != (ULONG_PTR)1) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (!gc_old_pages_unprotect_for_write(
            gc_state.old_pages,
            (const void *)record->ExceptionInformation[1])) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}
#endif

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

static bool gc_should_collect_major(void)
{
    return gc_state.minor_collections_since_major
           >= GC_DEFAULT_MAJOR_COLLECTION_INTERVAL;
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

static void gc_clear_marks(void)
{
    GCAllocation *allocation = gc_state.allocations;

    while (allocation != NULL) {
        allocation->marked = false;
        allocation = allocation->next;
    }
}

static GCStatus gc_mark_explicit_roots(GCMarkQueue *queue)
{
    GCRoot *root = gc_state.roots;

    while (root != NULL) {
        IntervalNode *interval = interval_tree_find(
            gc_state.allocation_tree, (uintptr_t)*root->location);

        if (interval != NULL) {
            GCMarkQueueResult result = gc_mark_queue_push(
                queue, (GCAllocation *)interval);

            if (result == GC_MARK_QUEUE_OUT_OF_MEMORY) {
                return GC_STATUS_OUT_OF_MEMORY;
            }
            if (result == GC_MARK_QUEUE_INVALID) {
                return GC_STATUS_INTERNAL_ERROR;
            }
        }
        root = root->next;
    }
    return GC_STATUS_OK;
}

static GCStatus gc_mark_register_roots(GCMarkQueue *queue)
{
    GCRegisterScanResult result;

    if (gc_state.allocation_tree == NULL) {
        return GC_STATUS_OK;
    }

    result = gc_register_roots_scan(gc_state.allocation_tree, queue);
    if (result == GC_REGISTER_SCAN_OUT_OF_MEMORY) {
        return GC_STATUS_OUT_OF_MEMORY;
    }
    if (result != GC_REGISTER_SCAN_OK) {
        return GC_STATUS_INTERNAL_ERROR;
    }
    return GC_STATUS_OK;
}

static GCStatus gc_mark_stack_roots(GCMarkQueue *queue)
{
    GCStackScanResult result;

    if (gc_state.allocation_tree == NULL) {
        return GC_STATUS_OK;
    }

    result = gc_stack_scan(gc_state.stack_low, gc_state.stack_high,
                           gc_state.allocation_tree, queue);
    if (result == GC_STACK_SCAN_OUT_OF_MEMORY) {
        return GC_STATUS_OUT_OF_MEMORY;
    }
    if (result != GC_STACK_SCAN_OK) {
        return GC_STATUS_INTERNAL_ERROR;
    }
    return GC_STATUS_OK;
}

static GCStatus gc_mark_roots(GCMarkQueue *queue)
{
    GCStatus status = gc_mark_explicit_roots(queue);

    if (status == GC_STATUS_OK) {
        status = gc_mark_register_roots(queue);
    }
    if (status == GC_STATUS_OK) {
        status = gc_mark_stack_roots(queue);
    }
    return status;
}

static bool gc_old_allocation_needs_minor_scan(
    const GCAllocation *allocation)
{
    const GCOldPage *page;

    if (allocation == NULL
        || allocation->generation != GC_GENERATION_OLD) {
        return false;
    }
#if !GC_OLD_PAGES_PROTECTION_SUPPORTED
    return true;
#endif
    if (!allocation->dedicated_mapping) {
        return true;
    }
    page = gc_old_pages_find(gc_state.old_pages, allocation->memory);
    return page == NULL || page->dirty;
}

static GCStatus gc_mark_remembered_old_roots(GCMarkQueue *queue)
{
    GCAllocation *allocation = gc_state.allocations;

    while (allocation != NULL) {
        if (gc_old_allocation_needs_minor_scan(allocation)) {
            GCMarkQueueResult result = gc_mark_queue_push(queue,
                                                          allocation);

            if (result == GC_MARK_QUEUE_OUT_OF_MEMORY) {
                return GC_STATUS_OUT_OF_MEMORY;
            }
            if (result == GC_MARK_QUEUE_INVALID) {
                return GC_STATUS_INTERNAL_ERROR;
            }
        }
        allocation = allocation->next;
    }
    return GC_STATUS_OK;
}

static GCStatus gc_process_mark_queue(GCMarkQueue *queue,
                                      size_t *examined)
{
    GCAllocation *allocation;

    *examined = 0;
    while ((allocation = gc_mark_queue_pop(queue)) != NULL) {
        GCMarkScanResult result;

        if (*examined == SIZE_MAX) {
            return GC_STATUS_SIZE_OVERFLOW;
        }
        ++*examined;
        result = gc_mark_scan_object(allocation,
                                     gc_state.allocation_tree, queue);
        if (result == GC_MARK_SCAN_OUT_OF_MEMORY) {
            return GC_STATUS_OUT_OF_MEMORY;
        }
        if (result != GC_MARK_SCAN_OK) {
            return GC_STATUS_INTERNAL_ERROR;
        }
    }
    return GC_STATUS_OK;
}

static GCStatus gc_rebuild_old_pages(void)
{
    GCOldPageResult result = gc_old_pages_rebuild(&gc_state.old_pages,
                                                  gc_state.allocations);

    if (result == GC_OLD_PAGE_OUT_OF_MEMORY) {
        return GC_STATUS_OUT_OF_MEMORY;
    }
    if (result != GC_OLD_PAGE_OK) {
        return GC_STATUS_INTERNAL_ERROR;
    }
    if (!gc_old_pages_protect(gc_state.old_pages)) {
        return GC_STATUS_INTERNAL_ERROR;
    }
    return GC_STATUS_OK;
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
#if GC_OLD_PAGES_PROTECTION_SUPPORTED
    gc_state.barrier_handler = AddVectoredExceptionHandler(
        1, gc_write_barrier_handler);
    if (gc_state.barrier_handler == NULL) {
        gc_state.status = GC_STATUS_INTERNAL_ERROR;
        return GC_FAILURE;
    }
#endif

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
    size_t fragmentation;
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
        || reserved < size
        || gc_state.stats.bytes_requested > SIZE_MAX - size
        || gc_state.stats.bytes_live > SIZE_MAX - size
        || gc_state.stats.bytes_internal_fragmentation
           > SIZE_MAX - (reserved - size)
        || !gc_prepare_memory_pressure(reserved, &next_limit)) {
        gc_state.status = GC_STATUS_SIZE_OVERFLOW;
        return NULL;
    }
    fragmentation = reserved - size;

    allocation = gc_allocator_create(size, reserved);
    if (allocation == NULL) {
        gc_state.status = GC_STATUS_OUT_OF_MEMORY;
        return NULL;
    }
    if (!interval_tree_insert(&gc_state.allocation_tree,
                              &allocation->interval)) {
        (void)gc_allocator_destroy_one(allocation);
        gc_state.status = GC_STATUS_INTERNAL_ERROR;
        return NULL;
    }

    allocation->next = gc_state.allocations;
    gc_state.allocations = allocation;
    ++gc_state.allocation_count;
    gc_state.stats.bytes_requested += size;
    gc_state.stats.bytes_live += size;
    gc_state.stats.bytes_reserved += reserved;
    gc_state.stats.bytes_internal_fragmentation += fragmentation;
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

int gc_set_promotion_threshold(size_t survivals)
{
    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return GC_FAILURE;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return GC_FAILURE;
    }
    if (survivals == 0) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return GC_FAILURE;
    }

    gc_state.promotion_threshold = survivals;
    gc_state.status = GC_STATUS_OK;
    return GC_SUCCESS;
}

int gc_get_stats(GCStats *out)
{
    if (out == NULL) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return GC_FAILURE;
    }
    *out = (GCStats){0};
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

int gc_add_root(void **root)
{
    GCRootResult result;

    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return GC_FAILURE;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return GC_FAILURE;
    }
    if (root == NULL) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return GC_FAILURE;
    }
    if (gc_state.root_count == SIZE_MAX) {
        gc_state.status = GC_STATUS_SIZE_OVERFLOW;
        return GC_FAILURE;
    }

    result = gc_roots_add(&gc_state.roots, root);
    if (result != GC_ROOT_RESULT_OK) {
        gc_state.status = result == GC_ROOT_RESULT_DUPLICATE
                          ? GC_STATUS_DUPLICATE_ROOT
                          : GC_STATUS_OUT_OF_MEMORY;
        return GC_FAILURE;
    }
    ++gc_state.root_count;
    gc_state.status = GC_STATUS_OK;
    return GC_SUCCESS;
}

int gc_remove_root(void **root)
{
    GCRootResult result;

    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return GC_FAILURE;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return GC_FAILURE;
    }
    if (root == NULL) {
        gc_state.status = GC_STATUS_INVALID_ARGUMENT;
        return GC_FAILURE;
    }

    result = gc_roots_remove(&gc_state.roots, root);
    if (result != GC_ROOT_RESULT_OK) {
        gc_state.status = GC_STATUS_ROOT_NOT_FOUND;
        return GC_FAILURE;
    }
    --gc_state.root_count;
    gc_state.status = GC_STATUS_OK;
    return GC_SUCCESS;
}

void gc_collect(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    GCMarkQueue queue;
    GCStatus status;
    size_t before_count;
    size_t examined;
    bool collect_major;

    if (!gc_state.initialized) {
        gc_state.status = GC_STATUS_NOT_INITIALIZED;
        return;
    }
    if (!gc_is_owner_thread()) {
        gc_state.status = GC_STATUS_WRONG_THREAD;
        return;
    }
    collect_major = gc_should_collect_major();
    if (gc_state.stats.collection_count == SIZE_MAX
        || (collect_major
            && gc_state.stats.major_collection_count == SIZE_MAX)
        || (!collect_major
            && (gc_state.stats.minor_collection_count == SIZE_MAX
                || gc_state.minor_collections_since_major == SIZE_MAX))) {
        gc_state.status = GC_STATUS_SIZE_OVERFLOW;
        return;
    }
    if (!QueryPerformanceFrequency(&frequency)
        || frequency.QuadPart <= 0
        || !QueryPerformanceCounter(&start)) {
        gc_state.status = GC_STATUS_TIMER_ERROR;
        return;
    }
    if (!gc_allocator_validate_all(gc_state.allocations)) {
        gc_state.status = GC_STATUS_CORRUPTED_MEMORY;
        return;
    }

    gc_mark_queue_init(&queue);
    status = gc_mark_roots(&queue);
    if (!collect_major && status == GC_STATUS_OK) {
        status = gc_mark_remembered_old_roots(&queue);
    }
    if (status == GC_STATUS_OK) {
        status = gc_process_mark_queue(&queue, &examined);
    }
    gc_mark_queue_destroy(&queue);
    if (status != GC_STATUS_OK) {
        gc_clear_marks();
        gc_state.status = status;
        return;
    }

    before_count = gc_state.allocation_count;
    if ((collect_major
         ? gc_sweep(&gc_state.allocations, &gc_state.allocation_tree,
                    &gc_state.allocation_count, &gc_state.stats,
                    gc_state.promotion_threshold)
         : gc_sweep_young(&gc_state.allocations, &gc_state.allocation_tree,
                          &gc_state.allocation_count, &gc_state.stats,
                          gc_state.promotion_threshold))
        != GC_SWEEP_OK) {
        gc_clear_marks();
        gc_state.status = GC_STATUS_INTERNAL_ERROR;
        return;
    }
    status = gc_rebuild_old_pages();
    if (status != GC_STATUS_OK) {
        gc_state.status = status;
        return;
    }
    if (!QueryPerformanceCounter(&end) || end.QuadPart < start.QuadPart) {
        gc_state.status = GC_STATUS_TIMER_ERROR;
        return;
    }

    ++gc_state.stats.collection_count;
    if (collect_major) {
        ++gc_state.stats.major_collection_count;
        gc_state.minor_collections_since_major = 0;
        gc_state.stats.last_major_pause_ticks =
            (uint64_t)(end.QuadPart - start.QuadPart);
    } else {
        ++gc_state.stats.minor_collection_count;
        ++gc_state.minor_collections_since_major;
        gc_state.stats.last_minor_pause_ticks =
            (uint64_t)(end.QuadPart - start.QuadPart);
    }
    gc_state.stats.last_objects_examined = examined;
    gc_state.stats.last_objects_collected =
        before_count - gc_state.allocation_count;
    gc_state.stats.last_pause_ticks =
        (uint64_t)(end.QuadPart - start.QuadPart);
    gc_state.stats.performance_frequency =
        (uint64_t)frequency.QuadPart;
    gc_state.status = GC_STATUS_OK;
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
    if (gc_state.barrier_handler != NULL) {
        (void)RemoveVectoredExceptionHandler(gc_state.barrier_handler);
    }
    gc_roots_destroy_all(gc_state.roots);
    gc_old_pages_destroy(gc_state.old_pages);
    gc_allocator_destroy_all(gc_state.allocations);
    gc_state.initialized = false;
    gc_state.owner_thread_id = 0;
    gc_state.barrier_handler = NULL;
    gc_state.stack_low = (uintptr_t)0;
    gc_state.stack_high = (uintptr_t)0;
    gc_state.page_size = 0;
    gc_state.memory_limit = GC_DEFAULT_MEMORY_LIMIT;
    gc_state.promotion_threshold = GC_DEFAULT_PROMOTION_THRESHOLD;
    gc_state.minor_collections_since_major = 0;
    gc_state.collection_request_count = 0;
    gc_state.allocation_count = 0;
    gc_state.root_count = 0;
    gc_state.allocations = NULL;
    gc_state.allocation_tree = NULL;
    gc_state.old_pages = NULL;
    gc_state.roots = NULL;
    gc_state.stats = (GCStats){0};
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

size_t gc_internal_promotion_threshold(void)
{
    return gc_state.promotion_threshold;
}

size_t gc_internal_old_page_count(void)
{
    return gc_old_pages_count(gc_state.old_pages);
}

bool gc_internal_get_old_page_info(const void *address,
                                   void **base,
                                   size_t *size)
{
    const GCOldPage *page;

    if (base != NULL) {
        *base = NULL;
    }
    if (size != NULL) {
        *size = 0;
    }
    if (!gc_state.initialized || !gc_is_owner_thread()
        || address == NULL || base == NULL || size == NULL) {
        return false;
    }

    page = gc_old_pages_find(gc_state.old_pages, address);
    if (page == NULL) {
        return false;
    }
    *base = page->base;
    *size = page->size;
    return true;
}

bool gc_internal_get_old_page_state(const void *address,
                                    bool *dirty,
                                    bool *is_protected)
{
    const GCOldPage *page;

    if (dirty != NULL) {
        *dirty = false;
    }
    if (is_protected != NULL) {
        *is_protected = false;
    }
    if (!gc_state.initialized || !gc_is_owner_thread()
        || address == NULL || dirty == NULL || is_protected == NULL) {
        return false;
    }

    page = gc_old_pages_find(gc_state.old_pages, address);
    if (page == NULL) {
        return false;
    }
    *dirty = page->dirty;
    *is_protected = page->protected;
    return true;
}

size_t gc_internal_collection_request_count(void)
{
    return gc_state.collection_request_count;
}

size_t gc_internal_root_count(void)
{
    return gc_state.root_count;
}

bool gc_internal_get_root_value(void **root, void **value)
{
    if (value != NULL) {
        *value = NULL;
    }
    if (!gc_state.initialized || !gc_is_owner_thread()
        || root == NULL || value == NULL) {
        return false;
    }
    return gc_roots_get_value(gc_state.roots, root, value);
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
