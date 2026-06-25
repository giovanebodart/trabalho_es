#include "old_pages.h"

#include <stdint.h>
#include <stdlib.h>

static bool gc_old_page_contains(const GCOldPage *page,
                                 const void *address)
{
    uintptr_t start;
    uintptr_t end;
    uintptr_t candidate;

    if (page == NULL || page->base == NULL || address == NULL
        || page->size == 0) {
        return false;
    }

    start = (uintptr_t)page->base;
    if (start > UINTPTR_MAX - page->size) {
        return false;
    }
    end = start + page->size;
    candidate = (uintptr_t)address;
    return candidate >= start && candidate < end;
}

static GCOldPageResult gc_old_pages_push(GCOldPage **pages,
                                         const GCAllocation *allocation)
{
    GCOldPage *page;

    if (pages == NULL || allocation == NULL || allocation->mapping == NULL
        || allocation->reserved_size == 0) {
        return GC_OLD_PAGE_INVALID;
    }
    if ((uintptr_t)allocation->mapping
        > UINTPTR_MAX - allocation->reserved_size) {
        return GC_OLD_PAGE_SIZE_OVERFLOW;
    }

    page = malloc(sizeof *page);
    if (page == NULL) {
        return GC_OLD_PAGE_OUT_OF_MEMORY;
    }
    page->base = allocation->mapping;
    page->size = allocation->reserved_size;
    page->next = *pages;
    *pages = page;
    return GC_OLD_PAGE_OK;
}

void gc_old_pages_destroy(GCOldPage *pages)
{
    while (pages != NULL) {
        GCOldPage *next = pages->next;

        free(pages);
        pages = next;
    }
}

GCOldPageResult gc_old_pages_rebuild(GCOldPage **pages,
                                      const GCAllocation *allocations)
{
    GCOldPage *rebuilt = NULL;

    if (pages == NULL) {
        return GC_OLD_PAGE_INVALID;
    }

    while (allocations != NULL) {
        if (allocations->generation == GC_GENERATION_OLD
            && allocations->dedicated_mapping) {
            GCOldPageResult result = gc_old_pages_push(&rebuilt,
                                                       allocations);

            if (result != GC_OLD_PAGE_OK) {
                gc_old_pages_destroy(rebuilt);
                return result;
            }
        }
        allocations = allocations->next;
    }

    gc_old_pages_destroy(*pages);
    *pages = rebuilt;
    return GC_OLD_PAGE_OK;
}

const GCOldPage *gc_old_pages_find(const GCOldPage *pages,
                                   const void *address)
{
    while (pages != NULL) {
        if (gc_old_page_contains(pages, address)) {
            return pages;
        }
        pages = pages->next;
    }
    return NULL;
}

size_t gc_old_pages_count(const GCOldPage *pages)
{
    size_t count = 0;

    while (pages != NULL) {
        ++count;
        pages = pages->next;
    }
    return count;
}
