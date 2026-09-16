#include <mem/page.h>
#include <test/static_assert.h>

static_assert_size(struct page, 8);
static_assert_size(struct page_table, 4096);
static_assert(PAGE_SIZE == 4096, "PAGE_SIZE must be 4096");
static_assert(PAGE_4K_SHIFT == 12, "PAGE_4K_SHIFT must be 12");
