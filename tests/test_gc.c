#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gc.h"
#include "gc_internal.h"
#include "test.h"

#include <stdint.h>

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

int main(void)
{
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_invalid_state_transitions());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_stack_limits_and_thread_ownership());
    puts("test_gc: ok");
    return EXIT_SUCCESS;
}
