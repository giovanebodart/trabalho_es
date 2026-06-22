#include "roots.h"

#include <stdlib.h>

static GCRoot *gc_roots_find(GCRoot *roots, void **location)
{
    while (roots != NULL) {
        if (roots->location == location) {
            return roots;
        }
        roots = roots->next;
    }
    return NULL;
}

GCRootResult gc_roots_add(GCRoot **roots, void **location)
{
    GCRoot *root;

    if (roots == NULL || location == NULL) {
        return GC_ROOT_RESULT_NOT_FOUND;
    }
    if (gc_roots_find(*roots, location) != NULL) {
        return GC_ROOT_RESULT_DUPLICATE;
    }

    root = malloc(sizeof *root);
    if (root == NULL) {
        return GC_ROOT_RESULT_OUT_OF_MEMORY;
    }
    root->location = location;
    root->next = *roots;
    *roots = root;
    return GC_ROOT_RESULT_OK;
}

GCRootResult gc_roots_remove(GCRoot **roots, void **location)
{
    GCRoot **link;

    if (roots == NULL || location == NULL) {
        return GC_ROOT_RESULT_NOT_FOUND;
    }

    link = roots;
    while (*link != NULL && (*link)->location != location) {
        link = &(*link)->next;
    }
    if (*link == NULL) {
        return GC_ROOT_RESULT_NOT_FOUND;
    }

    {
        GCRoot *removed = *link;

        *link = removed->next;
        free(removed);
    }
    return GC_ROOT_RESULT_OK;
}

bool gc_roots_get_value(const GCRoot *roots, void **location,
                        void **value)
{
    if (value != NULL) {
        *value = NULL;
    }
    if (location == NULL || value == NULL) {
        return false;
    }

    while (roots != NULL) {
        if (roots->location == location) {
            *value = *roots->location;
            return true;
        }
        roots = roots->next;
    }
    return false;
}

void gc_roots_destroy_all(GCRoot *roots)
{
    while (roots != NULL) {
        GCRoot *next = roots->next;

        free(roots);
        roots = next;
    }
}
