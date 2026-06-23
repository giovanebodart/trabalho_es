#include "stack_roots.h"

#include <stddef.h>
#include <string.h>

#if defined(__GNUC__)
#define GC_NOINLINE __attribute__((noinline))
#define GC_NO_ASAN __attribute__((no_sanitize_address))
#else
#define GC_NOINLINE
#define GC_NO_ASAN
#endif

static GC_NOINLINE GCStackDirection gc_stack_probe(uintptr_t outer)
{
    volatile unsigned char inner = 0;
    uintptr_t inner_address = (uintptr_t)&inner;

    if (inner_address < outer) {
        return GC_STACK_GROWS_DOWN;
    }
    if (inner_address > outer) {
        return GC_STACK_GROWS_UP;
    }
    return GC_STACK_DIRECTION_UNKNOWN;
}

GCStackDirection gc_stack_direction(void)
{
    volatile unsigned char outer = 0;

    return gc_stack_probe((uintptr_t)&outer);
}

/*
 * Conservative scanning intentionally crosses padding between C objects.
 * AddressSanitizer poisons that padding, although it remains inside the
 * readable stack mapping. Bounds and arithmetic stay instrumented elsewhere.
 */
GC_NO_ASAN GCStackScanResult gc_stack_scan_region(
    uintptr_t begin,
    uintptr_t end,
    IntervalNode *tree,
    GCMarkQueue *queue)
{
    uintptr_t address;

    if (begin > end || tree == NULL || queue == NULL) {
        return GC_STACK_SCAN_INVALID;
    }
    if (end - begin < sizeof(uintptr_t)) {
        return GC_STACK_SCAN_OK;
    }

    for (address = begin;
         address <= end - sizeof(uintptr_t);
         ++address) {
        GCMarkQueueResult result;
        IntervalNode *interval;
        uintptr_t candidate;

        memcpy(&candidate, (const void *)address, sizeof candidate);
        interval = interval_tree_find(tree, candidate);
        if (interval == NULL) {
            continue;
        }
        result = gc_mark_queue_push(queue, (GCAllocation *)interval);
        if (result == GC_MARK_QUEUE_OUT_OF_MEMORY) {
            return GC_STACK_SCAN_OUT_OF_MEMORY;
        }
        if (result == GC_MARK_QUEUE_INVALID) {
            return GC_STACK_SCAN_INVALID;
        }
    }
    return GC_STACK_SCAN_OK;
}

GCStackScanResult gc_stack_scan(uintptr_t low,
                                uintptr_t high,
                                IntervalNode *tree,
                                GCMarkQueue *queue)
{
    volatile unsigned char marker = 0;
    uintptr_t current = (uintptr_t)&marker;
    GCStackDirection direction = gc_stack_direction();

    if (low >= high || current < low || current >= high) {
        return GC_STACK_SCAN_INVALID;
    }
    if (direction == GC_STACK_GROWS_DOWN) {
        return gc_stack_scan_region(current, high, tree, queue);
    }
    if (direction == GC_STACK_GROWS_UP) {
        return gc_stack_scan_region(low, current + (uintptr_t)1,
                                    tree, queue);
    }
    return GC_STACK_SCAN_INVALID;
}
