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

static int validate_inserted_tree(const IntervalNode *node,
                                  uintptr_t lower,
                                  uintptr_t upper,
                                  int *height,
                                  uintptr_t *max_end)
{
    int left_height;
    int right_height;
    uintptr_t left_max_end;
    uintptr_t right_max_end;
    uintptr_t expected_max_end;
    int balance;

    if (node == NULL) {
        *height = 0;
        *max_end = (uintptr_t)0;
        return EXIT_SUCCESS;
    }

    TEST_ASSERT(lower <= node->start);
    TEST_ASSERT(node->start < node->end);
    TEST_ASSERT(node->end <= upper);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       validate_inserted_tree(node->left, lower, node->start,
                                              &left_height, &left_max_end));
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       validate_inserted_tree(node->right, node->end, upper,
                                              &right_height, &right_max_end));

    *height = 1 + (left_height > right_height ? left_height : right_height);
    expected_max_end = node->end;
    if (left_max_end > expected_max_end) {
        expected_max_end = left_max_end;
    }
    if (right_max_end > expected_max_end) {
        expected_max_end = right_max_end;
    }

    balance = left_height - right_height;
    TEST_ASSERT(balance >= -1 && balance <= 1);
    TEST_ASSERT_EQ_INT(*height, node->height);
    TEST_ASSERT(node->max_end == expected_max_end);
    *max_end = expected_max_end;
    return EXIT_SUCCESS;
}

static int insert_sequence(const uintptr_t *starts, size_t count)
{
    IntervalNode nodes[16];
    IntervalNode *root = NULL;
    int height;
    uintptr_t max_end;
    uintptr_t expected_max_end = (uintptr_t)0;
    size_t index;

    TEST_ASSERT(count <= 16);
    for (index = 0; index < count; ++index) {
        TEST_ASSERT(interval_node_init(&nodes[index], starts[index],
                                       starts[index] + (uintptr_t)5));
        TEST_ASSERT(interval_tree_insert(&root, &nodes[index]));
        if (nodes[index].end > expected_max_end) {
            expected_max_end = nodes[index].end;
        }
    }

    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       validate_inserted_tree(root, (uintptr_t)0, UINTPTR_MAX,
                                              &height, &max_end));
    TEST_ASSERT(max_end == expected_max_end);
    return EXIT_SUCCESS;
}

static int test_insert_sequences(void)
{
    static const uintptr_t increasing[] = {
        10, 20, 30, 40, 50, 60, 70, 80
    };
    static const uintptr_t decreasing[] = {
        80, 70, 60, 50, 40, 30, 20, 10
    };
    static const uintptr_t pseudo_random[] = {
        90, 20, 150, 40, 110, 10, 160, 70,
        130, 50, 100, 30, 140, 80, 60, 120
    };

    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       insert_sequence(increasing,
                                       sizeof increasing / sizeof *increasing));
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       insert_sequence(decreasing,
                                       sizeof decreasing / sizeof *decreasing));
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       insert_sequence(pseudo_random,
                                       sizeof pseudo_random
                                       / sizeof *pseudo_random));
    return EXIT_SUCCESS;
}

static int test_rejects_overlaps_without_changes(void)
{
    IntervalNode root_node;
    IntervalNode left;
    IntervalNode right;
    IntervalNode overlap_start;
    IntervalNode overlap_end;
    IntervalNode contained;
    IntervalNode containing;
    IntervalNode duplicate;
    IntervalNode *root = NULL;
    int original_height;
    uintptr_t original_max_end;

    TEST_ASSERT(interval_node_init(&root_node, (uintptr_t)100,
                                   (uintptr_t)200));
    TEST_ASSERT(interval_node_init(&left, (uintptr_t)50, (uintptr_t)100));
    TEST_ASSERT(interval_node_init(&right, (uintptr_t)200, (uintptr_t)250));
    TEST_ASSERT(interval_tree_insert(&root, &root_node));
    TEST_ASSERT(interval_tree_insert(&root, &left));
    TEST_ASSERT(interval_tree_insert(&root, &right));
    original_height = root->height;
    original_max_end = root->max_end;

    TEST_ASSERT(interval_node_init(&overlap_start, (uintptr_t)90,
                                   (uintptr_t)110));
    TEST_ASSERT(interval_node_init(&overlap_end, (uintptr_t)190,
                                   (uintptr_t)210));
    TEST_ASSERT(interval_node_init(&contained, (uintptr_t)120,
                                   (uintptr_t)150));
    TEST_ASSERT(interval_node_init(&containing, (uintptr_t)40,
                                   (uintptr_t)260));
    TEST_ASSERT(interval_node_init(&duplicate, (uintptr_t)100,
                                   (uintptr_t)200));

    TEST_ASSERT(!interval_tree_insert(&root, &overlap_start));
    TEST_ASSERT(!interval_tree_insert(&root, &overlap_end));
    TEST_ASSERT(!interval_tree_insert(&root, &contained));
    TEST_ASSERT(!interval_tree_insert(&root, &containing));
    TEST_ASSERT(!interval_tree_insert(&root, &duplicate));
    TEST_ASSERT_EQ_INT(original_height, root->height);
    TEST_ASSERT(root->max_end == original_max_end);
    TEST_ASSERT(root->left == &left);
    TEST_ASSERT(root->right == &right);
    return EXIT_SUCCESS;
}

