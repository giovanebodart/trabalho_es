#ifndef GC_H
#define GC_H

#include <stdbool.h>

#define GC_SUCCESS 0
#define GC_FAILURE (-1)

typedef enum {
    GC_STATUS_OK = 0,
    GC_STATUS_NOT_INITIALIZED,
    GC_STATUS_ALREADY_INITIALIZED,
    GC_STATUS_WRONG_THREAD,
    GC_STATUS_INVALID_ARGUMENT,
    GC_STATUS_INVALID_STACK_LIMITS
} GCStatus;

int gc_init(void);
void gc_shutdown(void);

bool gc_is_initialized(void);
GCStatus gc_get_status(void);

#endif
