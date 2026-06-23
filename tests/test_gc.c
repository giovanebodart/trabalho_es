#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "allocator.h"
#include "gc_config.h"
#include "gc.h"
#include "gc_internal.h"
#include "test.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#define GC_TEST_NOINLINE __attribute__((noinline))

static DWORD WINAPI shutdown_from_other_thread(LPVOID context)
{
    (void)context;
    gc_shutdown();
    return 0;
}

static DWORD WINAPI set_limit_from_other_thread(LPVOID context)
{
    size_t limit = *(const size_t *)context;

    (void)gc_set_memory_limit(limit);
    return 0;
}

static DWORD WINAPI add_root_from_other_thread(LPVOID context)
{
    (void)gc_add_root((void **)context);
    return 0;
}

static DWORD WINAPI collect_from_other_thread(LPVOID context)
{
    (void)context;
    gc_collect();
    return 0;
}

static int test_invalid_state_transitions(void)
{
    gc_shutdown();
    TEST_ASSERT(!gc_is_initialized());
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    TEST_ASSERT(gc_is_initialized());
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());

    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_init());
    TEST_ASSERT(gc_is_initialized());
    TEST_ASSERT_EQ_INT(GC_STATUS_ALREADY_INITIALIZED, gc_get_status());

    gc_shutdown();
    TEST_ASSERT(!gc_is_initialized());
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());

    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    return EXIT_SUCCESS;
}

static int test_stack_limits_and_thread_ownership(void)
{
    uintptr_t low;
    uintptr_t high;
    uintptr_t expected_low;
    uintptr_t expected_high;
    uintptr_t stack_address;
    int stack_marker = 0;
    HANDLE thread;
    DWORD wait_result;
    BOOL close_result;

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    TEST_ASSERT(gc_internal_get_stack_limits(&low, &high));
    expected_low = low;
    expected_high = high;
    stack_address = (uintptr_t)&stack_marker;
    TEST_ASSERT(low < high);
    TEST_ASSERT(stack_address >= low && stack_address < high);
    TEST_ASSERT(!gc_internal_get_stack_limits(NULL, &high));
    TEST_ASSERT_EQ_INT(GC_STATUS_INVALID_ARGUMENT, gc_get_status());
    TEST_ASSERT(gc_internal_get_stack_limits(&low, &high));
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());

    thread = CreateThread(NULL, 0, shutdown_from_other_thread,
                          NULL, 0, NULL);
    TEST_ASSERT(thread != NULL);
    wait_result = WaitForSingleObject(thread, INFINITE);
    close_result = CloseHandle(thread);
    TEST_ASSERT(wait_result == WAIT_OBJECT_0);
    TEST_ASSERT(close_result);
    TEST_ASSERT(gc_is_initialized());
    TEST_ASSERT_EQ_INT(GC_STATUS_WRONG_THREAD, gc_get_status());
    TEST_ASSERT(gc_internal_get_stack_limits(&low, &high));
    TEST_ASSERT(low == expected_low);
    TEST_ASSERT(high == expected_high);

    gc_shutdown();
    TEST_ASSERT(!gc_is_initialized());
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());

    low = UINTPTR_MAX;
    high = UINTPTR_MAX;
    TEST_ASSERT(!gc_internal_get_stack_limits(&low, &high));
    TEST_ASSERT(low == (uintptr_t)0);
    TEST_ASSERT(high == (uintptr_t)0);
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    return EXIT_SUCCESS;
}