static int test_insert_preconditions(void)
{
    IntervalNode node;
    IntervalNode attached;
    IntervalNode child;
    IntervalNode *root = NULL;

    TEST_ASSERT(!interval_tree_insert(NULL, NULL));
    TEST_ASSERT(!interval_tree_insert(&root, NULL));
    TEST_ASSERT(interval_node_init(&node, (uintptr_t)10, (uintptr_t)20));
    TEST_ASSERT(!interval_tree_insert(NULL, &node));

    TEST_ASSERT(interval_node_init(&attached, (uintptr_t)30, (uintptr_t)40));
    TEST_ASSERT(interval_node_init(&child, (uintptr_t)20, (uintptr_t)25));
    attached.left = &child;
    TEST_ASSERT(!interval_tree_insert(&root, &attached));
    TEST_ASSERT(root == NULL);
    return EXIT_SUCCESS;
}

static int test_find_boundaries_and_interior(void)
{
    IntervalNode nodes[4];
    IntervalNode *root = NULL;
    IntervalNode *found;

    TEST_ASSERT(interval_node_init(&nodes[0], (uintptr_t)100,
                                   (uintptr_t)110));
    TEST_ASSERT(interval_node_init(&nodes[1], (uintptr_t)200,
                                   (uintptr_t)210));
    TEST_ASSERT(interval_node_init(&nodes[2], (uintptr_t)300,
                                   (uintptr_t)310));
    TEST_ASSERT(interval_node_init(&nodes[3], UINTPTR_MAX - (uintptr_t)1,
                                   UINTPTR_MAX));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[0]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[1]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[2]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[3]));

    found = interval_tree_find(root, (uintptr_t)200);
    TEST_ASSERT(found == &nodes[1]);
    found = interval_tree_find(root, (uintptr_t)205);
    TEST_ASSERT(found == &nodes[1]);
    found = interval_tree_find(root, (uintptr_t)209);
    TEST_ASSERT(found == &nodes[1]);
    TEST_ASSERT(interval_tree_find(root, (uintptr_t)210) == NULL);
    TEST_ASSERT(interval_tree_find(root, (uintptr_t)99) == NULL);
    TEST_ASSERT(interval_tree_find(root, (uintptr_t)150) == NULL);
    TEST_ASSERT(interval_tree_find(root, (uintptr_t)311) == NULL);
    TEST_ASSERT(interval_tree_find(root, UINTPTR_MAX - (uintptr_t)1)
                == &nodes[3]);
    TEST_ASSERT(interval_tree_find(root, UINTPTR_MAX) == NULL);
    TEST_ASSERT(interval_tree_find(NULL, (uintptr_t)200) == NULL);
    return EXIT_SUCCESS;
}

