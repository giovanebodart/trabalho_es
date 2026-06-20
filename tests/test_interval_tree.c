#include "interval_tree.h"
#include "test.h"

static int test_rejects_invalid_intervals(void)
{
    IntervalNode node;

    TEST_ASSERT(!interval_node_init(NULL, (uintptr_t)10, (uintptr_t)20));
    TEST_ASSERT(!interval_node_init(&node, (uintptr_t)20, (uintptr_t)20));
    TEST_ASSERT(!interval_node_init(&node, (uintptr_t)30, (uintptr_t)20));
    return EXIT_SUCCESS;
}

static int test_initializes_leaf(void)
{
    IntervalNode node;

    TEST_ASSERT(interval_node_init(&node, (uintptr_t)100, (uintptr_t)200));
    TEST_ASSERT(node.start == (uintptr_t)100);
    TEST_ASSERT(node.end == (uintptr_t)200);
    TEST_ASSERT(node.max_end == (uintptr_t)200);
    TEST_ASSERT_EQ_INT(1, node.height);
    TEST_ASSERT(node.left == NULL);
    TEST_ASSERT(node.right == NULL);
    return EXIT_SUCCESS;
}

static int test_accepts_address_boundaries(void)
{
    IntervalNode node;

    TEST_ASSERT(interval_node_init(&node, (uintptr_t)0, (uintptr_t)1));
    TEST_ASSERT(node.max_end == (uintptr_t)1);

    TEST_ASSERT(interval_node_init(&node, UINTPTR_MAX - (uintptr_t)1,
                                   UINTPTR_MAX));
    TEST_ASSERT(node.start == UINTPTR_MAX - (uintptr_t)1);
    TEST_ASSERT(node.end == UINTPTR_MAX);
    TEST_ASSERT(node.max_end == UINTPTR_MAX);
    return EXIT_SUCCESS;
}

static int test_updates_subtree_metadata(void)
{
    IntervalNode root;
    IntervalNode left;
    IntervalNode right;
    IntervalNode right_right;

    TEST_ASSERT(interval_node_init(&root, (uintptr_t)200, (uintptr_t)250));
    TEST_ASSERT(interval_node_init(&left, (uintptr_t)100, (uintptr_t)150));
    TEST_ASSERT(interval_node_init(&right, (uintptr_t)300, (uintptr_t)350));
    TEST_ASSERT(interval_node_init(&right_right,
                                   (uintptr_t)400,
                                   (uintptr_t)450));

    right.right = &right_right;
    interval_node_update(&right);
    root.left = &left;
    root.right = &right;
    interval_node_update(&root);

    TEST_ASSERT_EQ_INT(2, right.height);
    TEST_ASSERT(right.max_end == (uintptr_t)450);
    TEST_ASSERT_EQ_INT(3, root.height);
    TEST_ASSERT(root.max_end == (uintptr_t)450);
    return EXIT_SUCCESS;
}

static int test_null_helpers_and_own_end(void)
{
    IntervalNode root;
    IntervalNode left;

    TEST_ASSERT_EQ_INT(0, interval_node_height(NULL));
    TEST_ASSERT(interval_node_max_end(NULL) == (uintptr_t)0);
    interval_node_update(NULL);

    TEST_ASSERT(interval_node_init(&root, (uintptr_t)500, (uintptr_t)600));
    TEST_ASSERT(interval_node_init(&left, (uintptr_t)100, (uintptr_t)200));
    root.left = &left;
    interval_node_update(&root);

    TEST_ASSERT_EQ_INT(2, root.height);
    TEST_ASSERT(root.max_end == (uintptr_t)600);
    return EXIT_SUCCESS;
}

static int test_rotate_right(void)
{
    IntervalNode root;
    IntervalNode pivot;
    IntervalNode left;
    IntervalNode transfer;
    IntervalNode *rotated;

    TEST_ASSERT(interval_node_init(&root, (uintptr_t)300, (uintptr_t)500));
    TEST_ASSERT(interval_node_init(&pivot, (uintptr_t)200, (uintptr_t)250));
    TEST_ASSERT(interval_node_init(&left, (uintptr_t)100, (uintptr_t)150));
    TEST_ASSERT(interval_node_init(&transfer, (uintptr_t)275, (uintptr_t)290));

    pivot.left = &left;
    pivot.right = &transfer;
    interval_node_update(&pivot);
    root.left = &pivot;
    interval_node_update(&root);

    rotated = interval_node_rotate_right(&root);

    TEST_ASSERT(rotated == &pivot);
    TEST_ASSERT(rotated->left == &left);
    TEST_ASSERT(rotated->right == &root);
    TEST_ASSERT(root.left == &transfer);
    TEST_ASSERT(left.end <= rotated->start);
    TEST_ASSERT(rotated->end <= transfer.start);
    TEST_ASSERT(transfer.end <= root.start);
    TEST_ASSERT_EQ_INT(3, rotated->height);
    TEST_ASSERT_EQ_INT(-1, interval_node_balance_factor(rotated));
    TEST_ASSERT(rotated->max_end == (uintptr_t)500);
    TEST_ASSERT(root.max_end == (uintptr_t)500);
    return EXIT_SUCCESS;
}