static int test_virtual_alloc_objects(void)
{
    SYSTEM_INFO system_info;
    GCStats stats;
    unsigned char *small;
    unsigned char *large;
    size_t requested;
    size_t reserved;
    size_t large_requested;
    size_t small_reserved;
    size_t large_reserved;
    size_t impossible_size;
    size_t initial_limit;
    MEMORY_BASIC_INFORMATION memory_info;
    MEMORY_BASIC_INFORMATION small_info;
    MEMORY_BASIC_INFORMATION large_info;

    GetSystemInfo(&system_info);
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    TEST_ASSERT(gc_malloc(0) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_INVALID_ARGUMENT, gc_get_status());
    TEST_ASSERT(gc_malloc(SIZE_MAX) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_SIZE_OVERFLOW, gc_get_status());
    initial_limit = gc_internal_memory_limit();
    impossible_size = SIZE_MAX
                      - (size_t)system_info.dwPageSize * (size_t)2;
    TEST_ASSERT(gc_malloc(impossible_size) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_OUT_OF_MEMORY, gc_get_status());
    TEST_ASSERT(gc_internal_collection_request_count() == (size_t)1);
    TEST_ASSERT(gc_internal_memory_limit() == initial_limit);

    small = gc_malloc(17);
    large = gc_malloc((size_t)system_info.dwPageSize + (size_t)1);
    TEST_ASSERT(small != NULL);
    TEST_ASSERT(large != NULL);
    TEST_ASSERT((uintptr_t)small % _Alignof(long double) == (uintptr_t)0);
    TEST_ASSERT(VirtualQuery(small, &small_info, sizeof small_info)
                == sizeof small_info);
    TEST_ASSERT(VirtualQuery(large, &large_info, sizeof large_info)
                == sizeof large_info);
    TEST_ASSERT(small_info.AllocationBase == large_info.AllocationBase);
    TEST_ASSERT(gc_internal_allocation_count() == (size_t)2);
    TEST_ASSERT(gc_internal_get_allocation_info(small + 16,
                                                &requested,
                                                &small_reserved));
    TEST_ASSERT(requested == (size_t)17);
    TEST_ASSERT(gc_internal_get_allocation_info(large,
                                                &requested,
                                                &large_reserved));
    TEST_ASSERT(requested == (size_t)system_info.dwPageSize + (size_t)1);
    large_requested = requested;
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_get_stats(&stats));
    TEST_ASSERT(stats.bytes_requested
                == (size_t)system_info.dwPageSize + (size_t)18);
    TEST_ASSERT(stats.bytes_live == stats.bytes_requested);
    TEST_ASSERT(stats.bytes_reserved == small_reserved + large_reserved);
    TEST_ASSERT(stats.bytes_collected == (size_t)0);
    reserved = small_reserved;
    TEST_ASSERT(!gc_internal_get_allocation_info(small + 17,
                                                 &requested, &reserved));
    TEST_ASSERT(reserved == (size_t)0);

    TEST_ASSERT(small[0] == 0 && small[16] == 0);
    TEST_ASSERT(large[0] == 0
                && large[large_requested - (size_t)1] == 0);
    memset(small, 0x5a, 17);
    memset(large, 0xa5, large_requested);
    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    TEST_ASSERT(gc_internal_allocation_count() == (size_t)0);
    TEST_ASSERT(VirtualQuery(small, &memory_info, sizeof memory_info)
                == sizeof memory_info);
    TEST_ASSERT(memory_info.State == MEM_FREE);
    return EXIT_SUCCESS;
}

static int test_memory_pressure(void)
{
    SYSTEM_INFO system_info;
    size_t default_limit;
    size_t page_size;
    size_t tiny_reserved;
    HANDLE thread;
    DWORD wait_result;

    GetSystemInfo(&system_info);
    page_size = (size_t)system_info.dwPageSize;
    TEST_ASSERT(gc_allocator_reservation_size(1, page_size,
                                              &tiny_reserved));
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    default_limit = gc_internal_memory_limit();
    TEST_ASSERT(default_limit == GC_DEFAULT_MEMORY_LIMIT);
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_set_memory_limit(0));
    TEST_ASSERT_EQ_INT(GC_STATUS_INVALID_ARGUMENT, gc_get_status());

    thread = CreateThread(NULL, 0, set_limit_from_other_thread,
                          &page_size, 0, NULL);
    TEST_ASSERT(thread != NULL);
    wait_result = WaitForSingleObject(thread, INFINITE);
    TEST_ASSERT(CloseHandle(thread));
    TEST_ASSERT(wait_result == WAIT_OBJECT_0);
    TEST_ASSERT_EQ_INT(GC_STATUS_WRONG_THREAD, gc_get_status());

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_set_memory_limit(tiny_reserved));
    TEST_ASSERT(gc_malloc(1) != NULL);
    TEST_ASSERT(gc_internal_collection_request_count() == (size_t)0);
    TEST_ASSERT(gc_internal_memory_limit() == tiny_reserved);

    TEST_ASSERT(gc_malloc(1) != NULL);
    TEST_ASSERT(gc_internal_collection_request_count() == (size_t)1);
    TEST_ASSERT(gc_internal_memory_limit() == tiny_reserved * (size_t)2);

    TEST_ASSERT(gc_malloc(1) != NULL);
    TEST_ASSERT(gc_internal_collection_request_count() == (size_t)2);
    TEST_ASSERT(gc_internal_memory_limit() == tiny_reserved * (size_t)4);
    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    TEST_ASSERT(gc_internal_collection_request_count() == (size_t)0);
    TEST_ASSERT(gc_internal_memory_limit() == default_limit);
    return EXIT_SUCCESS;
}

