#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "interval_tree.h"
#include <stdbool.h>
#include <stddef.h>
typedef enum {
    GC_GENERATION_YOUNG = 0,
    GC_GENERATION_OLD = 1
} GCGeneration;

/* 
Representa os metadados do objeto alocado, utilizado pelo GC para gerenciar o objeto 
*/
typedef struct GCAllocation {
    IntervalNode interval; //Nó (intervalo) a qual o objeto referenciado pertence 
    void *mapping; //Area total alocada, é diferente pq pode conter canários para proteção
    void *memory; //Area util alocada
    size_t requested_size; //Bytes pedidos durante a alocação
    size_t reserved_size; //Bytes que o GC reservou, é diferente pq pode conter canários para proteção
    size_t survival_count; 
    GCGeneration generation;
    /*
    true: O objeto veio de alocação dedicada com VirtualAlloc()
    false: O obejto veio de uma arena, quando coletado o bloco a qual faz parte volta para uma lista 
    */ 
    bool dedicated_mapping; 
    bool marked; //Usado no GC na fase mark-sweep, indica se o objeto foi alcançado durante o mark
    struct GCAllocation *next; 
} GCAllocation;
bool gc_allocator_reservation_size(size_t requested, size_t page_size,
                                   size_t *reserved);
GCAllocation *gc_allocator_create(size_t requested, size_t reserved);
bool gc_allocator_validate_canaries(const GCAllocation *allocation);
bool gc_allocator_validate_all(const GCAllocation *allocation);
bool gc_allocator_corrupt_canary(GCAllocation *allocation,
                                 bool after_object);
void gc_allocator_record_survival(GCAllocation *allocation,
                                  size_t promotion_threshold);
bool gc_allocator_destroy_one(GCAllocation *allocation);
void gc_allocator_destroy_all(GCAllocation *allocation);
#endif
