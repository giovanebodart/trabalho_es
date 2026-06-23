#include "sweeper.h"

#include <stdint.h>

static bool gc_sweep_state_valid(const GCAllocation *allocations,
                                 size_t allocation_count,
                                 const GCStats *stats)
{
    size_t count = 0;
    size_t live = 0;
    size_t reserved = 0;

    while (allocations != NULL) {
        if (count == SIZE_MAX
            || live > SIZE_MAX - allocations->requested_size
            || reserved > SIZE_MAX - allocations->reserved_size) {
            return false;
        }
        ++count;
        live += allocations->requested_size;
        reserved += allocations->reserved_size;
        allocations = allocations->next;
    }
    return count == allocation_count
           && live == stats->bytes_live
           && reserved == stats->bytes_reserved
           && stats->bytes_collected <= SIZE_MAX - live
           && stats->bytes_requested
              == stats->bytes_collected + live;
}

static bool gc_sweep_tree_valid(const GCAllocation *allocations,
                                IntervalNode *tree)
{
    while (allocations != NULL) {
        if (interval_tree_find(tree, allocations->interval.start)
            != &allocations->interval) {
            return false;
        }
        allocations = allocations->next;
    }
    return true;
}

GCSweepResult gc_sweep(GCAllocation **allocations,
                       IntervalNode **tree,
                       size_t *allocation_count,
                       GCStats *stats)
{
    GCAllocation **link;

    if (allocations == NULL || tree == NULL
        || allocation_count == NULL || stats == NULL) {
        return GC_SWEEP_INVALID;
    }
    if (!gc_sweep_state_valid(*allocations, *allocation_count, stats)) {
        return GC_SWEEP_STATS_ERROR;
    }
    if (!gc_sweep_tree_valid(*allocations, *tree)) {
        return GC_SWEEP_TREE_ERROR;
    }

    link = allocations;
    while (*link != NULL) {
        GCAllocation *allocation = *link;

        if (allocation->marked) {
            allocation->marked = false;
            link = &allocation->next;
            continue;
        }

        {
            IntervalNode *removed = NULL;
            GCAllocation *next = allocation->next;
            size_t requested = allocation->requested_size;
            size_t reserved = allocation->reserved_size;

            if (!interval_tree_remove(tree, allocation->interval.start,
                                      &removed)
                || removed != &allocation->interval) {
                return GC_SWEEP_TREE_ERROR;
            }
            if (!gc_allocator_destroy_one(allocation)) {
                if (!interval_tree_insert(tree, &allocation->interval)) {
                    return GC_SWEEP_TREE_ERROR;
                }
                return GC_SWEEP_RELEASE_ERROR;
            }

            *link = next;
            --*allocation_count;
            stats->bytes_live -= requested;
            stats->bytes_reserved -= reserved;
            stats->bytes_collected += requested;
        }
    }
    return GC_SWEEP_OK;
}