static int test_find_comparison_counts(void)
{
    IntervalNode nodes[31];
    IntervalNode *root = NULL;
    IntervalNode *found;
    size_t comparisons;
    size_t index;

    for (index = 0; index < 31; ++index) {
        const uintptr_t start = (uintptr_t)(index * 10 + 10);

        TEST_ASSERT(interval_node_init(&nodes[index], start,
                                       start + (uintptr_t)5));
        TEST_ASSERT(interval_tree_insert(&root, &nodes[index]));
    }

    comparisons = 0;
    found = interval_tree_find_counted(root, root->start, &comparisons);
    TEST_ASSERT(found == root);
    TEST_ASSERT(comparisons == (size_t)1);

    for (index = 0; index < 31; ++index) {
        comparisons = 0;
        found = interval_tree_find_counted(root, nodes[index].start,
                                           &comparisons);
        TEST_ASSERT(found == &nodes[index]);
        TEST_ASSERT(comparisons >= (size_t)1);
        TEST_ASSERT(comparisons <= (size_t)root->height);
    }

    comparisons = 0;
    TEST_ASSERT(interval_tree_find_counted(root, (uintptr_t)0,
                                           &comparisons) == NULL);
    TEST_ASSERT(comparisons <= (size_t)root->height);
    comparisons = 0;
    TEST_ASSERT(interval_tree_find_counted(root, (uintptr_t)999,
                                           &comparisons) == NULL);
    TEST_ASSERT(comparisons <= (size_t)root->height);
    return EXIT_SUCCESS;
}

static int test_find_prunes_left_subtree(void)
{
    IntervalNode root;
    IntervalNode left;
    IntervalNode left_left;
    IntervalNode left_right;
    IntervalNode right;
    size_t comparisons = 99;

    TEST_ASSERT(interval_node_init(&root, (uintptr_t)400, (uintptr_t)410));
    TEST_ASSERT(interval_node_init(&left, (uintptr_t)200, (uintptr_t)210));
    TEST_ASSERT(interval_node_init(&left_left,
                                   (uintptr_t)100,
                                   (uintptr_t)110));
    TEST_ASSERT(interval_node_init(&left_right,
                                   (uintptr_t)300,
                                   (uintptr_t)310));
    TEST_ASSERT(interval_node_init(&right, (uintptr_t)500, (uintptr_t)510));
    left.left = &left_left;
    left.right = &left_right;
    interval_node_update(&left);
    root.left = &left;
    root.right = &right;
    interval_node_update(&root);

    TEST_ASSERT(interval_tree_find_counted(&root, (uintptr_t)350,
                                           &comparisons) == NULL);
    TEST_ASSERT(comparisons == (size_t)1);

    comparisons = 99;
    TEST_ASSERT(interval_tree_find_counted(NULL, (uintptr_t)350,
                                           &comparisons) == NULL);
    TEST_ASSERT(comparisons == (size_t)0);
    return EXIT_SUCCESS;
}

static int validate_tree_after_removal(IntervalNode *root)
{
    int height;
    uintptr_t max_end;

    return validate_inserted_tree(root, (uintptr_t)0, UINTPTR_MAX,
                                  &height, &max_end);
}

static int validate_detached_node(const IntervalNode *node)
{
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->left == NULL);
    TEST_ASSERT(node->right == NULL);
    TEST_ASSERT_EQ_INT(1, node->height);
    TEST_ASSERT(node->max_end == node->end);
    return EXIT_SUCCESS;
}

static int test_remove_leaf(void)
{
    IntervalNode nodes[3];
    IntervalNode *root = NULL;
    IntervalNode *removed = NULL;

    TEST_ASSERT(interval_node_init(&nodes[0], (uintptr_t)40, (uintptr_t)45));
    TEST_ASSERT(interval_node_init(&nodes[1], (uintptr_t)20, (uintptr_t)25));
    TEST_ASSERT(interval_node_init(&nodes[2], (uintptr_t)60, (uintptr_t)65));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[0]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[1]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[2]));

    TEST_ASSERT(interval_tree_remove(&root, nodes[1].start, &removed));
    TEST_ASSERT(removed == &nodes[1]);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_detached_node(removed));
    TEST_ASSERT(interval_tree_find(root, nodes[1].start) == NULL);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_tree_after_removal(root));

    TEST_ASSERT(interval_tree_insert(&root, removed));
    TEST_ASSERT(interval_tree_find(root, nodes[1].start) == &nodes[1]);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_tree_after_removal(root));
    return EXIT_SUCCESS;
}

