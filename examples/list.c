#include "gc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined (__GNUC__) 
#define GC_EXAMPLE_NOINLINE __attribute__((noinline))
#else
#define GC_EXAMPLE_NOINLINE
#endif

typedef struct ListNode {
    struct ListNode *next;
    int value;
} ListNode;

static GC_EXAMPLE_NOINLINE ListNode *build_list(void)
{
    ListNode *head = NULL;
    size_t index;

    for (index = 0; index < (size_t)5; ++index) {
        ListNode *node = gc_malloc(sizeof *node);

        if (node == NULL) {
            return NULL;
        }
        node->value = (int)index;
        node->next = head;
        head = node;
    }
    return head;
}

static GC_EXAMPLE_NOINLINE void scrub_stack_roots(void)
{
    volatile uintptr_t noise[256];
    size_t index;

    for (index = 0; index < sizeof noise / sizeof noise[0]; ++index) {
        noise[index] = (uintptr_t)0;
    }
}

static int validate_collection(size_t examined, size_t collected,
                               size_t live)
{
    GCStats stats;

    return gc_get_stats(&stats) == GC_SUCCESS
           && stats.last_objects_examined == examined
           && stats.last_objects_collected == collected
           && stats.bytes_live == live;
}

static int validate_conservative_drop(size_t total, size_t object_size)
{
    GCStats stats;

    return gc_get_stats(&stats) == GC_SUCCESS
           && stats.last_objects_examined <= total
           && stats.last_objects_collected
              == total - stats.last_objects_examined
           && stats.bytes_live
              == stats.last_objects_examined * object_size
           && stats.bytes_collected
              == stats.last_objects_collected * object_size;
}

int main(void)
{
    void *root = NULL;
    ListNode *head = NULL;

    if (gc_init() != GC_SUCCESS || gc_add_root(&root) != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    head = build_list();
    if (head == NULL) {
        gc_shutdown();
        return EXIT_FAILURE;
    }
    root = head;
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_collection(5, 0, sizeof *head * (size_t)5)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }

    root = NULL;
    head = NULL;
    scrub_stack_roots();
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_conservative_drop(5, sizeof *head)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }
    gc_shutdown();
    puts("example_list: ok");
    return EXIT_SUCCESS;
}
