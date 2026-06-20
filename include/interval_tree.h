#ifndef INTERVAL_TREE_H
#define INTERVAL_TREE_H

#include <stdbool.h>
#include <stddef.h>
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
int interval_node_balance_factor(const IntervalNode *node);
void interval_node_update(IntervalNode *node);

IntervalNode *interval_node_rotate_left(IntervalNode *root);
IntervalNode *interval_node_rotate_right(IntervalNode *root);
IntervalNode *interval_node_rotate_left_right(IntervalNode *root);
IntervalNode *interval_node_rotate_right_left(IntervalNode *root);

bool interval_tree_insert(IntervalNode **root, IntervalNode *node);
bool interval_tree_remove(IntervalNode **root,
                          uintptr_t start,
                          IntervalNode **removed);
IntervalNode *interval_tree_find(IntervalNode *root, uintptr_t address);
/* comparisons receives the number of interval nodes examined. */
IntervalNode *interval_tree_find_counted(IntervalNode *root,
                                         uintptr_t address,
                                         size_t *comparisons);

#endif
