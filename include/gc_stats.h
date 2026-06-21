#ifndef GC_STATS_H
#define GC_STATS_H

#include <stddef.h>

typedef struct {
    size_t bytes_requested;
    size_t bytes_reserved;
    size_t bytes_live;
    size_t bytes_collected;
} GCStats;

#endif
