#ifndef GC_H
#define GC_H

#include <stdbool.h>
#include <stddef.h>

#include "gc_stats.h"

#define GC_SUCCESS 0
#define GC_FAILURE (-1)

typedef enum {
    GC_STATUS_OK = 0,
    GC_STATUS_NOT_INITIALIZED,
    GC_STATUS_ALREADY_INITIALIZED,
    GC_STATUS_WRONG_THREAD,
    GC_STATUS_INVALID_ARGUMENT,
    GC_STATUS_INVALID_STACK_LIMITS,
    GC_STATUS_SIZE_OVERFLOW,
    GC_STATUS_OUT_OF_MEMORY,
    GC_STATUS_INTERNAL_ERROR,
    GC_STATUS_CORRUPTED_MEMORY
} GCStatus;

int gc_init(void);
void *gc_malloc(size_t size);
int gc_set_memory_limit(size_t bytes);
int gc_get_stats(GCStats *out);
void gc_shutdown(void);

bool gc_is_initialized(void);
GCStatus gc_get_status(void);

#endif
