#ifndef MARKER_H
#define MARKER_H

#include "allocator.h"

#include <stddef.h>

typedef struct {
    GCAllocation **items;
    size_t head;
    size_t length;
    size_t capacity;
} GCMarkQueue;

typedef enum {
    GC_MARK_QUEUE_ADDED = 0,
    GC_MARK_QUEUE_ALREADY_MARKED,
    GC_MARK_QUEUE_INVALID,
    GC_MARK_QUEUE_OUT_OF_MEMORY
} GCMarkQueueResult;

void gc_mark_queue_init(GCMarkQueue *queue);
GCMarkQueueResult gc_mark_queue_push(GCMarkQueue *queue,
                                     GCAllocation *allocation);
GCAllocation *gc_mark_queue_pop(GCMarkQueue *queue);
size_t gc_mark_queue_pending(const GCMarkQueue *queue);
void gc_mark_queue_destroy(GCMarkQueue *queue);

#endif
