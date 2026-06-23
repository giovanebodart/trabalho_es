#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "stack_roots.h"
#include "test.h"

#include <stdint.h>
#include <string.h>

static GCAllocation allocation;
static unsigned char managed_object[32];

static int reset_fixture(void)
{
    memset(&allocation, 0, sizeof allocation);
    TEST_ASSERT(interval_node_init(
        &allocation.interval,
        (uintptr_t)managed_object,
        (uintptr_t)(managed_object + sizeof managed_object)));
    allocation.memory = managed_object;
    allocation.requested_size = sizeof managed_object;
    return EXIT_SUCCESS;
}

static int test_direction_and_invalid_ranges(void)
{
    GCMarkQueue queue;

    gc_mark_queue_init(&queue);
    TEST_ASSERT(gc_stack_direction() == GC_STACK_GROWS_DOWN);
    TEST_ASSERT(gc_stack_scan_region((uintptr_t)2, (uintptr_t)1,
                                     &allocation.interval, &queue)
                == GC_STACK_SCAN_INVALID);
    TEST_ASSERT(gc_stack_scan_region((uintptr_t)1, (uintptr_t)2,
                                     NULL, &queue)
                == GC_STACK_SCAN_INVALID);
    TEST_ASSERT(gc_stack_scan_region((uintptr_t)1, (uintptr_t)2,
                                     &allocation.interval, NULL)
                == GC_STACK_SCAN_INVALID);
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}

static int test_unaligned_region_scan(void)
{
    unsigned char bytes[sizeof(uintptr_t) + 3] = {0};
    uintptr_t candidate = (uintptr_t)(managed_object + 7);
    GCMarkQueue queue;

    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, reset_fixture());
    memcpy(bytes + 3, &candidate, sizeof candidate);
    gc_mark_queue_init(&queue);
    TEST_ASSERT(gc_stack_scan_region(
        (uintptr_t)bytes, (uintptr_t)(bytes + sizeof bytes),
        &allocation.interval, &queue) == GC_STACK_SCAN_OK);
    TEST_ASSERT(allocation.marked);
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)1);
    TEST_ASSERT(gc_mark_queue_pop(&queue) == &allocation);
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}

static int test_current_stack_scan(void)
{
    ULONG_PTR low;
    ULONG_PTR high;
    volatile uintptr_t stack_root;
    GCMarkQueue queue;

    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, reset_fixture());
    stack_root = (uintptr_t)(managed_object + 11);
    GetCurrentThreadStackLimits(&low, &high);
    gc_mark_queue_init(&queue);
    TEST_ASSERT(gc_stack_scan((uintptr_t)low, (uintptr_t)high,
                              &allocation.interval, &queue)
                == GC_STACK_SCAN_OK);
    TEST_ASSERT(stack_root == (uintptr_t)(managed_object + 11));
    TEST_ASSERT(allocation.marked);
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)1);
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}

int main(void)
{
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, reset_fixture());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_direction_and_invalid_ranges());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_unaligned_region_scan());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_current_stack_scan());
    puts("test_stack_roots: ok");
    return EXIT_SUCCESS;
}
