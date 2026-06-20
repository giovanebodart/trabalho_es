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

int interval_node_balance_factor(const IntervalNode *node)
{
    if (node == NULL) {
        return 0;
    }

    return interval_node_height(node->left)
           - interval_node_height(node->right);
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

IntervalNode *interval_node_rotate_left(IntervalNode *root)
{
    IntervalNode *pivot;

    if (root == NULL || root->right == NULL) {
        return root;
    }

    pivot = root->right;
    root->right = pivot->left;
    pivot->left = root;

    interval_node_update(root);
    interval_node_update(pivot);
    return pivot;
}

IntervalNode *interval_node_rotate_right(IntervalNode *root)
{
    IntervalNode *pivot;

    if (root == NULL || root->left == NULL) {
        return root;
    }

    pivot = root->left;
    root->left = pivot->right;
    pivot->right = root;

    interval_node_update(root);
    interval_node_update(pivot);
    return pivot;
}

IntervalNode *interval_node_rotate_left_right(IntervalNode *root)
{
    if (root == NULL || root->left == NULL || root->left->right == NULL) {
        return root;
    }

    root->left = interval_node_rotate_left(root->left);
    return interval_node_rotate_right(root);
}

IntervalNode *interval_node_rotate_right_left(IntervalNode *root)
{
    if (root == NULL || root->right == NULL || root->right->left == NULL) {
        return root;
    }

    root->right = interval_node_rotate_right(root->right);
    return interval_node_rotate_left(root);
}

static IntervalNode *interval_node_rebalance(IntervalNode *root)
{
    int balance;

    interval_node_update(root);
    balance = interval_node_balance_factor(root);

    if (balance > 1) {
        if (interval_node_balance_factor(root->left) < 0) {
            return interval_node_rotate_left_right(root);
        }
        return interval_node_rotate_right(root);
    }

    if (balance < -1) {
        if (interval_node_balance_factor(root->right) > 0) {
            return interval_node_rotate_right_left(root);
        }
        return interval_node_rotate_left(root);
    }

    return root;
}

static IntervalNode *interval_tree_insert_node(IntervalNode *root,
                                               IntervalNode *node,
                                               bool *inserted)
{
    if (root == NULL) {
        node->height = 1;
        node->max_end = node->end;
        *inserted = true;
        return node;
    }

    if (node->end <= root->start) {
        root->left = interval_tree_insert_node(root->left, node, inserted);
    } else if (node->start >= root->end) {
        root->right = interval_tree_insert_node(root->right, node, inserted);
    } else {
        return root;
    }

    if (!*inserted) {
        return root;
    }

    return interval_node_rebalance(root);
}

bool interval_tree_insert(IntervalNode **root, IntervalNode *node)
{
    bool inserted = false;

    if (root == NULL || node == NULL || node->start >= node->end
        || node->left != NULL || node->right != NULL) {
        return false;
    }

    *root = interval_tree_insert_node(*root, node, &inserted);
    return inserted;
}

IntervalNode *interval_tree_find_counted(IntervalNode *root,
                                         uintptr_t address,
                                         size_t *comparisons)
{
    size_t examined = 0;

    while (root != NULL) {
        ++examined;

        if (address >= root->start && address < root->end) {
            if (comparisons != NULL) {
                *comparisons = examined;
            }
            return root;
        }

        if (address < root->start) {
            if (root->left == NULL || address >= root->left->max_end) {
                break;
            }
            root = root->left;
        } else {
            root = root->right;
        }
    }

    if (comparisons != NULL) {
        *comparisons = examined;
    }
    return NULL;
}

IntervalNode *interval_tree_find(IntervalNode *root, uintptr_t address)
{
    return interval_tree_find_counted(root, address, NULL);
}
