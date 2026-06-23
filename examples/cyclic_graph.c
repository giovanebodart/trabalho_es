#include "gc.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct GraphNode {
    struct GraphNode *first;
    struct GraphNode *second;
    int id;
} GraphNode;

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
    GraphNode *nodes[4];
    size_t index;

    if (gc_init() != GC_SUCCESS || gc_add_root(&root) != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    for (index = 0; index < (size_t)4; ++index) {
        nodes[index] = gc_malloc(sizeof *nodes[index]);
        if (nodes[index] == NULL) {
            gc_shutdown();
            return EXIT_FAILURE;
        }
        nodes[index]->id = (int)index;
    }
    nodes[0]->first = nodes[1];
    nodes[0]->second = nodes[2];
    nodes[1]->first = nodes[2];
    nodes[1]->second = nodes[3];
    nodes[2]->first = nodes[0];
    nodes[2]->second = nodes[3];
    nodes[3]->first = nodes[1];
    nodes[3]->second = nodes[3];
    root = nodes[0];

    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_collection(4, 0, sizeof *nodes[0] * (size_t)4)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }
    root = NULL;
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_collection(0, 4, 0)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }
    gc_shutdown();
    puts("example_cyclic_graph: ok");
    return EXIT_SUCCESS;
}
