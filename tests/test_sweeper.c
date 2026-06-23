#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "sweeper.h"
#include "test.h"

static int test_sweep_releases_unmarked_objects(void)
{
    const size_t sizes[] = {8, 16, 24, 32, 40};
    GCAllocation *objects[5];
    void *memories[5];
    GCAllocation *allocations = NULL;
    IntervalNode *tree = NULL;
    GCStats stats = {0, 0, 0, 0};
    SYSTEM_INFO system_info;
    size_t allocation_count = 0;
    size_t index;

    GetSystemInfo(&system_info);
    for (index = 0; index < (size_t)5; ++index) {
        size_t reserved;

        TEST_ASSERT(gc_allocator_reservation_size(
            sizes[index], (size_t)system_info.dwPageSize, &reserved));
        objects[index] = gc_allocator_create(sizes[index], reserved);
        TEST_ASSERT(objects[index] != NULL);
        TEST_ASSERT(interval_tree_insert(&tree, &objects[index]->interval));
        objects[index]->next = allocations;
        allocations = objects[index];
        memories[index] = objects[index]->memory;
        ++allocation_count;
        stats.bytes_requested += sizes[index];
        stats.bytes_live += sizes[index];
        stats.bytes_reserved += reserved;
    }

    objects[1]->marked = true;
    objects[3]->marked = true;
    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree,
                                &allocation_count, &stats));
    TEST_ASSERT(allocation_count == (size_t)2);
    TEST_ASSERT(stats.bytes_requested == (size_t)120);
    TEST_ASSERT(stats.bytes_live == sizes[1] + sizes[3]);
    TEST_ASSERT(stats.bytes_collected
                == sizes[0] + sizes[2] + sizes[4]);
    TEST_ASSERT(stats.bytes_reserved
                == (size_t)system_info.dwPageSize * (size_t)2);
    TEST_ASSERT(!objects[1]->marked && !objects[3]->marked);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[1])
                == &objects[1]->interval);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[3])
                == &objects[3]->interval);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[0]) == NULL);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[2]) == NULL);
    TEST_ASSERT(interval_tree_find(tree, (uintptr_t)memories[4]) == NULL);
#ifndef NDEBUG
    TEST_ASSERT(interval_tree_validate(tree));
#endif

    for (index = 0; index < (size_t)5; index += (size_t)2) {
        MEMORY_BASIC_INFORMATION info;

        TEST_ASSERT(VirtualQuery(memories[index], &info, sizeof info)
                    == sizeof info);
        TEST_ASSERT(info.State == MEM_FREE);
    }

    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree,
                                &allocation_count, &stats));
    TEST_ASSERT(allocations == NULL);
    TEST_ASSERT(tree == NULL);
    TEST_ASSERT(allocation_count == (size_t)0);
    TEST_ASSERT(stats.bytes_live == (size_t)0);
    TEST_ASSERT(stats.bytes_reserved == (size_t)0);
    TEST_ASSERT(stats.bytes_collected == stats.bytes_requested);
    for (index = 1; index < (size_t)5; index += (size_t)2) {
        MEMORY_BASIC_INFORMATION info;

        TEST_ASSERT(VirtualQuery(memories[index], &info, sizeof info)
                    == sizeof info);
        TEST_ASSERT(info.State == MEM_FREE);
    }
    return EXIT_SUCCESS;
}

static int test_sweep_rejects_invalid_state(void)
{
    GCAllocation fake = {0};
    GCAllocation *allocations = NULL;
    IntervalNode *tree = NULL;
    GCStats stats = {0, 0, 0, 0};
    size_t count = 0;

    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(NULL, &tree, &count, &stats));
    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(&allocations, NULL, &count, &stats));
    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(&allocations, &tree, NULL, &stats));
    TEST_ASSERT_EQ_INT(GC_SWEEP_INVALID,
                       gc_sweep(&allocations, &tree, &count, NULL));
    count = 1;
    TEST_ASSERT_EQ_INT(GC_SWEEP_STATS_ERROR,
                       gc_sweep(&allocations, &tree, &count, &stats));
    fake.requested_size = 1;
    fake.reserved_size = 1;
    allocations = &fake;
    stats = (GCStats){1, 1, 1, 0};
    TEST_ASSERT_EQ_INT(GC_SWEEP_TREE_ERROR,
                       gc_sweep(&allocations, &tree, &count, &stats));
    allocations = NULL;
    count = 0;
    stats = (GCStats){0, 0, 0, 0};
    TEST_ASSERT_EQ_INT(GC_SWEEP_OK,
                       gc_sweep(&allocations, &tree, &count, &stats));
    return EXIT_SUCCESS;
}

int main(void)
{
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_sweep_rejects_invalid_state());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_sweep_releases_unmarked_objects());
    puts("test_sweeper: ok");
    return EXIT_SUCCESS;
}
