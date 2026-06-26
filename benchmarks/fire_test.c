#include "gc.h"
#include "gc_config.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIRE_CANARY UINT32_C(0xC0DEC0DE)
#define FIRE_DEFAULT_MAX ((size_t)50000)
#define FIRE_FULL_MAX ((size_t)10000000)

typedef struct FireNode {
    struct FireNode *next;
    struct FireNode *cross;
    uint32_t canary;
    uint32_t id;
} FireNode;

static void print_csv_header(void)
{
    puts("algorithm,seed,objects,heap_bytes,live_bytes,collected_bytes,"
         "pause_ticks,mark_ticks,sweep_ticks,tree_searches,tree_comparisons,"
         "minor_collections,major_collections,promoted_objects,dirty_pages,"
         "max_rss_bytes");
}

static bool parse_args(int argc, char **argv, size_t *limit, bool *csv)
{
    char *end = NULL;
    unsigned long long parsed;
    int index;

    if (limit == NULL || csv == NULL) {
        return false;
    }
    *limit = FIRE_DEFAULT_MAX;
    *csv = false;
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--csv") == 0) {
            *csv = true;
            continue;
        }
        if (strcmp(argv[index], "--full") == 0) {
            *limit = FIRE_FULL_MAX;
            continue;
        }
        parsed = strtoull(argv[index], &end, 10);
        if (argv[index][0] == '\0' || end == NULL || *end != '\0'
            || parsed == 0 || parsed > (unsigned long long)SIZE_MAX) {
            return false;
        }
        *limit = (size_t)parsed;
    }
    return true;
}

static int collect_until_major(void)
{
    size_t index;

    for (index = 0;
         index < GC_DEFAULT_MAJOR_COLLECTION_INTERVAL + (size_t)1;
         ++index) {
        gc_collect();
        if (gc_get_status() != GC_STATUS_OK) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

static int verify_roots(FireNode **roots, size_t root_count)
{
    size_t index;

    for (index = 0; index < root_count; ++index) {
        if (roots[index] != NULL
            && (roots[index]->canary != FIRE_CANARY
                || roots[index]->id == UINT32_MAX)) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

static int run_cycle(size_t count, size_t cycle, bool csv)
{
    FireNode **nodes = calloc(count, sizeof *nodes);
    size_t root_count = count / (size_t)16 + (size_t)1;
    FireNode **roots = calloc(root_count, sizeof *roots);
    GCStats peak = {0};
    GCStats stats;
    size_t index;

    if (nodes == NULL || roots == NULL) {
        free(nodes);
        free(roots);
        return EXIT_FAILURE;
    }

    for (index = 0; index < count; ++index) {
        nodes[index] = gc_malloc(sizeof **nodes);
        if (nodes[index] == NULL) {
            free(nodes);
            free(roots);
            return EXIT_FAILURE;
        }
        nodes[index]->canary = FIRE_CANARY;
        nodes[index]->id = (uint32_t)(index ^ (cycle << 16));
        nodes[index]->next = index == 0 ? nodes[index] : nodes[index - 1];
        nodes[index]->cross = nodes[index / (size_t)2];
        if (index > 2 && index % (size_t)97 == 0) {
            nodes[index - 2]->cross = nodes[index];
        }
    }
    if (count > 1) {
        nodes[0]->next = nodes[count - 1];
    }

    for (index = 0; index < root_count; ++index) {
        roots[index] = nodes[(index * (size_t)16) % count];
        if (gc_add_root((void **)&roots[index]) != GC_SUCCESS) {
            free(nodes);
            free(roots);
            return EXIT_FAILURE;
        }
    }
    if (gc_get_stats(&peak) != GC_SUCCESS) {
        free(nodes);
        free(roots);
        return EXIT_FAILURE;
    }

    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || verify_roots(roots, root_count) != EXIT_SUCCESS) {
        free(nodes);
        free(roots);
        return EXIT_FAILURE;
    }

    for (index = 1; index < root_count; index += (size_t)2) {
        if (gc_remove_root((void **)&roots[index]) != GC_SUCCESS) {
            free(nodes);
            free(roots);
            return EXIT_FAILURE;
        }
        roots[index] = NULL;
    }

    if (collect_until_major() != EXIT_SUCCESS
        || verify_roots(roots, root_count) != EXIT_SUCCESS) {
        free(nodes);
        free(roots);
        return EXIT_FAILURE;
    }

    for (index = 0; index < root_count; ++index) {
        if (roots[index] != NULL) {
            if (gc_remove_root((void **)&roots[index]) != GC_SUCCESS) {
                free(nodes);
                free(roots);
                return EXIT_FAILURE;
            }
            roots[index] = NULL;
        }
    }
    for (index = 0; index < count; ++index) {
        nodes[index] = NULL;
    }
    free(nodes);
    free(roots);

    if (collect_until_major() != EXIT_SUCCESS
        || gc_get_stats(&stats) != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (csv) {
        printf("generational,%zu,%zu,%zu,%zu,%zu,%llu,%llu,%llu,%zu,%zu,"
               "%zu,%zu,%zu,%zu,%zu\n",
               cycle, count, peak.bytes_reserved, stats.bytes_live,
               stats.bytes_collected,
               (unsigned long long)stats.pause_ticks,
               (unsigned long long)stats.mark_ticks,
               (unsigned long long)stats.sweep_ticks,
               stats.tree_searches, stats.tree_comparisons,
               stats.minor_collection_count, stats.major_collection_count,
               stats.promoted_objects, stats.last_dirty_pages,
               stats.max_resident_bytes);
    } else {
        printf("ciclo=%zu objetos=%zu vivos=%zu coletados=%zu "
               "menores=%zu maiores=%zu\n",
               cycle, count, stats.bytes_live, stats.bytes_collected,
               stats.minor_collection_count, stats.major_collection_count);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    size_t max_count;
    size_t cycle;
    bool csv;

    if (!parse_args(argc, argv, &max_count, &csv)) {
        fputs("uso: bench_fire_test.exe [objetos|--full] [--csv]\n",
              stderr);
        return EXIT_FAILURE;
    }
    if (csv) {
        print_csv_header();
    } else {
        puts("Teste de fogo deterministico do coletor");
    }
    if (gc_init() != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    for (cycle = 0; cycle < (size_t)3; ++cycle) {
        size_t count = max_count / ((size_t)4 - cycle);

        if (count == 0) {
            count = 1;
        }
        if (run_cycle(count, cycle + (size_t)1, csv) != EXIT_SUCCESS) {
            gc_shutdown();
            return EXIT_FAILURE;
        }
    }
    gc_shutdown();
    return gc_get_status() == GC_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
