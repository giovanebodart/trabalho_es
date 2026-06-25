#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCALE_STAGE_COUNT ((size_t)5)
#define SCALE_DEFAULT_MAX ((size_t)100000)
#define SCALE_LIMIT_HEADROOM ((size_t)64 * 1024u * 1024u)

#if defined(__GNUC__)
#define SCALE_NOINLINE __attribute__((noinline))
#else
#define SCALE_NOINLINE
#endif

typedef struct {
    uintptr_t tag;
    unsigned char payload[24];
} ScaleObject;

static const size_t scale_stages[SCALE_STAGE_COUNT] = {
    1000u,
    10000u,
    100000u,
    1000000u,
    10000000u
};

static double ticks_to_ms(uint64_t ticks, uint64_t frequency)
{
    if (frequency == 0) {
        return 0.0;
    }
    return (double)ticks * 1000.0 / (double)frequency;
}

static void print_legend(void)
{
    puts("Benchmark gradual de alocacoes do coletor");
    puts("");
    puts("Legenda:");
    puts("  objetos        quantidade de objetos alocados no estagio");
    puts("  tempo_ms       tempo total do estagio, incluindo alocacao e coleta");
    puts("  pausa_ms       duracao da ultima pausa de gc_collect()");
    puts("  reservado      bytes reservados no pico antes da coleta");
    puts("  vivos_pos_gc   bytes vivos depois da coleta");
    puts("  coletados      bytes recuperados pela coleta");
    puts("");
    printf("%10s | %10s | %10s | %14s | %12s | %12s\n",
           "objetos", "tempo_ms", "pausa_ms", "reservado",
           "vivos_pos_gc", "coletados");
    puts("-----------+------------+------------+----------------+--------------+-------------");
}

static void print_csv_header(void)
{
    puts("algorithm,seed,objects,heap_bytes,live_bytes,collected_bytes,"
         "mark_ticks,sweep_ticks,tree_searches,tree_comparisons,"
         "minor_collections,major_collections,promoted_objects,dirty_pages,"
         "max_rss_bytes");
}

static void print_csv_row(size_t count, const GCStats *peak,
                          const GCStats *after)
{
    printf("generational,0,%zu,%zu,%zu,%zu,0,0,0,0,%zu,%zu,0,0,0\n",
           count, peak->bytes_reserved, after->bytes_live,
           after->bytes_collected, after->minor_collection_count,
           after->major_collection_count);
}

static bool parse_args(int argc, char **argv, size_t *limit, bool *csv)
{
    unsigned long long parsed;
    char *end = NULL;
    int index;

    if (limit == NULL || csv == NULL) {
        return false;
    }
    *limit = SCALE_DEFAULT_MAX;
    *csv = false;
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--csv") == 0) {
            *csv = true;
            continue;
        }
        if (strcmp(argv[index], "--full") == 0) {
            *limit = scale_stages[SCALE_STAGE_COUNT - (size_t)1];
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

static bool memory_limit_for_stage(size_t count, size_t *limit)
{
    size_t scale;

    if (limit == NULL
        || count > (SIZE_MAX - SCALE_LIMIT_HEADROOM) / (size_t)128) {
        return false;
    }
    scale = count * (size_t)128;
    *limit = scale + SCALE_LIMIT_HEADROOM;
    return true;
}

static SCALE_NOINLINE void scrub_stack(void)
{
    volatile uintptr_t noise[512];
    size_t index;

    for (index = 0; index < sizeof noise / sizeof noise[0]; ++index) {
        noise[index] = (uintptr_t)0;
    }
}

static SCALE_NOINLINE bool allocate_objects(size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        ScaleObject *object = gc_malloc(sizeof *object);

        if (object == NULL) {
            return false;
        }
        object->tag = ((uintptr_t)index + (uintptr_t)1)
                      * (uintptr_t)2654435761u;
        object->payload[index % sizeof object->payload] =
            (unsigned char)(index & (size_t)0xff);
        object = NULL;
    }
    scrub_stack();
    return true;
}

static bool run_stage(size_t count, bool csv)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
    GCStats peak = {0};
    GCStats after = {0};
    size_t expected;
    size_t limit;
    bool ok = false;

    if (!memory_limit_for_stage(count, &limit)
        || count > SIZE_MAX / sizeof(ScaleObject)) {
        return false;
    }
    expected = count * sizeof(ScaleObject);

    if (gc_init() != GC_SUCCESS || gc_set_memory_limit(limit) != GC_SUCCESS
        || !QueryPerformanceFrequency(&frequency)
        || !QueryPerformanceCounter(&start)) {
        gc_shutdown();
        return false;
    }
    if (allocate_objects(count)
        && gc_get_stats(&peak) == GC_SUCCESS
        && peak.bytes_requested == expected) {
        scrub_stack();
        gc_collect();
        ok = gc_get_status() == GC_STATUS_OK
             && gc_get_stats(&after) == GC_SUCCESS
             && after.bytes_requested == expected
             && after.bytes_collected > 0
             && QueryPerformanceCounter(&end);
    }
    if (ok) {
        if (csv) {
            print_csv_row(count, &peak, &after);
        } else {
            printf("%10zu | %10.3f | %10.3f | %14zu | %12zu | %12zu\n",
                   count,
                   ticks_to_ms((uint64_t)(end.QuadPart - start.QuadPart),
                               (uint64_t)frequency.QuadPart),
                   ticks_to_ms(after.last_pause_ticks,
                               after.performance_frequency),
                   peak.bytes_reserved,
                   after.bytes_live,
                   after.bytes_collected);
        }
    }
    gc_shutdown();
    return ok;
}

int main(int argc, char **argv)
{
    size_t limit;
    size_t index;
    bool ran_limit = false;
    bool csv;

    if (!parse_args(argc, argv, &limit, &csv)) {
        fputs("uso: bench_scale_allocations.exe [objetos|--full] [--csv]\n",
              stderr);
        return EXIT_FAILURE;
    }

    if (csv) {
        print_csv_header();
    } else {
        print_legend();
    }
    for (index = 0; index < SCALE_STAGE_COUNT; ++index) {
        if (scale_stages[index] > limit) {
            break;
        }
        if (!run_stage(scale_stages[index], csv)) {
            return EXIT_FAILURE;
        }
        ran_limit = ran_limit || scale_stages[index] == limit;
    }
    if (!ran_limit && !run_stage(limit, csv)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
