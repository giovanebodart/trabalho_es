#ifndef GC_INTERNAL_H
#define GC_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

bool gc_internal_get_stack_limits(uintptr_t *low, uintptr_t *high);
size_t gc_internal_allocation_count(void);
size_t gc_internal_memory_limit(void);
size_t gc_internal_promotion_threshold(void);
size_t gc_internal_old_page_count(void);
bool gc_internal_get_old_page_info(const void *address,
                                   void **base,
                                   size_t *size);
size_t gc_internal_collection_request_count(void);
size_t gc_internal_root_count(void);
bool gc_internal_get_root_value(void **root, void **value);
bool gc_internal_get_allocation_info(const void *address,
                                     size_t *requested,
                                     size_t *reserved);
bool gc_internal_validate_canaries(const void *address);
bool gc_internal_corrupt_canary(const void *address, bool after_object);

#endif
