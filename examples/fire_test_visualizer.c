#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gc.h"
#include "gc_config.h"
#include "gc_internal.h"

#include <conio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIRE_VIS_CANARY UINT32_C(0xC0DEC0DE)
#define FIRE_VIS_SCALE_COUNT ((size_t)5)
#define FIRE_VIS_LIMIT_HEADROOM ((size_t)128 * 1024u * 1024u)
#define FIRE_VIS_MILLION_OBJECTS ((size_t)1000000)
#define FIRE_VIS_FULL_OBJECTS ((size_t)10000000)
#define FIRE_VIS_MILLION_ROOTS ((size_t)100)
#define FIRE_VIS_FULL_ROOTS ((size_t)1000)

#if defined(__GNUC__)
#define FIRE_VIS_NOINLINE __attribute__((noinline))
#else
#define FIRE_VIS_NOINLINE
#endif

typedef struct FireNode {
    struct FireNode *next;
    struct FireNode *cross;
    uint32_t canary;
    uint32_t id;
} FireNode;

typedef struct {
    FireNode **nodes;
    FireNode **roots;
    size_t count;
    size_t root_count;
    size_t registered_roots;
    size_t allocated_objects;
    size_t inserted_intervals;
    bool active;
} FireScenario;

typedef struct {
    size_t collections;
    size_t minor_collections;
    size_t major_collections;
    size_t marked_objects;
    size_t collected_objects;
    size_t tree_searches;
    size_t tree_comparisons;
    size_t last_tree_searches;
    size_t last_tree_comparisons;
    size_t before_intervals;
    size_t after_intervals;
    uint64_t pause_ticks;
    uint64_t mark_ticks;
    uint64_t sweep_ticks;
    uint64_t last_pause_ticks;
    uint64_t last_minor_pause_ticks;
    uint64_t last_major_pause_ticks;
    uint64_t last_mark_ticks;
    uint64_t last_sweep_ticks;
    uint64_t frequency;
    uint64_t operation_ticks;
} CleanupReport;

static const size_t fire_scales[FIRE_VIS_SCALE_COUNT] = {
    1000u,
    10000u,
    100000u,
    FIRE_VIS_MILLION_OBJECTS,
    FIRE_VIS_FULL_OBJECTS
};

static double ticks_to_ms(uint64_t ticks, uint64_t frequency)
{
    if (frequency == 0) {
        return 0.0;
    }
    return (double)ticks * 1000.0 / (double)frequency;
}

static void wait_key(void)
{
    puts("\nPressione qualquer tecla para continuar...");
    (void)_getch();
}

static FIRE_VIS_NOINLINE void scrub_stack_roots(void)
{
    volatile uintptr_t noise[512];
    size_t index;

    for (index = 0; index < sizeof noise / sizeof noise[0]; ++index) {
        noise[index] = (uintptr_t)0;
    }
}

static bool memory_limit_for_scale(size_t count, size_t *limit)
{
    size_t scaled;

    if (limit == NULL
        || count > (SIZE_MAX - FIRE_VIS_LIMIT_HEADROOM) / (size_t)192) {
        return false;
    }
    scaled = count * (size_t)192;
    *limit = scaled + FIRE_VIS_LIMIT_HEADROOM;
    return true;
}

static size_t root_count_for_scale(size_t count)
{
    if (count >= FIRE_VIS_FULL_OBJECTS) {
        return FIRE_VIS_FULL_ROOTS;
    }
    if (count >= FIRE_VIS_MILLION_OBJECTS) {
        return FIRE_VIS_MILLION_ROOTS;
    }
    return count / (size_t)16 + (size_t)1;
}

static void reset_scenario(FireScenario *scenario)
{
    memset(scenario, 0, sizeof *scenario);
}

static void release_scenario_arrays(FireScenario *scenario)
{
    free(scenario->nodes);
    free(scenario->roots);
    scenario->nodes = NULL;
    scenario->roots = NULL;
    scenario->count = 0;
    scenario->root_count = 0;
    scenario->registered_roots = 0;
    scenario->allocated_objects = 0;
    scenario->inserted_intervals = 0;
    scenario->active = false;
}

static void print_header(size_t count)
{
    puts("=== Visualizador do fire_test ===");
    printf("Escala selecionada: %zu objetos\n\n", count);
    puts("Como ler:");
    puts("  alocados     objetos criados com gc_malloc()");
    puts("  intervalos   objetos presentes na arvore de intervalos");
    puts("  marcados     objetos examinados pela fase mark");
    puts("  coletados    objetos removidos pela fase sweep");
    puts("  tempos       convertidos de QueryPerformanceCounter para ms");
    puts("  arvore       buscas e comparacoes feitas durante marcacao\n");
}

