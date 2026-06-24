#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "allocator.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GC_ARENA_SIZE ((size_t)64 * 1024u)
#define GC_SMALL_CLASS_COUNT ((size_t)6)

typedef union {
    long double long_double_value;
    long long long_long_value;
    void *pointer_value;
} GCNaturalAlignment;

typedef struct GCFreeBlock {
    struct GCFreeBlock *next;
} GCFreeBlock;

typedef struct {
    size_t block_size;
    GCFreeBlock *free_list;
} GCSizeClass;

typedef struct GCArena {
    unsigned char *memory;
    size_t capacity;
    size_t used;
    size_t block_size;
    struct GCArena *next;
} GCArena;

static GCSizeClass gc_size_classes[GC_SMALL_CLASS_COUNT] = {
    {32u, NULL},
    {64u, NULL},
    {128u, NULL},
    {256u, NULL},
    {512u, NULL},
    {1024u, NULL}
};
static GCArena *gc_arenas;

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

static bool align_size(size_t value, size_t alignment, size_t *result)
{
    size_t remainder;

    if (alignment == 0 || result == NULL) {
        return false;
    }
    remainder = value % alignment;
    if (remainder == 0) {
        *result = value;
        return true;
    }
    if (value > SIZE_MAX - (alignment - remainder)) {
        return false;
    }
    *result = value + alignment - remainder;
    return true;
}

static bool gc_small_class_size(size_t layout_size, size_t *block_size)
{
    size_t index;

    if (block_size == NULL) {
        return false;
    }
    for (index = 0; index < GC_SMALL_CLASS_COUNT; ++index) {
        if (layout_size <= gc_size_classes[index].block_size) {
            *block_size = gc_size_classes[index].block_size;
            return true;
        }
    }
    return false;
}

static GCSizeClass *gc_size_class_for_block(size_t block_size)
{
    size_t index;

    for (index = 0; index < GC_SMALL_CLASS_COUNT; ++index) {
        if (gc_size_classes[index].block_size == block_size) {
            return &gc_size_classes[index];
        }
    }
    return NULL;
}

static bool gc_is_small_block(size_t block_size)
{
    return block_size <= gc_size_classes[GC_SMALL_CLASS_COUNT - (size_t)1]
                         .block_size;
}

static GCArena *gc_arena_create(size_t block_size)
{
    GCArena *arena;

    if (block_size == 0 || block_size > GC_ARENA_SIZE) {
        return NULL;
    }

    arena = malloc(sizeof *arena);
    if (arena == NULL) {
        return NULL;
    }
    arena->memory = VirtualAlloc(NULL, GC_ARENA_SIZE,
                                 MEM_RESERVE | MEM_COMMIT,
                                 PAGE_READWRITE);
    if (arena->memory == NULL) {
        free(arena);
        return NULL;
    }

    arena->capacity = GC_ARENA_SIZE;
    arena->used = 0;
    arena->block_size = block_size;
    arena->next = gc_arenas;
    gc_arenas = arena;
    return arena;
}

static bool gc_size_class_refill(GCSizeClass *size_class)
{
    GCArena *arena;
    size_t count;
    size_t index;

    if (size_class == NULL) {
        return false;
    }

    arena = gc_arena_create(size_class->block_size);
    if (arena == NULL) {
        return false;
    }
    count = arena->capacity / size_class->block_size;
    for (index = 0; index < count; ++index) {
        GCFreeBlock *block = (GCFreeBlock *)
            (arena->memory + index * size_class->block_size);

        block->next = size_class->free_list;
        size_class->free_list = block;
    }
    arena->used = count * size_class->block_size;
    return count > 0;
}

static void *gc_size_class_allocate(size_t block_size)
{
    GCSizeClass *size_class = gc_size_class_for_block(block_size);
    GCFreeBlock *block;

    if (size_class == NULL) {
        return NULL;
    }
    if (size_class->free_list == NULL
        && !gc_size_class_refill(size_class)) {
        return NULL;
    }

    block = size_class->free_list;
    size_class->free_list = block->next;
    return block;
}

static void gc_size_classes_reset(void)
{
    size_t index;

    for (index = 0; index < GC_SMALL_CLASS_COUNT; ++index) {
        gc_size_classes[index].free_list = NULL;
    }
}

static void *gc_dedicated_allocate(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT,
                        PAGE_READWRITE);
}

bool gc_allocator_reservation_size(size_t requested,
                                   size_t page_size,
                                   size_t *reserved)
{
    size_t layout_size;

    if (requested == 0 || page_size == 0 || reserved == NULL) {
        return false;
    }

    if (!add_sizes(GC_PREFIX_SIZE, requested, &layout_size)
        || !add_sizes(layout_size, GC_CANARY_SIZE, &layout_size)
        || !align_size(layout_size, _Alignof(GCNaturalAlignment),
                       &layout_size)) {
        return false;
    }

    if (gc_small_class_size(layout_size, reserved)) {
        return true;
    }
    return align_size(layout_size, page_size, reserved);
}

GCAllocation *gc_allocator_create(size_t requested, size_t reserved)
{
    GCAllocation *allocation = malloc(sizeof *allocation);
    uintptr_t start;
    unsigned char *block;
    bool dedicated = !gc_is_small_block(reserved);

    if (allocation == NULL) {
        return NULL;
    }

    block = dedicated
            ? gc_dedicated_allocate(reserved)
            : gc_size_class_allocate(reserved);
    if (block == NULL) {
        free(allocation);
        return NULL;
    }

    allocation->mapping = block;
    allocation->memory = block + GC_PREFIX_SIZE;
    start = (uintptr_t)allocation->memory;
    if (requested > UINTPTR_MAX - start
        || !interval_node_init(&allocation->interval,
                               start, start + requested)) {
        if (dedicated) {
            (void)VirtualFree(block, 0, MEM_RELEASE);
        }
        free(allocation);
        return NULL;
    }

    allocation->requested_size = requested;
    allocation->reserved_size = reserved;
    allocation->dedicated_mapping = dedicated;
    allocation->marked = false;
    allocation->next = NULL;
    memset(allocation->memory, 0, requested);
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

bool gc_allocator_destroy_one(GCAllocation *allocation)
{
    if (allocation == NULL) {
        return false;
    }
    if (allocation->dedicated_mapping && allocation->mapping != NULL
        && !VirtualFree(allocation->mapping, 0, MEM_RELEASE)) {
        return false;
    }
    free(allocation);
    return true;
}

void gc_allocator_destroy_all(GCAllocation *allocation)
{
    while (allocation != NULL) {
        GCAllocation *next = allocation->next;

        if (allocation->dedicated_mapping && allocation->mapping != NULL) {
            (void)VirtualFree(allocation->mapping, 0, MEM_RELEASE);
        }
        free(allocation);
        allocation = next;
    }
    while (gc_arenas != NULL) {
        GCArena *next = gc_arenas->next;

        (void)VirtualFree(gc_arenas->memory, 0, MEM_RELEASE);
        free(gc_arenas);
        gc_arenas = next;
    }
    gc_size_classes_reset();
}
