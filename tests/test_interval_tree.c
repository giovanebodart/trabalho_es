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

int main(void)
{
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_rejects_invalid_intervals());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_initializes_leaf());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_accepts_address_boundaries());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_updates_subtree_metadata());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_null_helpers_and_own_end());
    puts("test_interval_tree: ok");
    return EXIT_SUCCESS;
}
