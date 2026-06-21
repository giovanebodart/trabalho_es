#include "interval_tree.h"

#include <conio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

enum { SLOT_COUNT = 23, SLOT_WIDTH = 32, INITIAL_NODES = 9 };

typedef struct {
    IntervalNode nodes[SLOT_COUNT];
    bool active[SLOT_COUNT];
    IntervalNode *root;
    size_t count;
} Visualizer;

static size_t choose_slot(const Visualizer *visualizer, bool active)
{
    size_t start = (size_t)rand() % SLOT_COUNT;
    size_t offset;

    for (offset = 0; offset < SLOT_COUNT; ++offset) {
        size_t index = (start + offset) % SLOT_COUNT;

        if (visualizer->active[index] == active) {
            return index;
        }
    }
    return SLOT_COUNT;
}

static void print_tree(const IntervalNode *node, int depth, char branch)
{
    if (node == NULL) {
        return;
    }

    print_tree(node->right, depth + 1, '/');
    printf("%*s%c-- [%" PRIuPTR ", %" PRIuPTR
           ") h=%d bf=%d max=%" PRIuPTR "\n",
           depth * 4, "", branch, node->start, node->end, node->height,
           interval_node_balance_factor(node), node->max_end);
    print_tree(node->left, depth + 1, '\\');
}

static bool tree_is_valid(const Visualizer *visualizer)
{
    return interval_tree_validate(visualizer->root);
}

static void render(const Visualizer *visualizer, const char *message)
{
    puts("=== Visualizador da arvore AVL de intervalos ===");
    printf("Nos: %zu/%d | Altura: %d | Invariantes: %s\n\n",
           visualizer->count, SLOT_COUNT,
           interval_node_height(visualizer->root),
           tree_is_valid(visualizer) ? "OK" : "FALHA");

    if (visualizer->root == NULL) {
        puts("(arvore vazia)");
    } else {
        print_tree(visualizer->root, 0, '*');
    }

    printf("\nUltima operacao: %s\n\n", message);
    puts("[1] Inserir intervalo aleatorio");
    puts("[2] Remover intervalo aleatorio");
    puts("[3] Buscar endereco aleatorio");
    puts("[4] Gerar nova arvore aleatoria");
    puts("[5] Executar animacao de operacoes");
    puts("[6] Validar invariantes");
    puts("[0] Sair");
    fputs("\nEscolha uma tecla: ", stdout);
    (void)fflush(stdout);
}

static void reset_tree(Visualizer *visualizer)
{
    visualizer->root = NULL;
    visualizer->count = 0;
    memset(visualizer->active, 0, sizeof visualizer->active);
}

static void insert_random(Visualizer *visualizer,
                          char *message,
                          size_t message_size)
{
    size_t slot = choose_slot(visualizer, false);
    uintptr_t base;
    uintptr_t start;
    uintptr_t end;

    if (slot == SLOT_COUNT) {
        (void)snprintf(message, message_size, "arvore cheia");
        return;
    }

    base = (uintptr_t)(slot * SLOT_WIDTH);
    start = base + (uintptr_t)(rand() % 8);
    end = start + (uintptr_t)(8 + rand() % 16);
    if (!interval_node_init(&visualizer->nodes[slot], start, end)
        || !interval_tree_insert(&visualizer->root,
                                 &visualizer->nodes[slot])) {
        (void)snprintf(message, message_size, "insercao rejeitada");
        return;
    }

    visualizer->active[slot] = true;
    ++visualizer->count;
    (void)snprintf(message, message_size,
                   "inserido [%" PRIuPTR ", %" PRIuPTR ")",
                   start, end);
}

static void remove_random(Visualizer *visualizer,
                          char *message,
                          size_t message_size)
{
    size_t slot = choose_slot(visualizer, true);
    IntervalNode *removed = NULL;

    if (slot == SLOT_COUNT) {
        (void)snprintf(message, message_size, "arvore vazia");
        return;
    }

    if (!interval_tree_remove(&visualizer->root,
                              visualizer->nodes[slot].start,
                              &removed)
        || removed != &visualizer->nodes[slot]) {
        (void)snprintf(message, message_size, "remocao falhou");
        return;
    }

    visualizer->active[slot] = false;
    --visualizer->count;
    (void)snprintf(message, message_size,
                   "removido [%" PRIuPTR ", %" PRIuPTR ")",
                   removed->start, removed->end);
}

static void find_random(Visualizer *visualizer,
                        char *message,
                        size_t message_size)
{
    uintptr_t address = (uintptr_t)(rand() % (SLOT_COUNT * SLOT_WIDTH));
    size_t comparisons = 0;
    IntervalNode *found = interval_tree_find_counted(
        visualizer->root, address, &comparisons);

    if (found == NULL) {
        (void)snprintf(message, message_size,
                       "endereco %" PRIuPTR " ausente (%zu comparacoes)",
                       address, comparisons);
    } else {
        (void)snprintf(message, message_size,
                       "%" PRIuPTR " pertence a [%" PRIuPTR ", %" PRIuPTR
                       ") (%zu comparacoes)",
                       address, found->start, found->end, comparisons);
    }
}

static void populate(Visualizer *visualizer,
                     char *message,
                     size_t message_size)
{
    int index;

    reset_tree(visualizer);
    for (index = 0; index < INITIAL_NODES; ++index) {
        insert_random(visualizer, message, message_size);
    }
    (void)snprintf(message, message_size,
                   "nova arvore com %d intervalos", INITIAL_NODES);
}

static void random_operation(Visualizer *visualizer,
                             char *message,
                             size_t message_size)
{
    int operation = rand() % 3;

    if (operation == 0) {
        insert_random(visualizer, message, message_size);
    } else if (operation == 1) {
        remove_random(visualizer, message, message_size);
    } else {
        find_random(visualizer, message, message_size);
    }
}

static int run_automatic_demo(Visualizer *visualizer, bool animate)
{
    char message[160] = "inicio";
    int step;

    for (step = 0; step < 12; ++step) {
        random_operation(visualizer, message, sizeof message);
        if (animate) {
            (void)system("cls");
            render(visualizer, message);
            Sleep(450);
        }
        if (!tree_is_valid(visualizer)) {
            return EXIT_FAILURE;
        }
    }
    if (!animate) {
        render(visualizer, message);
        putchar('\n');
    }
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    Visualizer visualizer = {0};
    char message[160] = "pronto";
    int key;

    srand((unsigned int)time(NULL) ^ (unsigned int)GetCurrentProcessId());
    populate(&visualizer, message, sizeof message);

    if (argc == 2 && strcmp(argv[1], "--demo") == 0) {
        return run_automatic_demo(&visualizer, false);
    }

    for (;;) {
        (void)system("cls");
        render(&visualizer, message);
        key = _getch();
        if (key == '0') {
            break;
        }
        if (key == '1') {
            insert_random(&visualizer, message, sizeof message);
        } else if (key == '2') {
            remove_random(&visualizer, message, sizeof message);
        } else if (key == '3') {
            find_random(&visualizer, message, sizeof message);
        } else if (key == '4') {
            populate(&visualizer, message, sizeof message);
        } else if (key == '5') {
            if (run_automatic_demo(&visualizer, true) != EXIT_SUCCESS) {
                (void)snprintf(message, sizeof message,
                               "invariantes violadas durante a animacao");
            }
        } else if (key == '6') {
            (void)snprintf(message, sizeof message, "invariantes: %s",
                           tree_is_valid(&visualizer) ? "OK" : "FALHA");
        }
    }

    puts("\nVisualizador encerrado.");
    return EXIT_SUCCESS;
}
