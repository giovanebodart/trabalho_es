#include "marker.h"
#include "test.h"

#include <string.h>

#define TEST_OBJECT_COUNT 40

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

int main(void)
{
    TEST_ASSERT(gc_mark_queue_pending(NULL) == (size_t)0);
    gc_mark_queue_init(NULL);
    gc_mark_queue_destroy(NULL);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_queue_order_and_duplicates());
    puts("test_marker: ok");
    return EXIT_SUCCESS;
}
