#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "interval_tree.h"
#include <stdbool.h>
#include <stddef.h>
typedef enum {
    GC_GENERATION_YOUNG = 0,
    GC_GENERATION_OLD = 1
} GCGeneration;
typedef struct GCAllocation {
    IntervalNode interval;
    void *mapping;
    void *memory;
    size_t requested_size;
    size_t reserved_size;
    size_t survival_count;
    GCGeneration generation;
    bool dedicated_mapping;
    bool marked;
    struct GCAllocation *next;
} GCAllocation;
bool gc_allocator_reservation_size(size_t requested, size_t page_size,
                                   size_t *reserved);
GCAllocation *gc_allocator_create(size_t requested, size_t reserved);
bool gc_allocator_validate_canaries(const GCAllocation *allocation);
bool gc_allocator_validate_all(const GCAllocation *allocation);
bool gc_allocator_corrupt_canary(GCAllocation *allocation,
                                 bool after_object);
void gc_allocator_record_survival(GCAllocation *allocation,
                                  size_t promotion_threshold);
bool gc_allocator_destroy_one(GCAllocation *allocation);
void gc_allocator_destroy_all(GCAllocation *allocation);
#endif
