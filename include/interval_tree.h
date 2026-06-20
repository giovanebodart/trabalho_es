#ifndef INTERVAL_TREE_H
#define INTERVAL_TREE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct IntervalNode IntervalNode;

struct IntervalNode {
    uintptr_t start;
    uintptr_t end;
    uintptr_t max_end;
    int height;
    IntervalNode *left;
    IntervalNode *right;
};

/*
 * Initializes a node for the half-open interval [start, end).
 * Empty and reversed intervals are rejected.
 */
bool interval_node_init(IntervalNode *node, uintptr_t start, uintptr_t end);

int interval_node_height(const IntervalNode *node);
uintptr_t interval_node_max_end(const IntervalNode *node);
void interval_node_update(IntervalNode *node);

#endif
