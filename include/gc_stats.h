#ifndef GC_STATS_H
#define GC_STATS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t bytes_requested;
    size_t bytes_reserved;
    size_t bytes_internal_fragmentation;
    size_t bytes_live;
    size_t bytes_collected;
    size_t collection_count;
    size_t minor_collection_count;
    size_t major_collection_count;
    size_t tree_searches;
    size_t tree_comparisons;
    size_t promoted_objects;
    size_t last_objects_examined;
    size_t last_objects_collected;
    size_t last_tree_searches;
    size_t last_tree_comparisons;
    size_t last_promoted_objects;
    size_t last_dirty_pages;
    size_t max_resident_bytes;
    uint64_t pause_ticks;
    uint64_t mark_ticks;
    uint64_t sweep_ticks;
    uint64_t last_pause_ticks;
    uint64_t last_minor_pause_ticks;
    uint64_t last_major_pause_ticks;
    uint64_t last_mark_ticks;
    uint64_t last_sweep_ticks;
    uint64_t performance_frequency;
} GCStats;

#endif
