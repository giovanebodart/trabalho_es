#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "old_pages.h"
#include "test.h"

static int test_old_pages_track_only_dedicated_old_mappings(void)
{
    GCAllocation *small;
    GCAllocation *large;
    GCOldPage *pages = NULL;
    const GCOldPage *page;
    SYSTEM_INFO system_info;
    size_t small_reserved;
    size_t large_reserved;
    size_t large_size;

    GetSystemInfo(&system_info);
    large_size = (size_t)system_info.dwPageSize + (size_t)1;
    TEST_ASSERT(gc_allocator_reservation_size(
        32, (size_t)system_info.dwPageSize, &small_reserved));
    TEST_ASSERT(gc_allocator_reservation_size(
        large_size, (size_t)system_info.dwPageSize, &large_reserved));
    small = gc_allocator_create(32, small_reserved);
    large = gc_allocator_create(large_size, large_reserved);
    TEST_ASSERT(small != NULL && large != NULL);

    small->generation = GC_GENERATION_OLD;
    large->generation = GC_GENERATION_YOUNG;
    small->next = large;
    TEST_ASSERT_EQ_INT(GC_OLD_PAGE_OK, gc_old_pages_rebuild(&pages, small));
    TEST_ASSERT(gc_old_pages_count(pages) == (size_t)0);
    TEST_ASSERT(gc_old_pages_find(pages, small->memory) == NULL);
    TEST_ASSERT(gc_old_pages_find(pages, large->memory) == NULL);

    large->generation = GC_GENERATION_OLD;
    TEST_ASSERT_EQ_INT(GC_OLD_PAGE_OK, gc_old_pages_rebuild(&pages, small));
    TEST_ASSERT(gc_old_pages_count(pages) == (size_t)1);
    page = gc_old_pages_find(pages, large->memory);
    TEST_ASSERT(page != NULL);
    TEST_ASSERT(page->base == large->mapping);
    TEST_ASSERT(page->size == large->reserved_size);
    TEST_ASSERT(gc_old_pages_find(pages, page) == NULL);
    TEST_ASSERT(gc_old_pages_find(
        pages, (unsigned char *)large->memory + large->requested_size - 1)
        == page);
    TEST_ASSERT(gc_old_pages_find(pages, small->memory) == NULL);

    gc_old_pages_destroy(pages);
    TEST_ASSERT(gc_allocator_destroy_one(small));
    TEST_ASSERT(gc_allocator_destroy_one(large));
    gc_allocator_destroy_all(NULL);
    return EXIT_SUCCESS;
}

int main(void)
{
    TEST_ASSERT_EQ_INT(GC_OLD_PAGE_INVALID,
                       gc_old_pages_rebuild(NULL, NULL));
    TEST_ASSERT(gc_old_pages_find(NULL, NULL) == NULL);
    TEST_ASSERT(gc_old_pages_count(NULL) == (size_t)0);
    gc_old_pages_destroy(NULL);
    TEST_ASSERT_EQ_INT(EXIT_SUCCESS,
                       test_old_pages_track_only_dedicated_old_mappings());
    puts("test_old_pages: ok");
    return EXIT_SUCCESS;
}
