#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

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
    page->dirty = false;
    page->protected = false;
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

bool gc_old_pages_protect(GCOldPage *pages)
{
    while (pages != NULL) {
#if GC_OLD_PAGES_PROTECTION_SUPPORTED
        DWORD old_protect;

        if (!pages->protected
            && !VirtualProtect(pages->base, pages->size,
                               PAGE_READONLY, &old_protect)) {
            return false;
        }
        pages->dirty = false;
        pages->protected = true;
#else
        pages->dirty = false;
        pages->protected = false;
#endif
        pages = pages->next;
    }
    return true;
}

bool gc_old_pages_unprotect_for_write(GCOldPage *pages,
                                      const void *address)
{
    GCOldPage *page = (GCOldPage *)gc_old_pages_find(pages, address);
#if GC_OLD_PAGES_PROTECTION_SUPPORTED
    DWORD old_protect;
#endif

    if (page == NULL || !page->protected) {
        return false;
    }
#if GC_OLD_PAGES_PROTECTION_SUPPORTED
    if (!VirtualProtect(page->base, page->size,    
                        PAGE_READWRITE, &old_protect)) {
        return false;
    }
    page->dirty = true;
    page->protected = false;
    return true;
#else
    return false;
#endif
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

size_t gc_old_pages_dirty_count(const GCOldPage *pages)
{
    size_t count = 0;

    while (pages != NULL) {
        if (pages->dirty && count < SIZE_MAX) {
            ++count;
        }
        pages = pages->next;
    }
    return count;
}