static void print_allocation_progress(size_t allocated)
{
    printf("  alocados=%zu | intervalos na arvore=%zu\n",
           allocated, gc_internal_allocation_count());
}

static void print_allocation_summary(const FireScenario *scenario,
                                     const GCStats *stats,
                                     size_t before_intervals)
{
    puts("\nAlocacao concluida:");
    printf("  objetos alocados:      %zu\n", scenario->allocated_objects);
    printf("  intervalos antes:      %zu\n", before_intervals);
    printf("  intervalos inseridos:  %zu\n",
           scenario->inserted_intervals);
    printf("  intervalos atuais:     %zu\n",
           gc_internal_allocation_count());
    printf("  raizes registradas:    %zu\n", scenario->registered_roots);
    printf("  bytes solicitados:     %zu\n", stats->bytes_requested);
    printf("  bytes reservados:      %zu\n", stats->bytes_reserved);
    printf("  bytes vivos:           %zu\n", stats->bytes_live);
    printf("  memoria residente max: %zu\n", stats->max_resident_bytes);
}

static int register_roots(FireScenario *scenario)
{
    size_t progress_step;
    size_t stride;
    size_t index;

    progress_step = scenario->root_count / (size_t)10;
    if (progress_step == 0) {
        progress_step = 1;
    }
    stride = scenario->count / scenario->root_count;
    if (stride == 0) {
        stride = 1;
    }
    printf("Registrando %zu raizes explicitas...\n",
           scenario->root_count);
    for (index = 0; index < scenario->root_count; ++index) {
        scenario->roots[index] =
            scenario->nodes[(index * stride) % scenario->count];
        if (gc_add_root((void **)&scenario->roots[index]) != GC_SUCCESS) {
            return EXIT_FAILURE;
        }
        ++scenario->registered_roots;
        if ((index + (size_t)1) % progress_step == 0
            || index + (size_t)1 == scenario->root_count) {
            printf("  raizes=%zu/%zu\n", index + (size_t)1,
                   scenario->root_count);
        }
    }
    return EXIT_SUCCESS;
}

static int allocate_scale(FireScenario *scenario, size_t count)
{
    GCStats stats = {0};
    size_t before_intervals;
    size_t progress_step;
    size_t memory_limit;
    size_t index;

    if (scenario->active) {
        puts("Ja existe uma escala alocada. Execute a limpeza antes.");
        return EXIT_FAILURE;
    }
    if (!memory_limit_for_scale(count, &memory_limit)
        || gc_set_memory_limit(memory_limit) != GC_SUCCESS) {
        puts("Nao foi possivel configurar limite de memoria para a escala.");
        return EXIT_FAILURE;
    }

    reset_scenario(scenario);
    scenario->count = count;
    scenario->root_count = root_count_for_scale(count);
    scenario->nodes = calloc(count, sizeof *scenario->nodes);
    scenario->roots = calloc(scenario->root_count, sizeof *scenario->roots);
    if (scenario->nodes == NULL || scenario->roots == NULL) {
        release_scenario_arrays(scenario);
        puts("Falha ao reservar estruturas auxiliares do visualizador.");
        return EXIT_FAILURE;
    }

    before_intervals = gc_internal_allocation_count();
    progress_step = count / (size_t)10;
    if (progress_step == 0) {
        progress_step = 1;
    }
    puts("Alocando objetos e inserindo intervalos na arvore...");
    for (index = 0; index < count; ++index) {
        FireNode *node = gc_malloc(sizeof *node);

        if (node == NULL) {
            scenario->active = true;
            printf("gc_malloc falhou apos %zu objetos. status=%d\n",
                   index, (int)gc_get_status());
            return EXIT_FAILURE;
        }
        node->canary = FIRE_VIS_CANARY;
        node->id = (uint32_t)(index + (size_t)1);
        node->next = index == 0 ? node : scenario->nodes[index - (size_t)1];
        node->cross = scenario->nodes[index / (size_t)2];
        scenario->nodes[index] = node;
        scenario->allocated_objects = index + (size_t)1;
        if (index > (size_t)2 && index % (size_t)97 == 0) {
            scenario->nodes[index - (size_t)2]->cross = node;
        }
        if ((index + (size_t)1) % progress_step == 0
            || index + (size_t)1 == count) {
            print_allocation_progress(index + (size_t)1);
        }
    }
    if (count > (size_t)1) {
        scenario->nodes[0]->next = scenario->nodes[count - (size_t)1];
    }
    scenario->inserted_intervals =
        gc_internal_allocation_count() >= before_intervals
        ? gc_internal_allocation_count() - before_intervals
        : (size_t)0;
    scenario->active = true;

    if (register_roots(scenario) != EXIT_SUCCESS
        || gc_get_stats(&stats) != GC_SUCCESS) {
        puts("Falha ao registrar raizes da escala alocada.");
        return EXIT_FAILURE;
    }
    print_allocation_summary(scenario, &stats, before_intervals);
    return EXIT_SUCCESS;
}

