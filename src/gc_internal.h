#ifndef GC_INTERNAL_H
#define GC_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

bool gc_internal_get_stack_limits(uintptr_t *low, uintptr_t *high);
size_t gc_internal_allocation_count(void);
bool gc_internal_get_allocation_info(const void *address,
                                     size_t *requested,
                                     size_t *reserved);

#endif