static int test_remove_node_with_one_child(void)
{
    IntervalNode nodes[4];
    IntervalNode *root = NULL;
    IntervalNode *removed = NULL;

    TEST_ASSERT(interval_node_init(&nodes[0], (uintptr_t)40, (uintptr_t)45));
    TEST_ASSERT(interval_node_init(&nodes[1], (uintptr_t)20, (uintptr_t)25));
    TEST_ASSERT(interval_node_init(&nodes[2], (uintptr_t)60, (uintptr_t)65));
    TEST_ASSERT(interval_node_init(&nodes[3], (uintptr_t)10, (uintptr_t)15));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[0]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[1]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[2]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[3]));
    TEST_ASSERT(nodes[1].left == &nodes[3]);

    TEST_ASSERT(interval_tree_remove(&root, nodes[1].start, &removed));
    TEST_ASSERT(removed == &nodes[1]);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_detached_node(removed));
    TEST_ASSERT(interval_tree_find(root, nodes[3].start) == &nodes[3]);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_tree_after_removal(root));
    return EXIT_SUCCESS;
}

static int test_remove_root_with_two_children(void)
{
    static const uintptr_t starts[] = {40, 20, 60, 10, 30, 50, 70, 55};
    IntervalNode nodes[8];
    IntervalNode *root = NULL;
    IntervalNode *removed = NULL;
    size_t index;

    for (index = 0; index < 8; ++index) {
        TEST_ASSERT(interval_node_init(&nodes[index], starts[index],
                                       starts[index] + (uintptr_t)5));
        TEST_ASSERT(interval_tree_insert(&root, &nodes[index]));
    }
    TEST_ASSERT(root == &nodes[0]);
    TEST_ASSERT(root->left != NULL);
    TEST_ASSERT(root->right != NULL);

    TEST_ASSERT(interval_tree_remove(&root, nodes[0].start, &removed));
    TEST_ASSERT(removed == &nodes[0]);
    TEST_ASSERT(root == &nodes[5]);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_detached_node(removed));
    TEST_ASSERT(interval_tree_find(root, removed->start) == NULL);
    for (index = 1; index < 8; ++index) {
        TEST_ASSERT(interval_tree_find(root, nodes[index].start)
                    == &nodes[index]);
    }
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_tree_after_removal(root));
    return EXIT_SUCCESS;
}

static int test_remove_rebalances_and_handles_missing(void)
{
    static const uintptr_t starts[] = {40, 20, 60, 10, 30, 50, 70, 5};
    IntervalNode nodes[8];
    IntervalNode *root = NULL;
    IntervalNode *original_root;
    IntervalNode *removed = &nodes[0];
    int original_height;
    uintptr_t original_max_end;
    size_t index;

    for (index = 0; index < 8; ++index) {
        TEST_ASSERT(interval_node_init(&nodes[index], starts[index],
                                       starts[index] + (uintptr_t)5));
        TEST_ASSERT(interval_tree_insert(&root, &nodes[index]));
    }

    original_root = root;
    original_height = root->height;
    original_max_end = root->max_end;
    TEST_ASSERT(!interval_tree_remove(&root, (uintptr_t)999, &removed));
    TEST_ASSERT(removed == NULL);
    TEST_ASSERT(root == original_root);
    TEST_ASSERT_EQ_INT(original_height, root->height);
    TEST_ASSERT(root->max_end == original_max_end);
    TEST_ASSERT(interval_tree_remove(&root, nodes[6].start, NULL));
    TEST_ASSERT(interval_tree_remove(&root, nodes[2].start, &removed));
    TEST_ASSERT(removed == &nodes[2]);
    TEST_ASSERT(root == &nodes[1]);
    TEST_ASSERT(root->max_end == (uintptr_t)55);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_detached_node(removed));
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_tree_after_removal(root));

    TEST_ASSERT(!interval_tree_remove(NULL, (uintptr_t)20, &removed));
    TEST_ASSERT(removed == NULL);
    return EXIT_SUCCESS;
}

static int test_remove_single_root(void)
{
    IntervalNode node;
    IntervalNode *root = NULL;

    TEST_ASSERT(interval_node_init(&node, (uintptr_t)100, (uintptr_t)110));
    TEST_ASSERT(interval_tree_insert(&root, &node));
    TEST_ASSERT(interval_tree_remove(&root, node.start, NULL));
    TEST_ASSERT(root == NULL);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, validate_detached_node(&node));
    return EXIT_SUCCESS;
}