static void remove_registered_roots(FireScenario *scenario)
{
    size_t index;

    for (index = 0; index < scenario->registered_roots; ++index) {
        if (scenario->roots[index] != NULL) {
            (void)gc_remove_root((void **)&scenario->roots[index]);
            scenario->roots[index] = NULL;
        }
    }
    scenario->registered_roots = 0;
}

static void forget_external_references(FireScenario *scenario)
{
    size_t index;

    for (index = 0; index < scenario->count; ++index) {
        scenario->nodes[index] = NULL;
    }
    for (index = 0; index < scenario->root_count; ++index) {
        scenario->roots[index] = NULL;
    }
}

static void accumulate_last_collection(CleanupReport *report,
                                       const GCStats *stats)
{
    report->marked_objects += stats->last_objects_examined;
    report->collected_objects += stats->last_objects_collected;
}

static int collect_cleanup_sequence(FireScenario *scenario,
                                    CleanupReport *report)
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    GCStats before = {0};
    GCStats after = {0};
    size_t limit = GC_DEFAULT_MAJOR_COLLECTION_INTERVAL + (size_t)3;
    size_t pass;

    memset(report, 0, sizeof *report);
    report->before_intervals = gc_internal_allocation_count();
    if (gc_get_stats(&before) != GC_SUCCESS
        || !QueryPerformanceCounter(&start)) {
        return EXIT_FAILURE;
    }

    puts("Coletando uma vez com raizes registradas para mostrar marcacao...");
    scrub_stack_roots();
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || gc_get_stats(&after) != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    accumulate_last_collection(report, &after);

    puts("Removendo raizes e esquecendo referencias externas...");
    remove_registered_roots(scenario);
    forget_external_references(scenario);
    release_scenario_arrays(scenario);

    for (pass = 0; pass < limit; ++pass) {
        scrub_stack_roots();
        gc_collect();
        if (gc_get_status() != GC_STATUS_OK
            || gc_get_stats(&after) != GC_SUCCESS) {
            return EXIT_FAILURE;
        }
        accumulate_last_collection(report, &after);
        if (gc_internal_allocation_count() == 0) {
            break;
        }
    }
    if (!QueryPerformanceCounter(&end)) {
        return EXIT_FAILURE;
    }
    report->after_intervals = gc_internal_allocation_count();
    report->collections =
        after.collection_count - before.collection_count;
    report->minor_collections =
        after.minor_collection_count - before.minor_collection_count;
    report->major_collections =
        after.major_collection_count - before.major_collection_count;
    report->tree_searches = after.tree_searches - before.tree_searches;
    report->tree_comparisons =
        after.tree_comparisons - before.tree_comparisons;
    report->last_tree_searches = after.last_tree_searches;
    report->last_tree_comparisons = after.last_tree_comparisons;
    report->pause_ticks = after.pause_ticks - before.pause_ticks;
    report->mark_ticks = after.mark_ticks - before.mark_ticks;
    report->sweep_ticks = after.sweep_ticks - before.sweep_ticks;
    report->last_pause_ticks = after.last_pause_ticks;
    report->last_minor_pause_ticks = after.last_minor_pause_ticks;
    report->last_major_pause_ticks = after.last_major_pause_ticks;
    report->last_mark_ticks = after.last_mark_ticks;
    report->last_sweep_ticks = after.last_sweep_ticks;
    report->frequency = after.performance_frequency;
    report->operation_ticks = (uint64_t)(end.QuadPart - start.QuadPart);
    if (report->frequency == 0) {
        report->frequency = before.performance_frequency;
    }
    return EXIT_SUCCESS;
}

