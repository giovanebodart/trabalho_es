#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gc_config.h"
#include "sweeper.h"
#include "test.h"

#include <string.h>

static int test_sweep_releases_unmarked_objects(void)
{
    size_t sizes[5];
    GCAllocation *objects[5];
    void *memories[5];
    GCAllocation *allocations = NULL;
    IntervalNode *tree = NULL;
    GCStats stats = {0};
    SYSTEM_INFO system_info;
    GCAllocation *replacement;
    void *reused_mapping;
    size_t allocation_count = 0;
    size_t total_requested = 0;
    size_t reused_reserved;
    size_t index;

    GetSystemInfo(&system_info);
    sizes[0] = 24;
    sizes[1] = 16;
    sizes[2] = (size_t)system_info.dwPageSize + (size_t)1;
    sizes[3] = 32;
    sizes[4] = 40;
    for (index = 0; index < (size_t)5; ++index) {
        size_t reserved;

        TEST_ASSERT(gc_allocator_reservation_size(
            sizes[index], (size_t)system_info.dwPageSize, &reserved));
        objects[index] = gc_allocator_create(sizes[index], reserved);
        TEST_ASSERT(objects[index] != NULL);
        TEST_ASSERT(objects[index]->generation == GC_GENERATION_YOUNG);
        TEST_ASSERT(objects[index]->survival_count == (size_t)0);
        TEST_ASSERT(interval_tree_insert(&tree, &objects[index]->interval));
        objects[index]->next = allocations;
        allocations = objects[index];
        memories[index] = objects[index]->memory;
        ++allocation_count;
        total_requested += sizes[index];
        stats.bytes_requested += sizes[index];
        stats.bytes_live += sizes[index];
        stats.bytes_reserved += reserved;
        stats.bytes_internal_fragmentation += reserved - sizes[index];
    }
    memset(memories[0], 0x5a, sizes[0]);
    reused_mapping = objects[0]->mapping;
    reused_reserved = objects[0]->reserved_size;

    objects[1]->marked = true;
    objects[3]->marked = true;
    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree,
                                &allocation_count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocation_count == (size_t)2);
    TEST_ASSERT(stats.bytes_requested == total_requested);
    TEST_ASSERT(stats.bytes_live == sizes[1] + sizes[3]);
    TEST_ASSERT(stats.bytes_collected
                == sizes[0] + sizes[2] + sizes[4]);
    TEST_ASSERT(stats.bytes_reserved
                == objects[1]->reserved_size + objects[3]->reserved_size);
    TEST_ASSERT(stats.bytes_internal_fragmentation
                == objects[1]->reserved_size - sizes[1]
                   + objects[3]->reserved_size - sizes[3]);
    TEST_ASSERT(!objects[1]->marked && !objects[3]->marked);
    TEST_ASSERT(objects[1]->generation == GC_GENERATION_YOUNG);
    TEST_ASSERT(objects[3]->generation == GC_GENERATION_YOUNG);
    TEST_ASSERT(objects[1]->survival_count == (size_t)1);
    TEST_ASSERT(objects[3]->survival_count == (size_t)1);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[1])
                == &objects[1]->interval);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[3])
                == &objects[3]->interval);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[0]) == NULL);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[2]) == NULL);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[4]) == NULL);
    {
        MEMORY_BASIC_INFORMATION info;

        TEST_ASSERT(VirtualQuery(memories[0], &info, sizeof info)
                    == sizeof info);
        TEST_ASSERT(info.State == MEM_COMMIT);
    }
    {
        MEMORY_BASIC_INFORMATION info;

        TEST_ASSERT(VirtualQuery(memories[2], &info, sizeof info)
                    == sizeof info);
        TEST_ASSERT(info.State == MEM_FREE);
    }
    replacement = gc_allocator_create(sizes[0], reused_reserved);
    TEST_ASSERT(replacement != NULL);
    TEST_ASSERT(replacement->generation == GC_GENERATION_YOUNG);
    TEST_ASSERT(replacement->survival_count == (size_t)0);
    TEST_ASSERT(replacement->mapping == reused_mapping);
    TEST_ASSERT(replacement->memory == memories[0]);
    TEST_ASSERT(((unsigned char *)replacement->memory)[0] == 0);
    TEST_ASSERT(gc_allocator_destroy_one(replacement));
#ifndef NDEBUG
    TEST_ASSERT(interval_tree_validate(tree));
#endif

    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree,
                                &allocation_count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocations == NULL);
    TEST_ASSERT(tree == NULL);
    TEST_ASSERT(allocation_count == (size_t)0);
    TEST_ASSERT(stats.bytes_live == (size_t)0);
    TEST_ASSERT(stats.bytes_reserved == (size_t)0);
    TEST_ASSERT(stats.bytes_internal_fragmentation == (size_t)0);
    TEST_ASSERT(stats.bytes_collected == stats.bytes_requested);
    for (index = 0; index < (size_t)5; ++index) {
        MEMORY_BASIC_INFORMATION info;

        TEST_ASSERT(VirtualQuery(memories[index], &info, sizeof info)
                    == sizeof info);
        TEST_ASSERT(info.State == MEM_FREE);
    }
    gc_allocator_destroy_all(NULL);
    return EXIT_SUCCESS;
}