static int test_rotate_left(void)
{
    IntervalNode root;
    IntervalNode pivot;
    IntervalNode transfer;
    IntervalNode right;
    IntervalNode *rotated;

    TEST_ASSERT(interval_node_init(&root, (uintptr_t)100, (uintptr_t)150));
    TEST_ASSERT(interval_node_init(&pivot, (uintptr_t)300, (uintptr_t)350));
    TEST_ASSERT(interval_node_init(&transfer, (uintptr_t)200, (uintptr_t)250));
    TEST_ASSERT(interval_node_init(&right, (uintptr_t)400, (uintptr_t)500));

    pivot.left = &transfer;
    pivot.right = &right;
    interval_node_update(&pivot);
    root.right = &pivot;
    interval_node_update(&root);

    rotated = interval_node_rotate_left(&root);

    TEST_ASSERT(rotated == &pivot);
    TEST_ASSERT(rotated->left == &root);
    TEST_ASSERT(rotated->right == &right);
    TEST_ASSERT(root.right == &transfer);
    TEST_ASSERT(root.end <= transfer.start);
    TEST_ASSERT(transfer.end <= rotated->start);
    TEST_ASSERT(rotated->end <= right.start);
    TEST_ASSERT_EQ_INT(3, rotated->height);
    TEST_ASSERT_EQ_INT(1, interval_node_balance_factor(rotated));
    TEST_ASSERT(rotated->max_end == (uintptr_t)500);
    TEST_ASSERT(root.max_end == (uintptr_t)250);
    return EXIT_SUCCESS;
}

static int test_double_rotations(void)
{
    IntervalNode left_root;
    IntervalNode left;
    IntervalNode left_right;
    IntervalNode right_root;
    IntervalNode right;
    IntervalNode right_left;
    IntervalNode *rotated;

    TEST_ASSERT(interval_node_init(&left_root,
                                   (uintptr_t)300,
                                   (uintptr_t)350));
    TEST_ASSERT(interval_node_init(&left, (uintptr_t)100, (uintptr_t)150));
    TEST_ASSERT(interval_node_init(&left_right,
                                   (uintptr_t)200,
                                   (uintptr_t)250));
    left.right = &left_right;
    interval_node_update(&left);
    left_root.left = &left;
    interval_node_update(&left_root);

    rotated = interval_node_rotate_left_right(&left_root);
    TEST_ASSERT(rotated == &left_right);
    TEST_ASSERT(rotated->left == &left);
    TEST_ASSERT(rotated->right == &left_root);
    TEST_ASSERT(left.end <= rotated->start);
    TEST_ASSERT(rotated->end <= left_root.start);
    TEST_ASSERT_EQ_INT(2, rotated->height);
    TEST_ASSERT_EQ_INT(0, interval_node_balance_factor(rotated));
    TEST_ASSERT(rotated->max_end == (uintptr_t)350);

    TEST_ASSERT(interval_node_init(&right_root,
                                   (uintptr_t)100,
                                   (uintptr_t)150));
    TEST_ASSERT(interval_node_init(&right, (uintptr_t)300, (uintptr_t)450));
    TEST_ASSERT(interval_node_init(&right_left,
                                   (uintptr_t)200,
                                   (uintptr_t)250));
    right.left = &right_left;
    interval_node_update(&right);
    right_root.right = &right;
    interval_node_update(&right_root);

    rotated = interval_node_rotate_right_left(&right_root);
    TEST_ASSERT(rotated == &right_left);
    TEST_ASSERT(rotated->left == &right_root);
    TEST_ASSERT(rotated->right == &right);
    TEST_ASSERT(right_root.end <= rotated->start);
    TEST_ASSERT(rotated->end <= right.start);
    TEST_ASSERT_EQ_INT(2, rotated->height);
    TEST_ASSERT_EQ_INT(0, interval_node_balance_factor(rotated));
    TEST_ASSERT(rotated->max_end == (uintptr_t)450);
    return EXIT_SUCCESS;
}

static int test_rotation_preconditions(void)
{
    IntervalNode root;

    TEST_ASSERT(interval_node_rotate_left(NULL) == NULL);
    TEST_ASSERT(interval_node_rotate_right(NULL) == NULL);
    TEST_ASSERT(interval_node_rotate_left_right(NULL) == NULL);
    TEST_ASSERT(interval_node_rotate_right_left(NULL) == NULL);

    TEST_ASSERT(interval_node_init(&root, (uintptr_t)100, (uintptr_t)200));
    TEST_ASSERT(interval_node_rotate_left(&root) == &root);
    TEST_ASSERT(interval_node_rotate_right(&root) == &root);
    TEST_ASSERT(interval_node_rotate_left_right(&root) == &root);
    TEST_ASSERT(interval_node_rotate_right_left(&root) == &root);
    TEST_ASSERT_EQ_INT(1, root.height);
    TEST_ASSERT(root.max_end == (uintptr_t)200);
    return EXIT_SUCCESS;
}

int main(void)
{
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_rejects_invalid_intervals());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_initializes_leaf());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_accepts_address_boundaries());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_updates_subtree_metadata());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_null_helpers_and_own_end());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_rotate_right());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_rotate_left());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_double_rotations());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_rotation_preconditions());
    puts("test_interval_tree: ok");
    return EXIT_SUCCESS;
}
