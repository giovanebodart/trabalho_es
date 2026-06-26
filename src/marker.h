#ifndef MARKER_H
#define MARKER_H

#include "allocator.h"
#include <stddef.h>

typedef struct {
    GCAllocation **items;
    size_t head;
    size_t length;
    size_t capacity;
    size_t tree_searches;
    size_t tree_comparisons;
} GCMarkQueue;

typedef enum {
    GC_MARK_QUEUE_ADDED = 0,
    GC_MARK_QUEUE_ALREADY_MARKED,
    GC_MARK_QUEUE_INVALID,
    GC_MARK_QUEUE_OUT_OF_MEMORY
} GCMarkQueueResult;

typedef enum {
    GC_MARK_SCAN_OK = 0,
    GC_MARK_SCAN_INVALID,
    GC_MARK_SCAN_OUT_OF_MEMORY
} GCMarkScanResult;

void gc_mark_queue_init(GCMarkQueue *queue);
IntervalNode *gc_mark_find_candidate(IntervalNode *tree,
                                     uintptr_t candidate,
                                     GCMarkQueue *queue);
GCMarkQueueResult gc_mark_queue_push(GCMarkQueue *queue, GCAllocation *allocation);
GCAllocation *gc_mark_queue_pop(GCMarkQueue *queue);
size_t gc_mark_queue_pending(const GCMarkQueue *queue);
void gc_mark_queue_destroy(GCMarkQueue *queue);
GCMarkScanResult gc_mark_scan_object(const GCAllocation *source, IntervalNode *tree, GCMarkQueue *queue);

#endif
