#ifndef OLD_PAGES_H
#define OLD_PAGES_H

#include "allocator.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    GC_OLD_PAGE_OK = 0,
    GC_OLD_PAGE_INVALID,
    GC_OLD_PAGE_OUT_OF_MEMORY,
    GC_OLD_PAGE_SIZE_OVERFLOW
} GCOldPageResult;

typedef struct GCOldPage {
    void *base;
    size_t size;
    struct GCOldPage *next;
} GCOldPage;

GCOldPageResult gc_old_pages_rebuild(GCOldPage **pages,
                                      const GCAllocation *allocations);
const GCOldPage *gc_old_pages_find(const GCOldPage *pages,
                                   const void *address);
size_t gc_old_pages_count(const GCOldPage *pages);
void gc_old_pages_destroy(GCOldPage *pages);

#endif
