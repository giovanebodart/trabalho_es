#include "gc.h"

#include <conio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
enum {
    OBJECT_COUNT = 10,
    INITIAL_OBJECT_COUNT = 7,
    ROOT_COUNT = 2,
    EDGE_COUNT = 2,
    EVENT_COUNT = 9
};
#if defined(__GNUC__)
#define GC_VIS_NOINLINE __attribute__((noinline))
#else
#define GC_VIS_NOINLINE
#endif
typedef struct Object {
    struct Object *edge[EDGE_COUNT];
} Object;

typedef enum {
    SCENARIO_LIST = 0,
    SCENARIO_TREE,
    SCENARIO_CYCLIC
} ScenarioKind;

typedef struct {
    size_t step;
    char phase[12];
    char detail[96];
} Event;

typedef struct {
    Object *object[OBJECT_COUNT];
    void *root[ROOT_COUNT];
    bool active[OBJECT_COUNT];
    ScenarioKind scenario;
    Event events[EVENT_COUNT];
    size_t retained_garbage;
    size_t event_count;
    size_t step;
    bool valid;
} Demo;
typedef void (*Action)(Demo *, char *, size_t);
static const char *scenario_name(ScenarioKind scenario) {
    switch (scenario) {
    case SCENARIO_LIST:
        return "lista";
    case SCENARIO_TREE:
        return "arvore";
    case SCENARIO_CYCLIC:
        return "grafo ciclico";
    }
    return "desconhecido";
}
static GC_VIS_NOINLINE void scrub_stack_roots(void) {
    volatile uintptr_t noise[256];
    size_t index;
    for (index = 0; index < sizeof noise / sizeof noise[0]; ++index) {
        noise[index] = (uintptr_t)0;
    }
}
static size_t object_index(const Demo *demo, const void *pointer) {
    size_t index;
    for (index = 0; index < OBJECT_COUNT; ++index) {
        if (demo->active[index] && demo->object[index] == pointer) {
            return index;
        }
    }
    return OBJECT_COUNT;
}
static size_t choose_object(const Demo *demo, bool active) {
    size_t start = (size_t)rand() % OBJECT_COUNT;
    size_t offset;
    for (offset = 0; offset < OBJECT_COUNT; ++offset) {
        size_t index = (start + offset) % OBJECT_COUNT;
        if (demo->active[index] == active) {
            return index;
        }
    }
    return OBJECT_COUNT;
}
static void record_event(Demo *demo, const char *phase,
                         const char *format, ...) {
    Event *event;
    va_list args;
    if (demo->event_count < EVENT_COUNT) {
        event = &demo->events[demo->event_count++];
    } else {
        memmove(&demo->events[0], &demo->events[1],
                sizeof demo->events[0] * (EVENT_COUNT - 1));
        event = &demo->events[EVENT_COUNT - 1];
    }
    event->step = ++demo->step;
    (void)snprintf(event->phase, sizeof event->phase, "%s", phase);
    va_start(args, format);
    (void)vsnprintf(event->detail, sizeof event->detail, format, args);
    va_end(args);
}
static size_t find_reachable(const Demo *demo, bool reachable[OBJECT_COUNT]) {
    size_t queue[OBJECT_COUNT];
    size_t head = 0;
    size_t length = 0;
    size_t index;
    memset(reachable, 0, sizeof(bool) * OBJECT_COUNT);
    for (index = 0; index < ROOT_COUNT; ++index) {
        size_t object = object_index(demo, demo->root[index]);
        if (object != OBJECT_COUNT && !reachable[object]) {
            reachable[object] = true;
            queue[length++] = object;
        }
    }
    while (head < length) {
        Object *source = demo->object[queue[head++]];
        size_t edge;
        for (edge = 0; edge < EDGE_COUNT; ++edge) {
            size_t target = object_index(demo, source->edge[edge]);
            if (target != OBJECT_COUNT && !reachable[target]) {
                reachable[target] = true;
                queue[length++] = target;
            }
        }
    }
    return length;
}
static void print_target(const Demo *demo, const void *pointer) {
    size_t target = object_index(demo, pointer);
    if (target == OBJECT_COUNT) {
        fputs("--", stdout);
    } else {
        printf("O%02zu", target);
    }
}
static void render_events(const Demo *demo) {
    size_t index;
    puts("Eventos recentes do GC:");
    if (demo->event_count == 0) {
        puts("  nenhum evento registrado ainda");
        return;
    }
    for (index = 0; index < demo->event_count; ++index) {
        const Event *event = &demo->events[index];
        printf("  #%02zu %-7s %s\n",
               event->step, event->phase, event->detail);
    }
}
static void render(const Demo *demo, const char *message) {
    bool reachable[OBJECT_COUNT];
    GCStats stats = {0};
    size_t marked = find_reachable(demo, reachable);
    size_t active = 0;
    size_t index;
    double pause_us = 0.0;
    (void)gc_get_stats(&stats);
    if (stats.performance_frequency != 0) {
        pause_us = (double)stats.last_pause_ticks * 1000000.0
                   / (double)stats.performance_frequency;
    }
    puts("=== Visualizador do garbage collector mark-sweep ===");
    printf("Cenario: %s\n", scenario_name(demo->scenario));
    puts("Como ler:");
    puts("  [R] raiz direta: esta em uma variavel registrada no GC");
    puts("  [V] vivo: alcancavel por raiz ou por outro objeto vivo");
    puts("  [L] lixo: inacessivel e deve desaparecer apos uma coleta\n");
    fputs("Raizes: ", stdout);
    for (index = 0; index < ROOT_COUNT; ++index) {
        printf("R%zu->", index);
        print_target(demo, demo->root[index]);
        fputs(index + 1 == ROOT_COUNT ? "\n\n" : "  ", stdout);
    }
    puts("Objetos gerenciados:");
    puts("ID    estado   edge0  edge1");
    for (index = 0; index < OBJECT_COUNT; ++index) {
        const char *state;
        if (!demo->active[index]) {
            continue;
        }
        ++active;
        state = (demo->root[0] == demo->object[index]
                 || demo->root[1] == demo->object[index]) ? "[R]"
                : (reachable[index] ? "[V]" : "[L]");
        printf("O%02zu  %-7s ", index, state);
        print_target(demo, demo->object[index]->edge[0]);
        fputs("  ", stdout);
        print_target(demo, demo->object[index]->edge[1]);
        putchar('\n');
    }
    printf("\nAtivos=%zu | alcancaveis=%zu | lixo=%zu | retidos=%zu\n",
           active, marked, active - marked, demo->retained_garbage);
    printf("Coletas=%zu | examinados=%zu | coletados=%zu | pausa=%.3f us\n",
           stats.collection_count, stats.last_objects_examined,
           stats.last_objects_collected, pause_us);
    printf("Bytes vivos=%zu | coletados=%zu | reservados=%zu\n",
           stats.bytes_live, stats.bytes_collected, stats.bytes_reserved);
    putchar('\n');
    render_events(demo);
    printf("\nUltima operacao: %s\n\n", message);
    puts("[1] Alocar e integrar objeto aleatorio");
    puts("[2] Alterar raiz ou referencia aleatoria");
    puts("[3] Descartar raiz ou referencia aleatoria");
    puts("[4] Executar coleta menor");
    puts("[5] Gerar novo exemplo do cenario atual");
    puts("[6] Executar sequencia automatica");
    puts("[0] Sair");
    fputs("\nEscolha uma tecla: ", stdout);
    (void)fflush(stdout);
}
static Object *create_object(Demo *demo, size_t slot, char *message,
                             size_t size) {
    Object *object;
    if (slot >= OBJECT_COUNT || demo->active[slot]) {
        (void)snprintf(message, size, "slot invalido para alocacao");
        record_event(demo, "erro", "slot invalido para alocacao");
        return NULL;
    }
    object = gc_malloc(sizeof *object);
    if (object == NULL) {
        demo->valid = false;
        (void)snprintf(message, size, "gc_malloc falhou: status %d",
                       (int)gc_get_status());
        record_event(demo, "erro", "gc_malloc falhou com status %d",
                     (int)gc_get_status());
        return NULL;
    }
    object->edge[0] = NULL;
    object->edge[1] = NULL;
    demo->object[slot] = object;
    demo->active[slot] = true;
    record_event(demo, "alloc", "O%02zu criado por gc_malloc()", slot);
    return object;
}
static bool create_initial_objects(Demo *demo, char *message, size_t size) {
    size_t index;
    for (index = 0; index < INITIAL_OBJECT_COUNT; ++index) {
        if (create_object(demo, index, message, size) == NULL) {
            return false;
        }
    }
    return true;
}
static void build_list_scenario(Demo *demo) {
    size_t index;
    demo->root[0] = demo->object[0];
    demo->root[1] = demo->object[INITIAL_OBJECT_COUNT / (size_t)2];
    record_event(demo, "root", "R0 -> O00");
    record_event(demo, "root", "R1 -> O%02zu",
                 INITIAL_OBJECT_COUNT / (size_t)2);
    for (index = 0; index + (size_t)1 < INITIAL_OBJECT_COUNT; ++index) {
        demo->object[index]->edge[0] = demo->object[index + (size_t)1];
        record_event(demo, "link", "O%02zu.edge0 -> O%02zu",
                     index, index + (size_t)1);
    }
}
static void build_tree_scenario(Demo *demo) {
    size_t index;
    demo->root[0] = demo->object[0];
    demo->root[1] = demo->object[2];
    record_event(demo, "root", "R0 -> O00");
    record_event(demo, "root", "R1 -> O02");
    for (index = 0; index < INITIAL_OBJECT_COUNT; ++index) {
        size_t left = index * (size_t)2 + (size_t)1;
        size_t right = left + (size_t)1;
        if (left < INITIAL_OBJECT_COUNT) {
            demo->object[index]->edge[0] = demo->object[left];
            record_event(demo, "link", "O%02zu.edge0 -> O%02zu",
                         index, left);
        }
        if (right < INITIAL_OBJECT_COUNT) {
            demo->object[index]->edge[1] = demo->object[right];
            record_event(demo, "link", "O%02zu.edge1 -> O%02zu",
                         index, right);
        }
    }
}
static void build_cyclic_scenario(Demo *demo) {
    demo->root[0] = demo->object[0];
    demo->root[1] = demo->object[6];
    record_event(demo, "root", "R0 -> O00");
    record_event(demo, "root", "R1 -> O06");
    demo->object[0]->edge[0] = demo->object[1];
    demo->object[1]->edge[0] = demo->object[2];
    demo->object[2]->edge[0] = demo->object[0];
    demo->object[2]->edge[1] = demo->object[3];
    demo->object[3]->edge[0] = demo->object[4];
    demo->object[4]->edge[0] = demo->object[5];
    demo->object[5]->edge[0] = demo->object[3];
    demo->object[6]->edge[0] = demo->object[6];
    record_event(demo, "link", "O00.edge0 -> O01");
    record_event(demo, "link", "O01.edge0 -> O02");
    record_event(demo, "link", "O02.edge0 -> O00");
    record_event(demo, "link", "O02.edge1 -> O03");
    record_event(demo, "link", "O03.edge0 -> O04");
    record_event(demo, "link", "O04.edge0 -> O05");
    record_event(demo, "link", "O05.edge0 -> O03");
    record_event(demo, "link", "O06.edge0 -> O06");
}
static bool build_scenario(Demo *demo, char *message, size_t size) {
    if (!create_initial_objects(demo, message, size)) {
        return false;
    }
    if (demo->scenario == SCENARIO_LIST) {
        build_list_scenario(demo);
    } else if (demo->scenario == SCENARIO_TREE) {
        build_tree_scenario(demo);
    } else {
        build_cyclic_scenario(demo);
    }
    (void)snprintf(message, size, "exemplo de %s criado",
                   scenario_name(demo->scenario));
    record_event(demo, "reset", "exemplo de %s pronto para explorar",
                 scenario_name(demo->scenario));
    return true;
}
static void grow_random(Demo *demo, char *message, size_t size) {
    size_t slot = choose_object(demo, false);
    size_t source = choose_object(demo, true);
    Object *object;
    if (slot == OBJECT_COUNT) {
        (void)snprintf(message, size, "limite de objetos atingido");
        record_event(demo, "alloc", "falhou: todos os slots estao ativos");
        return;
    }
    object = create_object(demo, slot, message, size);
    if (object == NULL) {
        return;
    }
    if (source != OBJECT_COUNT && rand() % 2 == 0) {
        size_t edge = (size_t)rand() % EDGE_COUNT;
        demo->object[source]->edge[edge] = object;
        record_event(demo, "link", "O%02zu.edge%zu -> O%02zu",
                     source, edge, slot);
    }
    if (rand() % 3 == 0) {
        size_t root = (size_t)rand() % ROOT_COUNT;
        demo->root[root] = object;
        record_event(demo, "root", "R%zu -> O%02zu", root, slot);
    }
    (void)snprintf(message, size, "O%02zu alocado e integrado", slot);
}
static void mutate_random(Demo *demo, char *message, size_t size) {
    size_t source = choose_object(demo, true);
    size_t target = choose_object(demo, true);
    if (target == OBJECT_COUNT) {
        (void)snprintf(message, size, "nao ha objetos para alterar");
        return;
    }
    if (rand() % 2 == 0) {
        size_t root = (size_t)rand() % ROOT_COUNT;
        demo->root[root] = demo->object[target];
        (void)snprintf(message, size, "R%zu aponta para O%02zu", root, target);
        record_event(demo, "root", "R%zu -> O%02zu", root, target);
    } else {
        size_t edge = (size_t)rand() % EDGE_COUNT;
        demo->object[source]->edge[edge] = demo->object[target];
        (void)snprintf(message, size, "O%02zu aponta para O%02zu",
                       source, target);
        record_event(demo, "link", "O%02zu.edge%zu -> O%02zu",
                     source, edge, target);
    }
}
static void discard_random(Demo *demo, char *message, size_t size) {
    size_t total = ROOT_COUNT + OBJECT_COUNT * EDGE_COUNT;
    size_t start = (size_t)rand() % total;
    size_t offset;
    for (offset = 0; offset < total; ++offset) {
        size_t item = (start + offset) % total;
        if (item < ROOT_COUNT && demo->root[item] != NULL) {
            demo->root[item] = NULL;
            (void)snprintf(message, size, "R%zu foi descartada", item);
            record_event(demo, "drop", "R%zu -> --", item);
            return;
        }
        if (item >= ROOT_COUNT) {
            size_t edge_item = item - ROOT_COUNT;
            size_t object = edge_item / EDGE_COUNT;
            size_t edge = edge_item % EDGE_COUNT;
            if (demo->active[object]
                && demo->object[object]->edge[edge] != NULL) {
                demo->object[object]->edge[edge] = NULL;
                (void)snprintf(message, size, "aresta %zu de O%02zu descartada",
                               edge, object);
                record_event(demo, "drop", "O%02zu.edge%zu -> --",
                             object, edge);
                return;
            }
        }
    }
    (void)snprintf(message, size, "nenhuma referencia para descartar");
    record_event(demo, "drop", "nenhuma referencia disponivel");
}
static void collect_now(Demo *demo, char *message, size_t size) {
    bool reachable[OBJECT_COUNT];
    size_t marked = find_reachable(demo, reachable);
    size_t collected = 0;
    size_t collectible;
    size_t index;
    GCStats stats;
    scrub_stack_roots();
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || gc_get_stats(&stats) != GC_SUCCESS) {
        demo->valid = false;
        (void)snprintf(message, size, "coleta falhou: status %d",
                       (int)gc_get_status());
        record_event(demo, "erro", "gc_collect falhou com status %d",
                     (int)gc_get_status());
        return;
    }
    collectible = demo->retained_garbage;
    for (index = 0; index < OBJECT_COUNT; ++index) {
        if (demo->active[index] && !reachable[index]) {
            demo->active[index] = false;
            demo->object[index] = NULL;
            ++collectible;
        }
    }
    collected = stats.last_objects_collected;
    demo->valid = stats.last_objects_examined >= marked
                  && collected <= collectible;
    if (demo->valid) {
        demo->retained_garbage = collectible - collected;
    }
    (void)snprintf(message, size, "mark>=%zu, sweep=%zu, retidos=%zu%s",
                   marked, collected, demo->retained_garbage,
                   demo->valid ? "" : " (DIVERGENCIA)");
    record_event(demo, demo->valid ? "collect" : "erro",
                 "marcou >=%zu, liberou %zu, retidos %zu",
                 marked, collected, demo->retained_garbage);
}
static bool reset_demo(Demo *demo, ScenarioKind scenario, char *message,
                       size_t size) {
    size_t index;
    if (gc_is_initialized()) {
        gc_shutdown();
    }
    memset(demo, 0, sizeof *demo);
    demo->scenario = scenario;
    demo->valid = true;
    if (gc_init() != GC_SUCCESS) {
        return false;
    }
    for (index = 0; index < ROOT_COUNT; ++index) {
        if (gc_add_root(&demo->root[index]) != GC_SUCCESS) {
            return false;
        }
    }
    record_event(demo, "reset", "GC reiniciado e %d raizes registradas",
                 ROOT_COUNT);
    (void)index;
    return build_scenario(demo, message, size) && demo->valid;
}
static bool run_demo(Demo *demo) {
    static Action actions[] = {
        grow_random, mutate_random, discard_random, collect_now
    };
    char message[128] = "animacao iniciada";
    int step;
    for (step = 0; step < 14 && demo->valid; ++step) {
        actions[(size_t)rand() % (sizeof actions / sizeof actions[0])](
            demo, message, sizeof message);
    }
    collect_now(demo, message, sizeof message);
    render(demo, message);
    putchar('\n');
    return demo->valid;
}
static bool run_all_demos(Demo *demo, char *message, size_t size) {
    static const ScenarioKind scenarios[] = {
        SCENARIO_LIST, SCENARIO_TREE, SCENARIO_CYCLIC
    };
    size_t index;
    for (index = 0; index < sizeof scenarios / sizeof scenarios[0]; ++index) {
        if (!reset_demo(demo, scenarios[index], message, size)
            || !run_demo(demo)) {
            return false;
        }
    }
    return true;
}
static bool choose_initial_scenario(ScenarioKind *scenario) {
    int key;
    for (;;) {
        (void)system("cls");
        puts("=== Visualizador do garbage collector ===\n");
        puts("[1] Exemplo com lista");
        puts("[2] Exemplo com arvore");
        puts("[3] Exemplo com grafo ciclico");
        puts("[0] Sair do visualizador");
        fputs("\nEscolha um exemplo: ", stdout);
        (void)fflush(stdout);
        key = _getch();
        if (key == '0') {
            return false;
        }
        if (key == '1') {
            *scenario = SCENARIO_LIST;
            return true;
        }
        if (key == '2') {
            *scenario = SCENARIO_TREE;
            return true;
        }
        if (key == '3') {
            *scenario = SCENARIO_CYCLIC;
            return true;
        }
    }
}
int main(int argc, char **argv) {
    static Action actions[] = {
        grow_random, mutate_random, discard_random, collect_now
    };
    ScenarioKind scenario = SCENARIO_CYCLIC;
    Demo *demo;
    char message[128] = "pronto";
    int key;
    srand((unsigned int)time(NULL) ^ (unsigned int)GetCurrentProcessId());
    demo = calloc((size_t)1, sizeof *demo);
    if (demo == NULL) {
        return EXIT_FAILURE;
    }
    if (argc == 2 && strcmp(argv[1], "--demo") == 0) {
        int result = run_all_demos(demo, message, sizeof message)
                     ? EXIT_SUCCESS
                     : EXIT_FAILURE;
        gc_shutdown();
        free(demo);
        return result;
    }
    if (!choose_initial_scenario(&scenario)) {
        free(demo);
        return EXIT_SUCCESS;
    }
    if (!reset_demo(demo, scenario, message, sizeof message)) {
        free(demo);
        return EXIT_FAILURE;
    }
    for (;;) {
        (void)system("cls");
        render(demo, message);
        key = _getch();
        if (key == '0') {
            break;
        }
        if (key >= '1' && key <= '4') {
            actions[key - '1'](demo, message, sizeof message);
        } else if (key == '5') {
            scenario = demo->scenario;
            demo->valid = reset_demo(demo, scenario, message,
                                     sizeof message);
        } else if (key == '6') {
            demo->valid = run_demo(demo);
            fputs("Pressione uma tecla para voltar ao menu...", stdout);
            (void)_getch();
        }
    }
    gc_shutdown();
    puts("\nVisualizador encerrado.");
    key = demo->valid ? EXIT_SUCCESS : EXIT_FAILURE;
    free(demo);
    return key;
}
