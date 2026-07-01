#ifndef REGISTER_ROOTS_H
#define REGISTER_ROOTS_H

#include "marker.h"

#include <stddef.h>

typedef enum {
    GC_REGISTER_SCAN_OK = 0,
    GC_REGISTER_SCAN_INVALID,
    GC_REGISTER_SCAN_OUT_OF_MEMORY
} GCRegisterScanResult;

/*
 * The saved context is treated only as an opaque byte region. The collector
 * must not depend on the internal order, size or register mapping of jmp_buf.
 */
GCRegisterScanResult gc_register_roots_scan_region(const void *region,
                                                   size_t size,
                                                   IntervalNode *tree,
                                                   GCMarkQueue *queue);
GCRegisterScanResult gc_register_roots_scan(IntervalNode *tree,
                                            GCMarkQueue *queue);

#endif
