#include "interval_tree.h"

#include <stddef.h>

static int max_height(int left, int right)
{
    return left > right ? left : right;
}

static uintptr_t max_address(uintptr_t first, uintptr_t second)
{
    return first > second ? first : second;
}

bool interval_node_init(IntervalNode *node, uintptr_t start, uintptr_t end)
{
    if (node == NULL || start >= end) {
        return false;
    }

    node->start = start;
    node->end = end;
    node->max_end = end;
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return true;
}

int interval_node_height(const IntervalNode *node)
{
    return node == NULL ? 0 : node->height;
}

uintptr_t interval_node_max_end(const IntervalNode *node)
{
    return node == NULL ? (uintptr_t)0 : node->max_end;
}

void interval_node_update(IntervalNode *node)
{
    uintptr_t child_max_end;

    if (node == NULL) {
        return;
    }

    node->height = 1 + max_height(interval_node_height(node->left),
                                  interval_node_height(node->right));

    child_max_end = max_address(interval_node_max_end(node->left),
                                interval_node_max_end(node->right));
    node->max_end = max_address(node->end, child_max_end);
}
