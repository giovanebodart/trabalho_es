#ifndef OLD_PAGES_H
#define OLD_PAGES_H

#include "allocator.h"

#include <stdbool.h>
#include <stddef.h>

#if defined(__has_feature)
#define GC_OLD_PAGES_HAS_FEATURE(feature) __has_feature(feature)
#else
#define GC_OLD_PAGES_HAS_FEATURE(feature) 0
#endif


#if defined(__SANITIZE_ADDRESS__) \
|| GC_OLD_PAGES_HAS_FEATURE(address_sanitizer)
#define GC_OLD_PAGES_PROTECTION_SUPPORTED 0
#else
#define GC_OLD_PAGES_PROTECTION_SUPPORTED 1
#endif

/*
Define se o GC terá proteção de memoria ou não
É importante não ter isso quando se roda testes ASan, pois o ASan já trabalha com proteção de memoria
Caso houvesse dois mecanismo de proteção rodando no mesmo build, isso poderia gerar comportanmentos estranhos. 
Dificultando a eficacia do teste 
*/
typedef enum {
    GC_OLD_PAGE_OK = 0,
    GC_OLD_PAGE_INVALID,
    GC_OLD_PAGE_OUT_OF_MEMORY,
    GC_OLD_PAGE_SIZE_OVERFLOW
} GCOldPageResult;

typedef struct GCOldPage {
    void *base;
    size_t size;
    bool dirty;
    bool protected;
    struct GCOldPage *next;
} GCOldPage;

GCOldPageResult gc_old_pages_rebuild(GCOldPage **pages,
                                      const GCAllocation *allocations);
const GCOldPage *gc_old_pages_find(const GCOldPage *pages,
                                   const void *address);
bool gc_old_pages_protect(GCOldPage *pages);
bool gc_old_pages_unprotect_for_write(GCOldPage *pages,
                                      const void *address);
size_t gc_old_pages_count(const GCOldPage *pages);
size_t gc_old_pages_dirty_count(const GCOldPage *pages);
void gc_old_pages_destroy(GCOldPage *pages);

#endif
