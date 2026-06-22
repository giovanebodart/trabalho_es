#include "marker.h"
#include "test.h"

#include <stdint.h>
#include <string.h>

#define TEST_OBJECT_COUNT 40
#define GRAPH_OBJECT_COUNT 3
#define GRAPH_OBJECT_SIZE 40

static int test_queue_order_and_duplicates(void)
{
    GCAllocation objects[TEST_OBJECT_COUNT];
    GCMarkQueue queue;
    size_t index;

    memset(objects, 0, sizeof objects);
    gc_mark_queue_init(&queue);
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)0);
    TEST_ASSERT(gc_mark_queue_pop(&queue) == NULL);
    TEST_ASSERT_EQ_INT(GC_MARK_QUEUE_INVALID,
                       gc_mark_queue_push(NULL, &objects[0]));
    TEST_ASSERT_EQ_INT(GC_MARK_QUEUE_INVALID,
                       gc_mark_queue_push(&queue, NULL));
    objects[0].marked = true;
    TEST_ASSERT_EQ_INT(GC_MARK_QUEUE_ALREADY_MARKED,
                       gc_mark_queue_push(&queue, &objects[0]));
    objects[0].marked = false;

    for (index = 0; index < TEST_OBJECT_COUNT; ++index) {
        TEST_ASSERT_EQ_INT(GC_MARK_QUEUE_ADDED,
                           gc_mark_queue_push(&queue, &objects[index]));
        TEST_ASSERT(objects[index].marked);
    }
    TEST_ASSERT(gc_mark_queue_pending(&queue)
                == (size_t)TEST_OBJECT_COUNT);
    TEST_ASSERT_EQ_INT(GC_MARK_QUEUE_ALREADY_MARKED,
                       gc_mark_queue_push(&queue, &objects[17]));
    TEST_ASSERT(gc_mark_queue_pending(&queue)
                == (size_t)TEST_OBJECT_COUNT);

    for (index = 0; index < TEST_OBJECT_COUNT; ++index) {
        TEST_ASSERT(gc_mark_queue_pop(&queue) == &objects[index]);
        TEST_ASSERT(gc_mark_queue_pending(&queue)
                    == (size_t)TEST_OBJECT_COUNT - index - (size_t)1);
    }
    TEST_ASSERT(gc_mark_queue_pop(&queue) == NULL);
    TEST_ASSERT(objects[0].marked && objects[TEST_OBJECT_COUNT - 1].marked);

    gc_mark_queue_destroy(&queue);
    TEST_ASSERT(queue.items == NULL);
    TEST_ASSERT(queue.head == (size_t)0);
    TEST_ASSERT(queue.length == (size_t)0);
    TEST_ASSERT(queue.capacity == (size_t)0);
    return EXIT_SUCCESS;
}

static bool initialize_graph(GCAllocation objects[GRAPH_OBJECT_COUNT],
                             unsigned char storage[GRAPH_OBJECT_COUNT]
                                                  [GRAPH_OBJECT_SIZE],
                             IntervalNode **tree)
{
    size_t index;

    memset(objects, 0, sizeof *objects * GRAPH_OBJECT_COUNT);
    memset(storage, 0, sizeof *storage * GRAPH_OBJECT_COUNT);
    *tree = NULL;
    for (index = 0; index < GRAPH_OBJECT_COUNT; ++index) {
        uintptr_t start = (uintptr_t)storage[index];

        objects[index].memory = storage[index];
        objects[index].requested_size = GRAPH_OBJECT_SIZE;
        if (!interval_node_init(&objects[index].interval, start,
                                start + GRAPH_OBJECT_SIZE)
            || !interval_tree_insert(tree, &objects[index].interval)) {
            return false;
        }
    }
    return true;
}

static void write_candidate(unsigned char *storage, size_t offset,
                            uintptr_t candidate)
{
    memcpy(storage + offset, &candidate, sizeof candidate);
}