static int test_minor_sweep_collects_only_young_objects(void)
{
    size_t sizes[3] = {24, 32, 48};
    GCAllocation *objects[3];
    void *young_dead_memory;
    GCAllocation *allocations = NULL;
    IntervalNode *tree = NULL;
    GCStats stats = {0};
    SYSTEM_INFO system_info;
    size_t allocation_count = 0;
    size_t index;

    GetSystemInfo(&system_info);
    for (index = 0; index < (size_t)3; ++index) {
        size_t reserved;

        TEST_ASSERT(gc_allocator_reservation_size(
            sizes[index], (size_t)system_info.dwPageSize, &reserved));
        objects[index] = gc_allocator_create(sizes[index], reserved);
        TEST_ASSERT(objects[index] != NULL);
        TEST_ASSERT(interval_tree_insert(&tree, &objects[index]->interval));
        objects[index]->next = allocations;
        allocations = objects[index];
        ++allocation_count;
        stats.bytes_requested += sizes[index];
        stats.bytes_live += sizes[index];
        stats.bytes_reserved += reserved;
        stats.bytes_internal_fragmentation += reserved - sizes[index];
    }

    young_dead_memory = objects[1]->memory;
    objects[0]->marked = true;
    objects[2]->generation = GC_GENERATION_OLD;
    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep_young(&allocations, &tree,
                                      &allocation_count, &stats,
                                      GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocation_count == (size_t)2);
    TEST_ASSERT(!objects[0]->marked);
    TEST_ASSERT(!objects[2]->marked);
    TEST_ASSERT(objects[0]->survival_count == (size_t)1);
    TEST_ASSERT(objects[2]->survival_count == (size_t)0);
    TEST_ASSERT(stats.bytes_live == sizes[0] + sizes[2]);
    TEST_ASSERT(stats.bytes_collected == sizes[1]);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)objects[0]->memory)
                == &objects[0]->interval);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)objects[2]->memory)
                == &objects[2]->interval);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)young_dead_memory)
                == NULL);

    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree,
                                &allocation_count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocations == NULL);
    TEST_ASSERT(tree == NULL);
    TEST_ASSERT(allocation_count == (size_t)0);
    TEST_ASSERT(stats.bytes_live == (size_t)0);
    TEST_ASSERT(stats.bytes_collected == stats.bytes_requested);
    gc_allocator_destroy_all(NULL);
    return EXIT_SUCCESS;
}

static int test_young_survivor_is_promoted_by_threshold(void)
{
    const size_t size = 40;
    GCAllocation *allocation;
    GCAllocation *allocations = NULL;
    IntervalNode *tree = NULL;
    GCStats stats = {0};
    SYSTEM_INFO system_info;
    size_t allocation_count = 1;
    size_t reserved;

    GetSystemInfo(&system_info);
    TEST_ASSERT(gc_allocator_reservation_size(
        size, (size_t)system_info.dwPageSize, &reserved));
    allocation = gc_allocator_create(size, reserved);
    TEST_ASSERT(allocation != NULL);
    TEST_ASSERT(interval_tree_insert(&tree, &allocation->interval));
    allocations = allocation;
    stats.bytes_requested = size;
    stats.bytes_live = size;
    stats.bytes_reserved = reserved;
    stats.bytes_internal_fragmentation = reserved - size;

    allocation->marked = true;
    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep_young(&allocations, &tree,
                                      &allocation_count, &stats,
                                      GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocation->generation == GC_GENERATION_YOUNG);
    TEST_ASSERT(allocation->survival_count == (size_t)1);

    allocation->marked = true;
    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep_young(&allocations, &tree,
                                      &allocation_count, &stats,
                                      GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocation->generation == GC_GENERATION_OLD);
    TEST_ASSERT(allocation->survival_count == (size_t)2);

    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep_young(&allocations, &tree,
                                      &allocation_count, &stats,
                                      GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocation_count == (size_t)1);
    TEST_ASSERT(stats.bytes_live == size);

    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree,
                                &allocation_count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT(allocations == NULL);
    TEST_ASSERT(tree == NULL);
    TEST_ASSERT(allocation_count == (size_t)0);
    TEST_ASSERT(stats.bytes_live == (size_t)0);
    TEST_ASSERT(stats.bytes_collected == size);
    gc_allocator_destroy_all(NULL);
    return EXIT_SUCCESS;
}

static int test_sweep_rejects_invalid_state(void)
{
    GCAllocation fake = {0};
    GCAllocation *allocations = NULL;
    IntervalNode *tree = NULL;
    GCStats stats = {0};
    size_t count = 0;

    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(NULL, &tree, &count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(&allocations, NULL, &count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(&allocations, &tree, NULL, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(&allocations, &tree, &count, NULL,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(&allocations, &tree, &count, &stats,
                                (size_t)0));
    count = 1;
    TEST_ASSERT_EQ_INT(GC_SWEEP_STATS_ERROR,
                       gc_sweep(&allocations, &tree, &count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    fake.requested_size = 1;
    fake.reserved_size = 1;
    fake.dedicated_mapping = false;
    allocations = &fake;
    stats = (GCStats){
        .bytes_requested = 1,
        .bytes_reserved = 1,
        .bytes_live = 1
    };
    TEST_ASSERT_EQ_INT(GC_SWEEP_TREE_ERROR,
                       gc_sweep(&allocations, &tree, &count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    allocations = NULL;
    count = 0;
    stats = (GCStats){0};
    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree, &count, &stats,
                                GC_DEFAULT_PROMOTION_THRESHOLD));
    return EXIT_SUCCESS;
}

int main(void)
{
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_sweep_rejects_invalid_state());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_sweep_releases_unmarked_objects());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_minor_sweep_collects_only_young_objects());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_young_survivor_is_promoted_by_threshold());
    puts("test_sweeper: ok");
    return EXIT_SUCCESS;
}