static void print_cleanup_report(const CleanupReport *report)
{
    puts("\nLimpeza concluida:");
    printf("  intervalos antes:       %zu\n", report->before_intervals);
    printf("  intervalos restantes:   %zu\n", report->after_intervals);
    printf("  coletas executadas:     %zu\n", report->collections);
    printf("  coletas menores:        %zu\n", report->minor_collections);
    printf("  coletas maiores:        %zu\n", report->major_collections);
    printf("  objetos marcados:       %zu\n", report->marked_objects);
    printf("  objetos coletados:      %zu\n", report->collected_objects);
    puts("\nMetricas de tempo:");
    printf("  limpeza total:          %.3f ms\n",
           ticks_to_ms(report->operation_ticks, report->frequency));
    printf("  pausa acumulada:        %.3f ms\n",
           ticks_to_ms(report->pause_ticks, report->frequency));
    printf("  mark acumulado:         %.3f ms\n",
           ticks_to_ms(report->mark_ticks, report->frequency));
    printf("  sweep acumulado:        %.3f ms\n",
           ticks_to_ms(report->sweep_ticks, report->frequency));
    printf("  ultima pausa:           %.3f ms\n",
           ticks_to_ms(report->last_pause_ticks, report->frequency));
    printf("  ultima pausa menor:     %.3f ms\n",
           ticks_to_ms(report->last_minor_pause_ticks, report->frequency));
    printf("  ultima pausa maior:     %.3f ms\n",
           ticks_to_ms(report->last_major_pause_ticks, report->frequency));
    printf("  ultimo mark:            %.3f ms\n",
           ticks_to_ms(report->last_mark_ticks, report->frequency));
    printf("  ultimo sweep:           %.3f ms\n",
           ticks_to_ms(report->last_sweep_ticks, report->frequency));
    puts("\nMetricas da arvore de intervalos:");
    printf("  buscas acumuladas:      %zu\n", report->tree_searches);
    printf("  comparacoes acumuladas: %zu\n", report->tree_comparisons);
    printf("  buscas da ultima coleta: %zu\n", report->last_tree_searches);
    printf("  comp. da ultima coleta: %zu\n",
           report->last_tree_comparisons);
}

static int cleanup_scale(FireScenario *scenario)
{
    CleanupReport report;

    if (!scenario->active) {
        puts("Nao ha objetos gerados nesta escala para limpar.");
        return EXIT_FAILURE;
    }
    if (collect_cleanup_sequence(scenario, &report) != EXIT_SUCCESS) {
        printf("Falha durante a limpeza. status=%d\n",
               (int)gc_get_status());
        return EXIT_FAILURE;
    }
    print_cleanup_report(&report);
    return EXIT_SUCCESS;
}

static void render_scale_menu(size_t count)
{
    (void)system("cls");
    print_header(count);
    puts("[1] Alocar a quantidade de objetos da escala");
    puts("[2] Fazer a limpeza dos objetos gerados");
    puts("[0] Sair do visualizador");
    fputs("\nEscolha uma tecla: ", stdout);
    (void)fflush(stdout);
}

static int run_scale_menu(size_t count)
{
    FireScenario scenario;
    int exit_code = EXIT_SUCCESS;
    bool running = true;

    reset_scenario(&scenario);
    while (running) {
        int key;

        render_scale_menu(count);
        key = _getch();
        putchar('\n');
        if (key == '1') {
            if (allocate_scale(&scenario, count) != EXIT_SUCCESS) {
                exit_code = EXIT_FAILURE;
            }
            wait_key();
        } else if (key == '2') {
            if (cleanup_scale(&scenario) != EXIT_SUCCESS) {
                exit_code = EXIT_FAILURE;
            }
            wait_key();
        } else if (key == '0') {
            running = false;
        }
    }
    if (scenario.active) {
        (void)cleanup_scale(&scenario);
    }
    return exit_code;
}

static void render_main_menu(void)
{
    (void)system("cls");
    puts("=== Visualizador do fire_test com escalas ===\n");
    puts("[1] Escala 1.000 objetos");
    puts("[2] Escala 10.000 objetos");
    puts("[3] Escala 100.000 objetos");
    puts("[4] Escala 1.000.000 objetos");
    puts("[5] Objetivo 10^7 objetos");
    puts("[0] Sair");
    fputs("\nEscolha uma escala: ", stdout);
    (void)fflush(stdout);
}

static int run_demo(void)
{
    FireScenario scenario;
    int result = EXIT_SUCCESS;

    reset_scenario(&scenario);
    puts("Demo automatica: escala 1.000");
    if (allocate_scale(&scenario, fire_scales[0]) != EXIT_SUCCESS) {
        result = EXIT_FAILURE;
    }
    if (cleanup_scale(&scenario) != EXIT_SUCCESS) {
        result = EXIT_FAILURE;
    }
    return result;
}

int main(int argc, char **argv)
{
    bool demo = argc > 1 && strcmp(argv[1], "--demo") == 0;
    int result = EXIT_SUCCESS;
    bool running = true;

    if (gc_init() != GC_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (demo) {
        result = run_demo();
        gc_shutdown();
        return result;
    }
    while (running) {
        int key;

        render_main_menu();
        key = _getch();
        if (key >= '1' && key <= '5') {
            size_t index = (size_t)(key - '1');

            result = run_scale_menu(fire_scales[index]);
            running = false;
        } else if (key == '0') {
            running = false;
        }
    }
    gc_shutdown();
    puts("\nVisualizador do fire_test encerrado.");
    return result;
}