static int test_debug_canaries(void)
{
    unsigned char *object;

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    object = gc_malloc(32);
    TEST_ASSERT(object != NULL);
    TEST_ASSERT(gc_internal_validate_canaries(object));

    TEST_ASSERT(gc_internal_corrupt_canary(object, false));
    TEST_ASSERT(!gc_internal_validate_canaries(object));
    TEST_ASSERT_EQ_INT(GC_STATUS_CORRUPTED_MEMORY, gc_get_status());
    TEST_ASSERT(gc_internal_corrupt_canary(object, false));
    TEST_ASSERT(gc_internal_validate_canaries(object));

    TEST_ASSERT(gc_internal_corrupt_canary(object, true));
    TEST_ASSERT(!gc_internal_validate_canaries(object));
    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_CORRUPTED_MEMORY, gc_get_status());
    return EXIT_SUCCESS;
}

static int test_explicit_roots(void)
{
    void *root = NULL;
    void *second_root = NULL;
    void *current = (void *)(uintptr_t)1;
    void *first_object;
    void *second_object;
    HANDLE thread;

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_add_root(NULL));
    TEST_ASSERT_EQ_INT(GC_STATUS_INVALID_ARGUMENT, gc_get_status());

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_add_root(&root));
    TEST_ASSERT(gc_internal_root_count() == (size_t)1);
    TEST_ASSERT(gc_internal_get_root_value(&root, &current));
    TEST_ASSERT(current == NULL);
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_add_root(&root));
    TEST_ASSERT_EQ_INT(GC_STATUS_DUPLICATE_ROOT, gc_get_status());

    first_object = gc_malloc(8);
    second_object = gc_malloc(8);
    TEST_ASSERT(first_object != NULL && second_object != NULL);
    root = first_object;
    TEST_ASSERT(gc_internal_get_root_value(&root, &current));
    TEST_ASSERT(current == first_object);
    root = second_object;
    TEST_ASSERT(gc_internal_get_root_value(&root, &current));
    TEST_ASSERT(current == second_object);

    thread = CreateThread(NULL, 0, add_root_from_other_thread,
                          &second_root, 0, NULL);
    TEST_ASSERT(thread != NULL);
    TEST_ASSERT(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0);
    TEST_ASSERT(CloseHandle(thread));
    TEST_ASSERT_EQ_INT(GC_STATUS_WRONG_THREAD, gc_get_status());
    TEST_ASSERT(gc_internal_root_count() == (size_t)1);

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_remove_root(&root));
    TEST_ASSERT(gc_internal_root_count() == (size_t)0);
    TEST_ASSERT(!gc_internal_get_root_value(&root, &current));
    TEST_ASSERT(current == NULL);
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_remove_root(&root));
    TEST_ASSERT_EQ_INT(GC_STATUS_ROOT_NOT_FOUND, gc_get_status());
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_remove_root(NULL));
    TEST_ASSERT_EQ_INT(GC_STATUS_INVALID_ARGUMENT, gc_get_status());

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_add_root(&second_root));
    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    TEST_ASSERT(gc_internal_root_count() == (size_t)0);
    return EXIT_SUCCESS;
}

static void store_pointer(void *object, size_t offset, const void *value)
{
    uintptr_t candidate = (uintptr_t)value;

    memcpy((unsigned char *)object + offset,
           &candidate, sizeof candidate);
}

static GC_TEST_NOINLINE void scrub_stack_roots(void)
{
    volatile uintptr_t noise[256];
    size_t index;

    for (index = 0; index < sizeof noise / sizeof noise[0]; ++index) {
        noise[index] = (uintptr_t)0;
    }
}

