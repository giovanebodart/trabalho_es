#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gc.h"
#include "gc_internal.h"
#include "test.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

static DWORD WINAPI shutdown_from_other_thread(LPVOID context)
{
    (void)context;
    gc_shutdown();
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
    unsigned char *small;
    unsigned char *large;
    size_t requested;
    size_t reserved;
    size_t impossible_size;
    MEMORY_BASIC_INFORMATION memory_info;

    GetSystemInfo(&system_info);
    TEST_ASSERT_EQ_INT(GC_SUCCESS, gc_init());
    TEST_ASSERT(gc_malloc(0) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_INVALID_ARGUMENT, gc_get_status());
    TEST_ASSERT(gc_malloc(SIZE_MAX) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_SIZE_OVERFLOW, gc_get_status());
    impossible_size = SIZE_MAX - ((size_t)system_info.dwPageSize - (size_t)1);
    TEST_ASSERT(gc_malloc(impossible_size) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_OUT_OF_MEMORY, gc_get_status());

    small = gc_malloc(17);
    large = gc_malloc((size_t)system_info.dwPageSize + (size_t)1);
    TEST_ASSERT(small != NULL);
    TEST_ASSERT(large != NULL);
    TEST_ASSERT(gc_internal_allocation_count() == (size_t)2);
    TEST_ASSERT(gc_internal_get_allocation_info(small + 16,
                                                &requested, &reserved));
    TEST_ASSERT(requested == (size_t)17);
    TEST_ASSERT(reserved == (size_t)system_info.dwPageSize);
    TEST_ASSERT(!gc_internal_get_allocation_info(small + 17,
                                                 &requested, &reserved));
    TEST_ASSERT(gc_internal_get_allocation_info(large,
                                                &requested, &reserved));
    TEST_ASSERT(requested == (size_t)system_info.dwPageSize + (size_t)1);
    TEST_ASSERT(reserved == (size_t)system_info.dwPageSize * (size_t)2);

    TEST_ASSERT(small[0] == 0 && small[16] == 0);
    TEST_ASSERT(large[0] == 0 && large[requested - (size_t)1] == 0);
    memset(small, 0x5a, 17);
    memset(large, 0xa5, requested);
    gc_shutdown();
    TEST_ASSERT_EQ_INT(GC_STATUS_OK, gc_get_status());
    TEST_ASSERT(gc_internal_allocation_count() == (size_t)0);
    TEST_ASSERT(VirtualQuery(small, &memory_info, sizeof memory_info)
                == sizeof memory_info);
    TEST_ASSERT(memory_info.State == MEM_FREE);
    return EXIT_SUCCESS;
}

int main(void)
{
    TEST_ASSERT(gc_malloc(8) == NULL);
    TEST_ASSERT_EQ_INT(GC_STATUS_NOT_INITIALIZED, gc_get_status());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_invalid_state_transitions());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_stack_limits_and_thread_ownership());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_virtual_alloc_objects());
    puts("test_gc: ok");
    return EXIT_SUCCESS;
}
