#ifndef ROOTS_H
#define ROOTS_H

#include <stdbool.h>

typedef struct GCRoot {
    void **location;
    struct GCRoot *next;
} GCRoot;

typedef enum {
    GC_ROOT_RESULT_OK = 0,
    GC_ROOT_RESULT_DUPLICATE,
    GC_ROOT_RESULT_NOT_FOUND,
    GC_ROOT_RESULT_OUT_OF_MEMORY
} GCRootResult;

GCRootResult gc_roots_add(GCRoot **roots, void **location);
GCRootResult gc_roots_remove(GCRoot **roots, void **location);
bool gc_roots_get_value(const GCRoot *roots, void **location,
                        void **value);
void gc_roots_destroy_all(GCRoot *roots);

#endif
