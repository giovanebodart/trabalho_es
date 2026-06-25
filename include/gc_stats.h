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
    size_t last_objects_examined;
    size_t last_objects_collected;
    uint64_t last_pause_ticks;
    uint64_t last_minor_pause_ticks;
    uint64_t last_major_pause_ticks;
    uint64_t performance_frequency;
} GCStats;

#endif
