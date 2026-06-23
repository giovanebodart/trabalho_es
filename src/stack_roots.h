#ifndef STACK_ROOTS_H
#define STACK_ROOTS_H

#include "marker.h"

#include <stdint.h>

typedef enum {
    GC_STACK_DIRECTION_UNKNOWN = 0,
    GC_STACK_GROWS_DOWN,
    GC_STACK_GROWS_UP
} GCStackDirection;

typedef enum {
    GC_STACK_SCAN_OK = 0,
    GC_STACK_SCAN_INVALID,
    GC_STACK_SCAN_OUT_OF_MEMORY
} GCStackScanResult;

/*
 * The current frame address approximates the stack pointer. Candidates kept
 * only in registers are intentionally outside this module's responsibility.
 */
GCStackDirection gc_stack_direction(void);
GCStackScanResult gc_stack_scan_region(uintptr_t begin,
                                       uintptr_t end,
                                       IntervalNode *tree,
                                       GCMarkQueue *queue);
GCStackScanResult gc_stack_scan(uintptr_t low,
                                uintptr_t high,
                                IntervalNode *tree,
                                GCMarkQueue *queue);

#endif
