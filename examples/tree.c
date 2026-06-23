#include "gc.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    struct TreeNode *left;
    struct TreeNode *right;
    int value;
} TreeNode;

static TreeNode *new_node(int value)
{
    TreeNode *node = gc_malloc(sizeof *node);

    if (node != NULL) {
        node->left = NULL;
        node->right = NULL;
        node->value = value;
    }
    return node;
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

int main(void)
{
    void *root = NULL;
    TreeNode *nodes[7];
    size_t index;

    if (gc_init() != GC_SUCCESS || gc_add_root(&root) != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    for (index = 0; index < (size_t)7; ++index) {
        nodes[index] = new_node((int)index);
        if (nodes[index] == NULL) {
            gc_shutdown();
            return EXIT_FAILURE;
        }
    }
    nodes[0]->left = nodes[1];
    nodes[0]->right = nodes[2];
    nodes[1]->left = nodes[3];
    nodes[1]->right = nodes[4];
    nodes[2]->left = nodes[5];
    nodes[2]->right = nodes[6];
    root = nodes[0];

    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_collection(7, 0, sizeof *nodes[0] * (size_t)7)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }
    root = NULL;
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || !validate_collection(0, 7, 0)) {
        gc_shutdown();
        return EXIT_FAILURE;
    }
    gc_shutdown();
    puts("example_tree: ok");
    return EXIT_SUCCESS;
}
