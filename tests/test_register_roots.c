#include "register_roots.h"
#include "test.h"

#include <stdint.h>
#include <string.h>

#if defined(__GNUC__)
#define GC_TEST_NOINLINE __attribute__((noinline))
#else
#define GC_TEST_NOINLINE
#endif

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

static int test_invalid_arguments(void)
{
    GCMarkQueue queue;
    unsigned char bytes[sizeof(uintptr_t)] = {0};

    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, reset_fixture());
    gc_mark_queue_init(&queue);
    TEST_ASSERT(gc_register_roots_scan_region(
        NULL, sizeof bytes, &allocation.interval, &queue)
        == GC_REGISTER_SCAN_INVALID);
    TEST_ASSERT(gc_register_roots_scan_region(
        bytes, sizeof bytes, NULL, &queue)
        == GC_REGISTER_SCAN_INVALID);
    TEST_ASSERT(gc_register_roots_scan_region(
        bytes, sizeof bytes, &allocation.interval, NULL)
        == GC_REGISTER_SCAN_INVALID);
    TEST_ASSERT(gc_register_roots_scan(NULL, &queue)
                == GC_REGISTER_SCAN_INVALID);
    TEST_ASSERT(gc_register_roots_scan(&allocation.interval, NULL)
                == GC_REGISTER_SCAN_INVALID);
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}

static int test_unaligned_context_region(void)
{
    unsigned char bytes[sizeof(uintptr_t) + 5] = {0};
    uintptr_t candidate = (uintptr_t)(managed_object + 9);
    GCMarkQueue queue;

    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, reset_fixture());
    memcpy(bytes + 5, &candidate, sizeof candidate);
    gc_mark_queue_init(&queue);
    TEST_ASSERT(gc_register_roots_scan_region(
        bytes, sizeof bytes, &allocation.interval, &queue)
        == GC_REGISTER_SCAN_OK);
    TEST_ASSERT(allocation.marked);
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)1);
    TEST_ASSERT(gc_mark_queue_pop(&queue) == &allocation);
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}

#if defined(__GNUC__) && defined(__x86_64__)
static GC_TEST_NOINLINE GCRegisterScanResult scan_with_preserved_register(
    uintptr_t candidate,
    IntervalNode *tree,
    GCMarkQueue *queue)
{
    register uintptr_t preserved __asm__("r12") = candidate;
    GCRegisterScanResult result;

    __asm__ volatile("" : "+r"(preserved));
    result = gc_register_roots_scan(tree, queue);
    __asm__ volatile("" : "+r"(preserved));
    if (preserved != candidate) {
        return GC_REGISTER_SCAN_INVALID;
    }
    return result;
}

static int test_setjmp_preserved_register(void)
{
    uintptr_t candidate = (uintptr_t)(managed_object + 13);
    GCMarkQueue queue;

    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, reset_fixture());
    gc_mark_queue_init(&queue);
    TEST_ASSERT(scan_with_preserved_register(
        candidate, &allocation.interval, &queue) == GC_REGISTER_SCAN_OK);
    TEST_ASSERT(allocation.marked);
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)1);
    TEST_ASSERT(gc_mark_queue_pop(&queue) == &allocation);
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}
#else
static int test_setjmp_preserved_register(void)
{
    return EXIT_SUCCESS;
}
#endif

int main(void)
{
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_invalid_arguments());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_unaligned_context_region());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_setjmp_preserved_register());
    puts("test_register_roots: ok");
    return EXIT_SUCCESS;
}
