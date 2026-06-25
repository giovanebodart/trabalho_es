#ifndef SWEEPER_H
#define SWEEPER_H

#include "allocator.h"
#include "gc_stats.h"

#include <stddef.h>

typedef enum {
    GC_SWEEP_OK = 0,
    GC_SWEEP_INVALID,
    GC_SWEEP_TREE_ERROR,
    GC_SWEEP_RELEASE_ERROR,
    GC_SWEEP_STATS_ERROR
} GCSweepResult;

GCSweepResult gc_sweep(GCAllocation **allocations,
                       IntervalNode **tree,
                       size_t *allocation_count,
                       GCStats *stats);
GCSweepResult gc_sweep_young(GCAllocation **allocations,
                             IntervalNode **tree,
                             size_t *allocation_count,
                             GCStats *stats);

#endif
