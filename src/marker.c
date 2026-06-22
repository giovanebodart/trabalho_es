#include "marker.h"

#include <stdint.h>
#include <stdlib.h>

#define GC_MARK_QUEUE_INITIAL_CAPACITY ((size_t)16)

static bool gc_mark_queue_grow(GCMarkQueue *queue)
{
    GCAllocation **items;
    size_t capacity;

    if (queue->capacity == 0) {
        capacity = GC_MARK_QUEUE_INITIAL_CAPACITY;
    } else {
        if (queue->capacity > SIZE_MAX / (size_t)2) {
            return false;
        }
        capacity = queue->capacity * (size_t)2;
    }
    if (capacity > SIZE_MAX / sizeof *items) {
        return false;
    }

    items = realloc(queue->items, capacity * sizeof *items);
    if (items == NULL) {
        return false;
    }
    queue->items = items;
    queue->capacity = capacity;
    return true;
}

void gc_mark_queue_init(GCMarkQueue *queue)
{
    if (queue == NULL) {
        return;
    }
    queue->items = NULL;
    queue->head = 0;
    queue->length = 0;
    queue->capacity = 0;
}

GCMarkQueueResult gc_mark_queue_push(GCMarkQueue *queue,
                                     GCAllocation *allocation)
{
    if (queue == NULL || allocation == NULL) {
        return GC_MARK_QUEUE_INVALID;
    }
    if (allocation->marked) {
        return GC_MARK_QUEUE_ALREADY_MARKED;
    }
    if (queue->length == queue->capacity
        && !gc_mark_queue_grow(queue)) {
        return GC_MARK_QUEUE_OUT_OF_MEMORY;
    }

    queue->items[queue->length] = allocation;
    ++queue->length;
    allocation->marked = true;
    return GC_MARK_QUEUE_ADDED;
}

GCAllocation *gc_mark_queue_pop(GCMarkQueue *queue)
{
    if (queue == NULL || queue->head >= queue->length) {
        return NULL;
    }
    return queue->items[queue->head++];
}

size_t gc_mark_queue_pending(const GCMarkQueue *queue)
{
    if (queue == NULL || queue->head > queue->length) {
        return 0;
    }
    return queue->length - queue->head;
}

void gc_mark_queue_destroy(GCMarkQueue *queue)
{
    if (queue == NULL) {
        return;
    }
    free(queue->items);
    gc_mark_queue_init(queue);
}