static int test_conservative_object_scan(void)
{
    GCAllocation objects[GRAPH_OBJECT_COUNT];
    unsigned char storage[GRAPH_OBJECT_COUNT][GRAPH_OBJECT_SIZE];
    GCMarkQueue queue;
    IntervalNode *tree;
    GCAllocation *current;
    size_t processed = 0;

    TEST_ASSERT(initialize_graph(objects, storage, &tree));
#ifndef NDEBUG
    TEST_ASSERT(interval_tree_validate(tree));
#endif
    write_candidate(storage[0], 1, (uintptr_t)(storage[1] + 5));
    write_candidate(storage[0], 16, UINTPTR_MAX);
    write_candidate(storage[0], GRAPH_OBJECT_SIZE - sizeof(uintptr_t),
                    (uintptr_t)(storage[2] + 7));
    write_candidate(storage[1], 3, (uintptr_t)(storage[2] + 1));
    write_candidate(storage[2], 5, (uintptr_t)storage[1]);

    gc_mark_queue_init(&queue);
    TEST_ASSERT_EQ_INT(GC_MARK_QUEUE_ADDED,
                       gc_mark_queue_push(&queue, &objects[0]));
    while ((current = gc_mark_queue_pop(&queue)) != NULL) {
        TEST_ASSERT_EQ_INT(GC_MARK_SCAN_OK,
                           gc_mark_scan_object(current, tree, &queue));
        ++processed;
    }
    TEST_ASSERT(processed == (size_t)GRAPH_OBJECT_COUNT);
    TEST_ASSERT(objects[0].marked);
    TEST_ASSERT(objects[1].marked);
    TEST_ASSERT(objects[2].marked);
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)0);
#ifndef NDEBUG
    TEST_ASSERT(interval_tree_validate(tree));
#endif
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}

static int test_scan_boundaries_and_invalid_candidates(void)
{
    GCAllocation objects[GRAPH_OBJECT_COUNT];
    unsigned char storage[GRAPH_OBJECT_COUNT][GRAPH_OBJECT_SIZE];
    GCMarkQueue queue;
    IntervalNode *tree;

    TEST_ASSERT(initialize_graph(objects, storage, &tree));
    objects[0].requested_size = sizeof(uintptr_t);
    write_candidate(storage[0], 0,
                    (uintptr_t)(storage[2] + GRAPH_OBJECT_SIZE));
    gc_mark_queue_init(&queue);
    TEST_ASSERT_EQ_INT(GC_MARK_SCAN_OK,
                       gc_mark_scan_object(&objects[0], tree, &queue));
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)0);
    write_candidate(storage[0], 0, UINTPTR_MAX);
    TEST_ASSERT_EQ_INT(GC_MARK_SCAN_OK,
                       gc_mark_scan_object(&objects[0], tree, &queue));
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)0);
    write_candidate(storage[0], 0, (uintptr_t)storage[1]);
    objects[0].requested_size = sizeof(uintptr_t) - (size_t)1;
    TEST_ASSERT_EQ_INT(GC_MARK_SCAN_OK,
                       gc_mark_scan_object(&objects[0], tree, &queue));
    TEST_ASSERT(gc_mark_queue_pending(&queue) == (size_t)0);
    TEST_ASSERT_EQ_INT(GC_MARK_SCAN_INVALID,
                       gc_mark_scan_object(NULL, tree, &queue));
    TEST_ASSERT_EQ_INT(GC_MARK_SCAN_INVALID,
                       gc_mark_scan_object(&objects[0], tree, NULL));
    objects[0].memory = NULL;
    TEST_ASSERT_EQ_INT(GC_MARK_SCAN_INVALID,
                       gc_mark_scan_object(&objects[0], tree, &queue));
    gc_mark_queue_destroy(&queue);
    return EXIT_SUCCESS;
}

int main(void)
{
    TEST_ASSERT(gc_mark_queue_pending(NULL) == (size_t)0);
    gc_mark_queue_init(NULL);
    gc_mark_queue_destroy(NULL);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_queue_order_and_duplicates());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_conservative_object_scan());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_scan_boundaries_and_invalid_candidates());
    puts("test_marker: ok");
    return EXIT_SUCCESS;
}
