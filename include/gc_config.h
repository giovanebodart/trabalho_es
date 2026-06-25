#ifndef GC_CONFIG_H
#define GC_CONFIG_H

#include <stddef.h>

#define GC_DEFAULT_MEMORY_LIMIT ((size_t)64 * 1024u * 1024u)
#define GC_DEFAULT_PROMOTION_THRESHOLD ((size_t)2)
#define GC_DEFAULT_MAJOR_COLLECTION_INTERVAL ((size_t)4)

#endif