static GC_TEST_NOINLINE int allocate_unrooted_object(void)
{
    void *object = gc_malloc(32);

    TEST_ASSERT(object != NULL);
    object = NULL;
    return EXIT_SUCCESS;
}

static int test_mark_sweep_collection(void)
{
    void *root;
    void *first;
    void *second;
    void *garbage;
    GCStats stats;
    HANDLE thread;

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    first = gc_malloc(32);
    second = gc_malloc(32);
    garbage = gc_malloc(32);
    TEST_ASSERT(first != NULL && second != NULL && garbage != NULL);
    store_pointer(first, 1, (unsigned char *)second + 5);
    store_pointer(second, 3, (unsigned char *)first + 7);
    root = (unsigned char *)first + 4;
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_add_root(&root));

    thread = CreateThread(NULL, 0, collect_from_other_thread,
                          NULL, 0, NULL);
    TEST_ASSERT(thread != NULL);
    TEST_ASSERT(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0);
    TEST_ASSERT(CloseHandle(thread));
    TEST_ASSERT_EQ_INT(GC_STATUS_WRONG_THREAD, gc_get_status());
    TEST_ASSERT(gc_internal_allocation_count() == (size_t)3);

    gc_collect();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    TEST_ASSERT(gc_internal_allocation_count() == (size_t)3);
    TEST_ASSERT(gc_internal_get_allocation_info(first, &(size_t){0},
                                                &(size_t){0}));
    TEST_ASSERT(gc_internal_get_allocation_info(second, &(size_t){0},
                                                &(size_t){0}));
    TEST_ASSERT(gc_internal_get_allocation_info(garbage, &(size_t){0},
                                                &(size_t){0}));
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_get_stats(&stats));
    TEST_ASSERT(stats.collection_count == (size_t)1);
    TEST_ASSERT(stats.last_objects_examined == (size_t)3);
    TEST_ASSERT(stats.last_objects_collected == (size_t)0);
    TEST_ASSERT(stats.bytes_live == (size_t)96);
    TEST_ASSERT(stats.bytes_collected == (size_t)0);
    TEST_ASSERT(stats.performance_frequency > (uint64_t)0);

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_remove_root(&root));
    gc_collect();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    TEST_ASSERT(gc_internal_allocation_count() == (size_t)3);
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_get_stats(&stats));
    TEST_ASSERT(stats.collection_count == (size_t)2);
    TEST_ASSERT(stats.last_objects_examined == (size_t)3);
    TEST_ASSERT(stats.last_objects_collected == (size_t)0);
    TEST_ASSERT(stats.bytes_live == (size_t)96);
    TEST_ASSERT(stats.bytes_collected == (size_t)0);
    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    return EXIT_SUCCESS;
}

static int test_unrooted_object_collection(void)
{
    GCStats stats;
    size_t remaining;

    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, allocate_unrooted_object());
    scrub_stack_roots();
    gc_collect();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    remaining = gc_internal_allocation_count();
    TEST_ASSERT(remaining <= (size_t)1);
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_get_stats(&stats));
    TEST_ASSERT(stats.collection_count == (size_t)1);
    TEST_ASSERT(stats.last_objects_examined <= (size_t)1);
    TEST_ASSERT(stats.last_objects_collected == (size_t)1 - remaining);
    TEST_ASSERT(stats.bytes_live == remaining * (size_t)32);
    TEST_ASSERT(stats.bytes_collected == ((size_t)1 - remaining) * (size_t)32);
    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    return EXIT_SUCCESS;
}

int main(void)
{
    GCStats stats = {0};

    TEST_ASSERT(gc_malloc(8) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    gc_collect();
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_set_memory_limit(4096));
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_add_root(NULL));
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_remove_root(NULL));
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_get_stats(NULL));
    TEST_ASSERT_EQ_INT(GC_STATUS_INVALID_ARGUMENT, gc_get_status());
    TEST_ASSERT_EQ_INT(GC_FAILURE, gc_get_stats(&stats));
    TEST_ASSERT(stats.bytes_requested == (size_t)0);
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_invalid_state_transitions());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_stack_limits_and_thread_ownership());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_virtual_alloc_objects());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_memory_pressure());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_debug_canaries());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_explicit_roots());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_mark_sweep_collection());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_unrooted_object_collection());
    puts("test_gc: ok");
    return EXIT_SUCCESS;
}
