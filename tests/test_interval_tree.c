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
    puts("test_interval_tree: ok");
    return EXIT_SUCCESS;
}
