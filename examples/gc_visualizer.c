#include "gc.h"

#include <conio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
enum { OBJECT_COUNT = 10, ROOT_COUNT = 2, EDGE_COUNT = 2 };
typedef struct Object {
    struct Object *edge[EDGE_COUNT];
} Object;

typedef struct {
    Object *object[OBJECT_COUNT];
    void *root[ROOT_COUNT];
    bool active[OBJECT_COUNT];
    bool valid;
} Demo;
typedef void (*Action)(Demo *, char *, size_t);
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
    puts("Legenda: [R] raiz  [V] alcancavel  [L] lixo aguardando coleta\n");
    fputs("Raizes: ", stdout);
    for (index = 0; index < ROOT_COUNT; ++index) {
        printf("R%zu->", index);
        print_target(demo, demo->root[index]);
        fputs(index + 1 == ROOT_COUNT ? "\n\n" : "  ", stdout);
    }
    for (index = 0; index < OBJECT_COUNT; ++index) {
        if (!demo->active[index]) {
            continue;
        }
        ++active;
        printf("O%02zu [%c] -> ", index,
               (demo->root[0] == demo->object[index]
                || demo->root[1] == demo->object[index]) ? 'R'
               : (reachable[index] ? 'V' : 'L'));
        print_target(demo, demo->object[index]->edge[0]);
        fputs(", ", stdout);
        print_target(demo, demo->object[index]->edge[1]);
        putchar('\n');
    }
    printf("\nAtivos=%zu | alcancaveis=%zu | lixo=%zu\n",
           active, marked, active - marked);
    printf("Coletas=%zu | examinados=%zu | coletados=%zu | pausa=%.3f us\n",
           stats.collection_count, stats.last_objects_examined,
           stats.last_objects_collected, pause_us);
    printf("Bytes vivos=%zu | coletados=%zu | reservados=%zu\n",
           stats.bytes_live, stats.bytes_collected, stats.bytes_reserved);
    printf("\nUltima operacao: %s\n\n", message);
    puts("[1] Alocar e integrar objeto aleatorio");
    puts("[2] Alterar raiz ou referencia aleatoria");
    puts("[3] Descartar raiz ou referencia aleatoria");
    puts("[4] Executar coleta mark-sweep");
    puts("[5] Gerar novo grafo aleatorio");
    puts("[6] Executar sequencia automatica");
    puts("[0] Sair");
    fputs("\nEscolha uma tecla: ", stdout);
    (void)fflush(stdout);
}
static void grow_random(Demo *demo, char *message, size_t size) {
    size_t slot = choose_object(demo, false);
    size_t source = choose_object(demo, true);
    Object *object;
    if (slot == OBJECT_COUNT) {
        (void)snprintf(message, size, "limite de objetos atingido");
        return;
    }
    object = gc_malloc(sizeof *object);
    if (object == NULL) {
        demo->valid = false;
        (void)snprintf(message, size, "gc_malloc falhou: status %d",
                       (int)gc_get_status());
        return;
    }
    object->edge[0] = NULL;
    object->edge[1] = NULL;
    demo->object[slot] = object;
    demo->active[slot] = true;
    if (source != OBJECT_COUNT && rand() % 2 == 0) {
        demo->object[source]->edge[(size_t)rand() % EDGE_COUNT] = object;
    }
    if (rand() % 3 == 0) {
        demo->root[(size_t)rand() % ROOT_COUNT] = object;
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
    } else {
        size_t edge = (size_t)rand() % EDGE_COUNT;
        demo->object[source]->edge[edge] = demo->object[target];
        (void)snprintf(message, size, "O%02zu aponta para O%02zu",
                       source, target);
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
                return;
            }
        }
    }
    (void)snprintf(message, size, "nenhuma referencia para descartar");
}
static void collect_now(Demo *demo, char *message, size_t size) {
    bool reachable[OBJECT_COUNT];
    size_t marked = find_reachable(demo, reachable);
    size_t collected = 0;
    size_t index;
    GCStats stats;
    gc_collect();
    if (gc_get_status() != GC_STATUS_OK
        || gc_get_stats(&stats) != GC_SUCCESS) {
        demo->valid = false;
        (void)snprintf(message, size, "coleta falhou: status %d",
                       (int)gc_get_status());
        return;
    }
    for (index = 0; index < OBJECT_COUNT; ++index) {
        if (demo->active[index] && !reachable[index]) {
            demo->active[index] = false;
            demo->object[index] = NULL;
            ++collected;
        }
    }
    demo->valid = stats.last_objects_examined == marked
                  && stats.last_objects_collected == collected;
    (void)snprintf(message, size, "mark=%zu, sweep=%zu%s", marked, collected,
                   demo->valid ? "" : " (DIVERGENCIA)");
}
static bool reset_demo(Demo *demo, char *message, size_t size) {
    size_t index;
    if (gc_is_initialized()) {
        gc_shutdown();
    }
    memset(demo, 0, sizeof *demo);
    demo->valid = true;
    if (gc_init() != GC_SUCCESS) {
        return false;
    }
    for (index = 0; index < ROOT_COUNT; ++index) {
        if (gc_add_root(&demo->root[index]) != GC_SUCCESS) {
            return false;
        }
    }
    for (index = 0; index < (size_t)7; ++index) {
        grow_random(demo, message, size);
    }
    for (index = 0; index < (size_t)8; ++index) {
        mutate_random(demo, message, size);
    }
    (void)snprintf(message, size, "novo grafo aleatorio criado");
    return demo->valid;
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
int main(int argc, char **argv) {
    static Action actions[] = {
        grow_random, mutate_random, discard_random, collect_now
    };
    Demo demo = {0};
    char message[128] = "pronto";
    int key;
    srand((unsigned int)time(NULL) ^ (unsigned int)GetCurrentProcessId());
    if (!reset_demo(&demo, message, sizeof message)) {
        return EXIT_FAILURE;
    }
    if (argc == 2 && strcmp(argv[1], "--demo") == 0) {
        int result = run_demo(&demo) ? EXIT_SUCCESS : EXIT_FAILURE;
        gc_shutdown();
        return result;
    }
    for (;;) {
        (void)system("cls");
        render(&demo, message);
        key = _getch();
        if (key == '0') {
            break;
        }
        if (key >= '1' && key <= '4') {
            actions[key - '1'](&demo, message, sizeof message);
        } else if (key == '5') {
            demo.valid = reset_demo(&demo, message, sizeof message);
        } else if (key == '6') {
            demo.valid = run_demo(&demo);
            fputs("Pressione uma tecla para voltar ao menu...", stdout);
            (void)_getch();
        }
    }
    gc_shutdown();
    puts("\nVisualizador encerrado.");
    return demo.valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
