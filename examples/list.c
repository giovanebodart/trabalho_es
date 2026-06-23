#include "gc.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct ListNode {
    struct ListNode *next;
    int value;
} ListNode;

static int validate_collection(size_t examined, size_t collected,
                               size_t live)
{
    GCStats stats;

    return gc_get_stats(&stats) == GC_SUCCESS
           && stats.last_objects_examined == examined
           && stats.last_objects_collected == collected
           && stats.bytes_live == live;
}

int main(void)
{
    void *root = NULL;
    ListNode *head = NULL;
    size_t index;

    if (gc_init() != GC_SUCCESS || gc_add_root(&root) != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    for (index = 0; index < (size_t)5; ++index) {
        ListNode *node = gc_malloc(sizeof *node);

        if (node == NULL) {
            gc_shutdown();
            return EXIT_FAILURE;
        }
        node->value = (int)index;
        node->next = head;
        head = node;
    }
    root = head;
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_collection(5, 0, sizeof *head * (size_t)5)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }

    root = NULL;
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_collection(0, 5, 0)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }
    gc_shutdown();
    puts("example_list: ok");
    return EXIT_SUCCESS;
}
