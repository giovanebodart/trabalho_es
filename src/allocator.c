#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "allocator.h"

#include <stdint.h>
#include <stdlib.h>

bool gc_allocator_round_size(size_t requested,
                             size_t page_size,
                             size_t *reserved)
{
    size_t remainder;

    if (requested == 0 || page_size == 0 || reserved == NULL) {
        return false;
    }

    remainder = requested % page_size;
    if (remainder == 0) {
        *reserved = requested;
        return true;
    }
    if (requested > SIZE_MAX - (page_size - remainder)) {
        return false;
    }

    *reserved = requested + page_size - remainder;
    return true;
}

GCAllocation *gc_allocator_create(size_t requested, size_t reserved)
{
    GCAllocation *allocation = malloc(sizeof *allocation);
    uintptr_t start;

    if (allocation == NULL) {
        return NULL;
    }

    allocation->memory = VirtualAlloc(NULL, reserved,
                                      MEM_RESERVE | MEM_COMMIT,
                                      PAGE_READWRITE);
    if (allocation->memory == NULL) {
        free(allocation);
        return NULL;
    }

    start = (uintptr_t)allocation->memory;
    if (requested > UINTPTR_MAX - start
        || !interval_node_init(&allocation->interval,
                               start, start + requested)) {
        (void)VirtualFree(allocation->memory, 0, MEM_RELEASE);
        free(allocation);
        return NULL;
    }

    allocation->requested_size = requested;
    allocation->reserved_size = reserved;
    allocation->next = NULL;
    return allocation;
}

void gc_allocator_destroy_all(GCAllocation *allocation)
{
    while (allocation != NULL) {
        GCAllocation *next = allocation->next;

        (void)VirtualFree(allocation->memory, 0, MEM_RELEASE);
        free(allocation);
        allocation = next;
    }
}
