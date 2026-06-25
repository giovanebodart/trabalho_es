#include "gc.h"
#include "gc_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FIRE_CANARY UINT32_C(0xC0DEC0DE)
#define FIRE_DEFAULT_MAX ((size_t)50000)
#define FIRE_FULL_MAX ((size_t)10000000)

typedef struct FireNode {
    struct FireNode *next;
    struct FireNode *cross;
    uint32_t canary;
    uint32_t id;
} FireNode;

static size_t parse_limit(int argc, char **argv)
{
    char *end = NULL;
    unsigned long long parsed;

    if (argc > 1 && argv[1] != NULL) {
        if (argv[1][0] == '-' && argv[1][1] == '-' && argv[1][2] == 'f') {
            return FIRE_FULL_MAX;
        }
        parsed = strtoull(argv[1], &end, 10);
        if (end != argv[1] && end != NULL && *end == '\0'
            && parsed > 0) {
            return (size_t)parsed;
        }
    }
    return FIRE_DEFAULT_MAX;
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

static int run_cycle(size_t count, size_t cycle)
{
    FireNode **nodes = calloc(count, sizeof *nodes);
    size_t root_count = count / (size_t)16 + (size_t)1;
    FireNode **roots = calloc(root_count, sizeof *roots);
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
    printf("ciclo=%zu objetos=%zu vivos=%zu coletados=%zu "
           "menores=%zu maiores=%zu\n",
           cycle, count, stats.bytes_live, stats.bytes_collected,
           stats.minor_collection_count, stats.major_collection_count);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    size_t max_count = parse_limit(argc, argv);
    size_t cycle;

    puts("Teste de fogo deterministico do coletor");
    if (gc_init() != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    for (cycle = 0; cycle < (size_t)3; ++cycle) {
        size_t count = max_count / ((size_t)4 - cycle);

        if (count == 0) {
            count = 1;
        }
        if (run_cycle(count, cycle + (size_t)1) != EXIT_SUCCESS) {
            gc_shutdown();
            return EXIT_FAILURE;
        }
    }
    gc_shutdown();
    return gc_get_status() == GC_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
