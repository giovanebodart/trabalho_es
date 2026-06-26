#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gc.h"
#include "gc_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMPARE_DEFAULT_OBJECTS ((size_t)50000)
#define COMPARE_REPEAT_COUNT ((size_t)3)
#define COMPARE_CANARY UINT32_C(0xBADC0DE)

typedef struct CompareNode {
    struct CompareNode *next;
    struct CompareNode *cross;
    uint32_t canary;
    uint32_t id;
} CompareNode;

typedef struct {
    uint64_t elapsed_ticks;
    uint64_t pause_ticks;
    size_t collected_bytes;
    size_t minor_collections;
    size_t major_collections;
    size_t promoted_objects;
} CompareSample;

static int compare_u64(const void *left, const void *right)
{
    uint64_t a = *(const uint64_t *)left;
    uint64_t b = *(const uint64_t *)right;

    return (a > b) - (a < b);
}

static uint64_t median_u64(uint64_t *values, size_t count)
{
    qsort(values, count, sizeof *values, compare_u64);
    return values[count / (size_t)2];
}

static uint64_t mean_u64(const uint64_t *values, size_t count)
{
    uint64_t total = 0;
    size_t index;

    for (index = 0; index < count; ++index) {
        total += values[index];
    }
    return total / count;
}

static uint64_t range_u64(const uint64_t *values, size_t count)
{
    uint64_t min = values[0];
    uint64_t max = values[0];
    size_t index;

    for (index = 1; index < count; ++index) {
        if (values[index] < min) {
            min = values[index];
        }
        if (values[index] > max) {
            max = values[index];
        }
    }
    return max - min;
}

static bool parse_args(int argc, char **argv, size_t *objects, bool *csv)
{
    char *end = NULL;
    unsigned long long parsed;
    int index;

    if (objects == NULL || csv == NULL) {
        return false;
    }
    *objects = COMPARE_DEFAULT_OBJECTS;
    *csv = false;
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--csv") == 0) {
            *csv = true;
            continue;
        }
        parsed = strtoull(argv[index], &end, 10);
        if (argv[index][0] == '\0' || end == NULL || *end != '\0'
            || parsed == 0 || parsed > (unsigned long long)SIZE_MAX) {
            return false;
        }
        *objects = (size_t)parsed;
    }
    return true;
}

static void scrub_stack(void)
{
    volatile uintptr_t noise[256];
    size_t index;

    for (index = 0; index < sizeof noise / sizeof noise[0]; ++index) {
        noise[index] = (uintptr_t)0;
    }
}

static bool allocate_graph(CompareNode **nodes, size_t count, size_t seed)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        nodes[index] = gc_malloc(sizeof *nodes[index]);
        if (nodes[index] == NULL) {
            return false;
        }
        nodes[index]->canary = COMPARE_CANARY;
        nodes[index]->id = (uint32_t)(index ^ seed);
        nodes[index]->next = index == 0 ? nodes[index] : nodes[index - 1];
        nodes[index]->cross = nodes[(index * (size_t)7) / (size_t)11];
        if (index > (size_t)3 && index % (size_t)53 == 0) {
            nodes[index - (size_t)3]->cross = nodes[index];
        }
    }
    return true;
}

static bool run_workload(size_t count, size_t seed, bool pure_mark_sweep,
                         CompareSample *sample)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    CompareNode **nodes = calloc(count, sizeof *nodes);
    size_t root_count = count / (size_t)32 + (size_t)1;
    CompareNode **roots = calloc(root_count, sizeof *roots);
    GCStats stats;
    size_t index;
    bool ok = false;

    if (nodes == NULL || roots == NULL || sample == NULL
        || gc_init() != GC_SUCCESS) {
        free(nodes);
        free(roots);
        return false;
    }
    if (pure_mark_sweep
        && gc_set_major_collection_interval(0) != GC_SUCCESS) {
        goto done;
    }
    if (!QueryPerformanceCounter(&start)
        || !allocate_graph(nodes, count, seed)) {
        goto done;
    }
    for (index = 0; index < root_count; ++index) {
        roots[index] = nodes[(index * (size_t)32) % count];
        if (gc_add_root((void **)&roots[index]) != GC_SUCCESS) {
            goto done;
        }
    }
    gc_collect();
    for (index = 1; index < root_count; index += (size_t)2) {
        if (gc_remove_root((void **)&roots[index]) != GC_SUCCESS) {
            goto done;
        }
        roots[index] = NULL;
    }
    gc_collect();
    for (index = 0; index < root_count; ++index) {
        if (roots[index] != NULL
            && gc_remove_root((void **)&roots[index]) != GC_SUCCESS) {
            goto done;
        }
        roots[index] = NULL;
    }
    for (index = 0; index < count; ++index) {
        nodes[index] = NULL;
    }
    scrub_stack();
    for (index = 0;
         index < GC_DEFAULT_MAJOR_COLLECTION_INTERVAL + (size_t)1;
         ++index) {
        gc_collect();
    }
    if (!QueryPerformanceCounter(&end)
        || gc_get_stats(&stats) != GC_SUCCESS
        || stats.bytes_live != (size_t)0) {
        goto done;
    }
    sample->elapsed_ticks = (uint64_t)(end.QuadPart - start.QuadPart);
    sample->pause_ticks = stats.pause_ticks;
    sample->collected_bytes = stats.bytes_collected;
    sample->minor_collections = stats.minor_collection_count;
    sample->major_collections = stats.major_collection_count;
    sample->promoted_objects = stats.promoted_objects;
    ok = true;