static int test_validator_detects_corruption(void)
{
    IntervalNode nodes[3];
    IntervalNode unbalanced[3];
    IntervalNode *root = NULL;

    TEST_ASSERT(interval_tree_validate(NULL));
    TEST_ASSERT(interval_node_init(&nodes[0], (uintptr_t)20, (uintptr_t)25));
    TEST_ASSERT(interval_node_init(&nodes[1], (uintptr_t)10, (uintptr_t)15));
    TEST_ASSERT(interval_node_init(&nodes[2], (uintptr_t)30, (uintptr_t)35));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[0]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[1]));
    TEST_ASSERT(interval_tree_insert(&root, &nodes[2]));
    TEST_ASSERT(interval_tree_validate(root));

    nodes[1].start = (uintptr_t)40;
    nodes[1].end = (uintptr_t)45;
    TEST_ASSERT(!interval_tree_validate(root));
    nodes[1].start = (uintptr_t)10;
    nodes[1].end = (uintptr_t)15;

    nodes[1].end = (uintptr_t)22;
    TEST_ASSERT(!interval_tree_validate(root));
    nodes[1].end = (uintptr_t)15;

    ++root->height;
    TEST_ASSERT(!interval_tree_validate(root));
    --root->height;

    --root->max_end;
    TEST_ASSERT(!interval_tree_validate(root));
    ++root->max_end;
    TEST_ASSERT(interval_tree_validate(root));

    TEST_ASSERT(interval_node_init(&unbalanced[0],
                                   (uintptr_t)30, (uintptr_t)35));
    TEST_ASSERT(interval_node_init(&unbalanced[1],
                                   (uintptr_t)20, (uintptr_t)25));
    TEST_ASSERT(interval_node_init(&unbalanced[2],
                                   (uintptr_t)10, (uintptr_t)15));
    unbalanced[1].left = &unbalanced[2];
    interval_node_update(&unbalanced[1]);
    unbalanced[0].left = &unbalanced[1];
    interval_node_update(&unbalanced[0]);
    TEST_ASSERT(!interval_tree_validate(&unbalanced[0]));
    return EXIT_SUCCESS;
}

static uint32_t next_random(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static int test_randomized_operations(void)
{
    enum { NODE_COUNT = 64, OPERATION_COUNT = 2048 };
    IntervalNode nodes[NODE_COUNT];
    bool active[NODE_COUNT] = {false};
    IntervalNode *root = NULL;
    uint32_t random_state = UINT32_C(0x5eed1234);
    size_t operation;
    size_t index;

    for (index = 0; index < NODE_COUNT; ++index) {
        const uintptr_t start = (uintptr_t)(index * (size_t)16 + (size_t)8);

        TEST_ASSERT(interval_node_init(&nodes[index], start,
                                       start + (uintptr_t)8));
    }

    for (operation = 0; operation < OPERATION_COUNT; ++operation) {
        IntervalNode *removed = NULL;
        IntervalNode *found;
        uintptr_t address;

        index = (size_t)(next_random(&random_state) % NODE_COUNT);
        if (active[index]) {
            TEST_ASSERT(interval_tree_remove(&root, nodes[index].start,
                                             &removed));
            TEST_ASSERT(removed == &nodes[index]);
            active[index] = false;
        } else {
            TEST_ASSERT(interval_tree_insert(&root, &nodes[index]));
            active[index] = true;
        }
        TEST_ASSERT(interval_tree_validate(root));

        index = (size_t)(next_random(&random_state) % NODE_COUNT);
        address = nodes[index].start
                  + (uintptr_t)(next_random(&random_state) % UINT32_C(8));
        found = interval_tree_find(root, address);
        TEST_ASSERT(found == (active[index] ? &nodes[index] : NULL));
    }

    for (index = 0; index < NODE_COUNT; ++index) {
        if (active[index]) {
            TEST_ASSERT(interval_tree_remove(&root, nodes[index].start, NULL));
            TEST_ASSERT(interval_tree_validate(root));
        }
    }
    TEST_ASSERT(root == NULL);
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
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_insert_sequences());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_rejects_overlaps_without_changes());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_insert_preconditions());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_find_boundaries_and_interior());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_find_comparison_counts());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_find_prunes_left_subtree());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_remove_leaf());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_remove_node_with_one_child());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_remove_root_with_two_children());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_remove_rebalances_and_handles_missing());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_remove_single_root());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_validator_detects_corruption());
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS, test_randomized_operations());
    puts("test_interval_tree: ok");
    return EXIT_SUCCESS;
}
