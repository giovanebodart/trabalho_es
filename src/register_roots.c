#include "register_roots.h"

#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#if defined(__GNUC__)
#define GC_NOINLINE __attribute__((noinline))
#else
#define GC_NOINLINE
#endif

/*
Aqui são encontrados endereços provaveis em registradores da CPU
Qualquer valor provavel presente na arvore de intervalos é tratado como objeto vivo
Podem ocorrer falsos positivos, mas nunca a morte de um objeto vivo 
*/
static GCRegisterScanResult gc_register_roots_push(uintptr_t candidate,
                                                   IntervalNode *tree,
                                                   GCMarkQueue *queue)
{
    GCMarkQueueResult result;
    IntervalNode *interval = gc_mark_find_candidate(tree, candidate, queue);

    if (interval == NULL) {
        return GC_REGISTER_SCAN_OK;
    }

    result = gc_mark_queue_push(queue, (GCAllocation *)interval);
    if (result == GC_MARK_QUEUE_OUT_OF_MEMORY) {
        return GC_REGISTER_SCAN_OUT_OF_MEMORY;
    }
    if (result == GC_MARK_QUEUE_INVALID) {
        return GC_REGISTER_SCAN_INVALID;
    }
    return GC_REGISTER_SCAN_OK;
}

GCRegisterScanResult gc_register_roots_scan_region(const void *region,
                                                   size_t size,
                                                   IntervalNode *tree,
                                                   GCMarkQueue *queue)
{
    const unsigned char *bytes;
    size_t last_offset;
    size_t offset;

    if (region == NULL || tree == NULL || queue == NULL) {
        return GC_REGISTER_SCAN_INVALID;
    }
    if (size < sizeof(uintptr_t)) {
        return GC_REGISTER_SCAN_OK;
    }

    bytes = region;
    last_offset = size - sizeof(uintptr_t);
    for (offset = 0; offset <= last_offset; ++offset) {
        uintptr_t candidate;
        GCRegisterScanResult result;

        memcpy(&candidate, bytes + offset, sizeof candidate);
        result = gc_register_roots_push(candidate, tree, queue);
        if (result != GC_REGISTER_SCAN_OK) {
            return result;
        }
    }
    return GC_REGISTER_SCAN_OK;
}

GC_NOINLINE GCRegisterScanResult gc_register_roots_scan(IntervalNode *tree,
                                                        GCMarkQueue *queue)
{
    jmp_buf context;

    if (tree == NULL || queue == NULL) {
        return GC_REGISTER_SCAN_INVALID;
    }
    if (setjmp(context) != 0) {
        return GC_REGISTER_SCAN_INVALID;
    }

    return gc_register_roots_scan_region(&context, sizeof context,
                                         tree, queue);
}
