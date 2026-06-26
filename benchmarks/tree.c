#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "interval_tree.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TREE_STAGE_COUNT ((size_t)4)
#define TREE_REPEAT_COUNT ((size_t)3)
#define TREE_DEFAULT_MAX ((size_t)100000)
#define TREE_FULL_MAX ((size_t)1000000)
#define TREE_INTERVAL_STRIDE ((uintptr_t)16)

static const size_t tree_stages[TREE_STAGE_COUNT] = {
    1000u, 10000u, 100000u, 1000000u
};

static uint32_t next_random(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static void shuffle(size_t *items, size_t count, uint32_t seed)
{
    size_t index;

    for (index = count; index > (size_t)1; --index) {
        size_t swap = (size_t)(next_random(&seed) % (uint32_t)index);
        size_t tmp = items[index - (size_t)1];

        items[index - (size_t)1] = items[swap];
        items[swap] = tmp;
    }
}

static size_t ceil_log2_size(size_t value)
{
    size_t result = 0;
    size_t power = 1;

    while (power < value && power <= SIZE_MAX / (size_t)2) {
        power *= (size_t)2;
        ++result;
    }
    return result;
}

static bool parse_args(int argc, char **argv, size_t *limit, bool *csv)
{
    unsigned long long parsed;
    char *end = NULL;
    int index;

    if (limit == NULL || csv == NULL) {
        return false;
    }
    *limit = TREE_DEFAULT_MAX;
    *csv = false;
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--csv") == 0) {
            *csv = true;
            continue;
        }
        if (strcmp(argv[index], "--full") == 0) {
            *limit = TREE_FULL_MAX;
            continue;
        }
        parsed = strtoull(argv[index], &end, 10);
        if (argv[index][0] == '\0' || end == NULL || *end != '\0'
            || parsed == 0 || parsed > (unsigned long long)TREE_FULL_MAX) {
            return false;
        }
        *limit = (size_t)parsed;
    }
    return true;
}

static bool prepare_nodes(IntervalNode *nodes, size_t *order, size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        uintptr_t start;

        order[index] = index;
        if (index > (size_t)((UINTPTR_MAX - 8u)
                             / TREE_INTERVAL_STRIDE)) {
            return false;
        }
        start = (uintptr_t)index * TREE_INTERVAL_STRIDE;
        if (!interval_node_init(&nodes[index], start, start + 8u)) {
            return false;
        }
    }
    return true;
}

static bool run_case(size_t count, size_t repeat, bool csv)
{
    IntervalNode *nodes = calloc(count, sizeof *nodes);
    size_t *order = calloc(count, sizeof *order);
    uint32_t seed = UINT32_C(0xC0FFEE) + (uint32_t)repeat;
    LARGE_INTEGER start;
    LARGE_INTEGER inserted;
    LARGE_INTEGER found;
    LARGE_INTEGER removed;
    IntervalNode *root = NULL;
    size_t comparisons = 0;
    size_t index;
    int height;

    if (nodes == NULL || order == NULL
        || !prepare_nodes(nodes, order, count)
        || !QueryPerformanceCounter(&start)) {
        free(nodes);
        free(order);
        return false;
    }
    shuffle(order, count, seed);
    for (index = 0; index < count; ++index) {
        if (!interval_tree_insert(&root, &nodes[order[index]])) {
            free(nodes);
            free(order);
            return false;
        }
    }
#ifndef NDEBUG
    if (!interval_tree_validate(root)) {
        free(nodes);
        free(order);
        return false;
    }
#endif
    height = interval_node_height(root);
    if (!QueryPerformanceCounter(&inserted)) {
        free(nodes);
        free(order);
        return false;
    }

    shuffle(order, count, seed ^ UINT32_C(0xA5A5A5A5));
    for (index = 0; index < count; ++index) {
        size_t current = 0;

        if (interval_tree_find_counted(root,
                                       nodes[order[index]].start + 3u,
                                       &current)
            != &nodes[order[index]]) {
            free(nodes);
            free(order);
            return false;
        }
        comparisons += current;
    }
    if (!QueryPerformanceCounter(&found)) {
        free(nodes);
        free(order);
        return false;
    }

    shuffle(order, count, seed ^ UINT32_C(0x5A5A5A5A));
    for (index = 0; index < count; ++index) {
        IntervalNode *detached = NULL;

        if (!interval_tree_remove(&root, nodes[order[index]].start,
                                  &detached)
            || detached != &nodes[order[index]]) {
            free(nodes);
            free(order);
            return false;
        }
    }
    if (root != NULL || !QueryPerformanceCounter(&removed)) {
        free(nodes);
        free(order);
        return false;
    }

    if (csv) {
        printf("%zu,%u,%zu,%d,%zu,%lld,%lld,%lld,%.2f\n",
               count, seed, repeat, height, ceil_log2_size(count),
               inserted.QuadPart - start.QuadPart,
               found.QuadPart - inserted.QuadPart,
               removed.QuadPart - found.QuadPart,
               (double)comparisons / (double)count);
    } else {
        printf("%9zu | %08x | %3zu | %6d | %4zu | %10lld | %10lld | %10lld | %8.2f\n",
               count, seed, repeat, height, ceil_log2_size(count),
               inserted.QuadPart - start.QuadPart,
               found.QuadPart - inserted.QuadPart,
               removed.QuadPart - found.QuadPart,
               (double)comparisons / (double)count);
    }
    free(nodes);
    free(order);
    return true;
}

int main(int argc, char **argv)
{
    LARGE_INTEGER frequency;
    size_t limit;
    size_t stage;
    size_t repeat;
    bool ran_limit = false;
    bool csv;

    if (!parse_args(argc, argv, &limit, &csv)
        || !QueryPerformanceFrequency(&frequency)) {
        fputs("uso: bench_tree.exe [objetos|--full] [--csv]\n", stderr);
        return EXIT_FAILURE;
    }
    if (csv) {
        puts("nodes,seed,repeat,height,ceil_log2,insert_ticks,find_ticks,remove_ticks,avg_find_comparisons");
    } else {
        puts("Benchmark da arvore de intervalos");
        printf("%9s | %8s | %3s | %6s | %4s | %10s | %10s | %10s | %8s\n",
               "nos", "semente", "rep", "altura", "log2",
               "insert", "find", "remove", "cmp_find");
        puts("----------+----------+-----+--------+------+------------+------------+------------+----------");
    }
    for (stage = 0; stage < TREE_STAGE_COUNT
         && tree_stages[stage] <= limit; ++stage) {
        for (repeat = 1; repeat <= TREE_REPEAT_COUNT; ++repeat) {
            if (!run_case(tree_stages[stage], repeat, csv)) {
                return EXIT_FAILURE;
            }
        }
        ran_limit = ran_limit || tree_stages[stage] == limit;
    }
    if (!ran_limit) {
        for (repeat = 1; repeat <= TREE_REPEAT_COUNT; ++repeat) {
            if (!run_case(limit, repeat, csv)) {
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}
