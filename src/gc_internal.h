#ifndef GC_INTERNAL_H
#define GC_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

bool gc_internal_get_stack_limits(uintptr_t *low, uintptr_t *high);

#endif