done:
    gc_shutdown();
    free(nodes);
    free(roots);
    return ok;
}

static bool collect_samples(const char *name, size_t count,
                            bool pure_mark_sweep, bool csv)
{
    CompareSample samples[COMPARE_REPEAT_COUNT];
    uint64_t elapsed[COMPARE_REPEAT_COUNT];
    uint64_t pauses[COMPARE_REPEAT_COUNT];
    size_t index;

    if (!run_workload(count / (size_t)4 + (size_t)1,
                      (size_t)17, pure_mark_sweep, &samples[0])) {
        return false;
    }
    for (index = 0; index < COMPARE_REPEAT_COUNT; ++index) {
        if (!run_workload(count, index + (size_t)1,
                          pure_mark_sweep, &samples[index])) {
            return false;
        }
        elapsed[index] = samples[index].elapsed_ticks;
        pauses[index] = samples[index].pause_ticks;
    }
    if (csv) {
        printf("%s,%zu,%zu,%llu,%llu,%llu,%llu,%llu,%llu,%zu,%zu,%zu,%zu\n",
               name, count, COMPARE_REPEAT_COUNT,
               (unsigned long long)mean_u64(pauses, COMPARE_REPEAT_COUNT),
               (unsigned long long)median_u64(pauses, COMPARE_REPEAT_COUNT),
               (unsigned long long)range_u64(pauses, COMPARE_REPEAT_COUNT),
               (unsigned long long)mean_u64(elapsed, COMPARE_REPEAT_COUNT),
               (unsigned long long)median_u64(elapsed, COMPARE_REPEAT_COUNT),
               (unsigned long long)range_u64(elapsed, COMPARE_REPEAT_COUNT),
               samples[COMPARE_REPEAT_COUNT - (size_t)1].collected_bytes,
               samples[COMPARE_REPEAT_COUNT - (size_t)1].minor_collections,
               samples[COMPARE_REPEAT_COUNT - (size_t)1].major_collections,
               samples[COMPARE_REPEAT_COUNT - (size_t)1].promoted_objects);
    } else {
        printf("%-12s | %8zu | %3zu | %12llu | %12llu | %12llu | %12llu | %12llu | %12llu\n",
               name, count, COMPARE_REPEAT_COUNT,
               (unsigned long long)mean_u64(pauses, COMPARE_REPEAT_COUNT),
               (unsigned long long)median_u64(pauses, COMPARE_REPEAT_COUNT),
               (unsigned long long)range_u64(pauses, COMPARE_REPEAT_COUNT),
               (unsigned long long)mean_u64(elapsed, COMPARE_REPEAT_COUNT),
               (unsigned long long)median_u64(elapsed, COMPARE_REPEAT_COUNT),
               (unsigned long long)range_u64(elapsed, COMPARE_REPEAT_COUNT));
    }
    return true;
}

int main(int argc, char **argv)
{
    size_t objects;
    bool csv;

    if (!parse_args(argc, argv, &objects, &csv)) {
        fputs("uso: bench_compare_collectors.exe [objetos] [--csv]\n",
              stderr);
        return EXIT_FAILURE;
    }
    if (csv) {
        puts("algorithm,objects,repetitions,mean_pause_ticks,median_pause_ticks,pause_range_ticks,mean_elapsed_ticks,median_elapsed_ticks,elapsed_range_ticks,collected_bytes,minor_collections,major_collections,promoted_objects");
    } else {
        puts("Comparacao entre mark-sweep puro e coletor geracional");
        printf("%-12s | %8s | %3s | %12s | %12s | %12s | %12s | %12s | %12s\n",
               "algoritmo", "objetos", "rep", "pause_mean",
               "pause_med", "pause_rng", "elapsed_mean", "elapsed_med",
               "elapsed_rng");
        puts("-------------+----------+-----+--------------+--------------+--------------+--------------+--------------+--------------");
    }
    return collect_samples("mark_sweep", objects, true, csv)
           && collect_samples("generational", objects, false, csv)
           ? EXIT_SUCCESS
           : EXIT_FAILURE;
}
