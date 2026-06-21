#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "interval_tree.h"
#include <stdbool.h>
#include <stddef.h>
typedef struct GCAllocation {
    IntervalNode interval;
    void *memory;
    size_t requested_size;
    size_t reserved_size;
    struct GCAllocation *next;
} GCAllocation;
bool gc_allocator_round_size(size_t requested, size_t page_size,
                             size_t *reserved);
GCAllocation *gc_allocator_create(size_t requested, size_t reserved);
void gc_allocator_destroy_all(GCAllocation *allocation);
#endif
