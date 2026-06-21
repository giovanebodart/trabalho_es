#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "allocator.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef union {
    long double long_double_value;
    long long long_long_value;
    void *pointer_value;
} GCNaturalAlignment;

#ifndef NDEBUG
static const uint64_t GC_CANARY_BEFORE = UINT64_C(0xD15EA5ECAFEBABE);
static const uint64_t GC_CANARY_AFTER = UINT64_C(0xC0FFEEBADF00D123);
#define GC_CANARY_SIZE (sizeof(uint64_t))
#define GC_PREFIX_SIZE (sizeof(GCNaturalAlignment))
_Static_assert(GC_PREFIX_SIZE >= GC_CANARY_SIZE,
               "the aligned prefix must contain the leading canary");
#else
#define GC_CANARY_SIZE ((size_t)0)
#define GC_PREFIX_SIZE ((size_t)0)
#endif

static bool add_sizes(size_t left, size_t right, size_t *result)
{
    if (result == NULL || left > SIZE_MAX - right) {
        return false;
    }
    *result = left + right;
    return true;
}

bool gc_allocator_reservation_size(size_t requested,
                                   size_t page_size,
                                   size_t *reserved)
{
    size_t layout_size;
    size_t remainder;

    if (requested == 0 || page_size == 0 || reserved == NULL) {
        return false;
    }

    if (!add_sizes(GC_PREFIX_SIZE, requested, &layout_size)
        || !add_sizes(layout_size, GC_CANARY_SIZE, &layout_size)) {
        return false;
    }

    remainder = layout_size % page_size;
    if (remainder == 0) {
        *reserved = layout_size;
        return true;
    }
    if (layout_size > SIZE_MAX - (page_size - remainder)) {
        return false;
    }

    *reserved = layout_size + page_size - remainder;
    return true;
}

GCAllocation *gc_allocator_create(size_t requested, size_t reserved)
{
    GCAllocation *allocation = malloc(sizeof *allocation);
    uintptr_t start;
    unsigned char *mapping;

    if (allocation == NULL) {
        return NULL;
    }

    allocation->mapping = VirtualAlloc(NULL, reserved,
                                       MEM_RESERVE | MEM_COMMIT,
                                       PAGE_READWRITE);
    if (allocation->mapping == NULL) {
        free(allocation);
        return NULL;
    }

    mapping = allocation->mapping;
    allocation->memory = mapping + GC_PREFIX_SIZE;
    start = (uintptr_t)allocation->memory;
    if (requested > UINTPTR_MAX - start
        || !interval_node_init(&allocation->interval,
                               start, start + requested)) {
        (void)VirtualFree(allocation->mapping, 0, MEM_RELEASE);
        free(allocation);
        return NULL;
    }

    allocation->requested_size = requested;
    allocation->reserved_size = reserved;
    allocation->next = NULL;
#ifndef NDEBUG
    memcpy((unsigned char *)allocation->memory - GC_CANARY_SIZE,
           &GC_CANARY_BEFORE, GC_CANARY_SIZE);
    memcpy((unsigned char *)allocation->memory + requested,
           &GC_CANARY_AFTER, GC_CANARY_SIZE);
#endif
    return allocation;
}

bool gc_allocator_validate_canaries(const GCAllocation *allocation)
{
#ifndef NDEBUG
    uint64_t before;
    uint64_t after;

    if (allocation == NULL || allocation->mapping == NULL
        || allocation->memory == NULL) {
        return false;
    }
    memcpy(&before,
           (const unsigned char *)allocation->memory - GC_CANARY_SIZE,
           GC_CANARY_SIZE);
    memcpy(&after,
           (const unsigned char *)allocation->memory
               + allocation->requested_size,
           GC_CANARY_SIZE);
    return before == GC_CANARY_BEFORE && after == GC_CANARY_AFTER;
#else
    return allocation != NULL;
#endif
}

bool gc_allocator_validate_all(const GCAllocation *allocation)
{
    while (allocation != NULL) {
        if (!gc_allocator_validate_canaries(allocation)) {
            return false;
        }
        allocation = allocation->next;
    }
    return true;
}

bool gc_allocator_corrupt_canary(GCAllocation *allocation,
                                 bool after_object)
{
#ifndef NDEBUG
    unsigned char *canary;

    if (allocation == NULL) {
        return false;
    }
    canary = after_object
             ? (unsigned char *)allocation->memory
                   + allocation->requested_size
             : (unsigned char *)allocation->memory - GC_CANARY_SIZE;
    canary[0] ^= UINT8_C(0xff);
    return true;
#else
    (void)allocation;
    (void)after_object;
    return false;
#endif
}

void gc_allocator_destroy_all(GCAllocation *allocation)
{
    while (allocation != NULL) {
        GCAllocation *next = allocation->next;

        (void)VirtualFree(allocation->mapping, 0, MEM_RELEASE);
        free(allocation);
        allocation = next;
    }
}
