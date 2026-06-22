#pragma once
#include <compiler.h>
#include <math/min_max.h>
#include <mem/bitmap.h>
#include <mem/buddy.h>
#include <mem/page.h>
#include <stdbool.h>
#include <stdint.h>
#include <sync/spinlock.h>
#include <types/types.h>

#define PAGE_TO_BUDDY_PAGE(p) ((struct buddy_page *) (p))
#define BUDDY_PAGE_TO_PAGE(p) ((struct page *) (p))

/*
 * Layout of buddy_page::meta
 *   bits [0,  2]  : page tag (PAGE_TAG_*)
 *   bit  [3]      : is_free
 *   bit  [4]      : is_zeroed
 *   bits [5, 11]  : order (7 bits)
 *   bits [12, 63] : next_pfn (52 bits)
 */
#define BUDDY_IS_FREE_SHIFT 3
#define BUDDY_IS_FREE_MASK (1ULL << BUDDY_IS_FREE_SHIFT)

#define BUDDY_IS_ZEROED_SHIFT 4
#define BUDDY_IS_ZEROED_MASK (1ULL << BUDDY_IS_ZEROED_SHIFT)

#define BUDDY_ORDER_SHIFT 5
#define BUDDY_ORDER_BITS 7
#define BUDDY_ORDER_MASK (((1ULL << BUDDY_ORDER_BITS) - 1) << BUDDY_ORDER_SHIFT)

#define BUDDY_NEXT_PFN_SHIFT PAGE_4K_SHIFT
#define BUDDY_NEXT_PFN_BITS (64 - PAGE_4K_SHIFT)
#define BUDDY_NEXT_PFN_MASK                                                    \
    (((1ULL << BUDDY_NEXT_PFN_BITS) - 1) << BUDDY_NEXT_PFN_SHIFT)

struct buddy_page {
    uint64_t meta;
};
static_assert_struct_size_eq(buddy_page, 8);

static inline uint64_t buddy_page_get_order(const struct buddy_page *bp) {
    return (bp->meta & BUDDY_ORDER_MASK) >> BUDDY_ORDER_SHIFT;
}

static inline void buddy_page_set_order(struct buddy_page *bp, uint64_t order) {
    bp->meta = (bp->meta & ~BUDDY_ORDER_MASK) |
               ((order << BUDDY_ORDER_SHIFT) & BUDDY_ORDER_MASK);
}

static inline pfn_t buddy_page_get_next_pfn(const struct buddy_page *bp) {
    return (pfn_t) ((bp->meta & BUDDY_NEXT_PFN_MASK) >> BUDDY_NEXT_PFN_SHIFT);
}

static inline void buddy_page_set_next_pfn(struct buddy_page *bp, pfn_t pfn) {
    bp->meta = (bp->meta & ~BUDDY_NEXT_PFN_MASK) |
               (((uint64_t) pfn << BUDDY_NEXT_PFN_SHIFT) & BUDDY_NEXT_PFN_MASK);
}

static inline bool buddy_page_is_free(const struct buddy_page *bp) {
    return (bp->meta & BUDDY_IS_FREE_MASK) != 0;
}

static inline void buddy_page_set_free(struct buddy_page *bp, bool is_free) {
    bp->meta =
        (bp->meta & ~BUDDY_IS_FREE_MASK) | (is_free ? BUDDY_IS_FREE_MASK : 0);
}

static inline bool buddy_page_is_zeroed(const struct buddy_page *bp) {
    return (bp->meta & BUDDY_IS_ZEROED_MASK) != 0;
}

static inline void buddy_page_set_zeroed(struct buddy_page *bp,
                                         bool is_zeroed) {
    bp->meta = (bp->meta & ~BUDDY_IS_ZEROED_MASK) |
               (is_zeroed ? BUDDY_IS_ZEROED_MASK : 0);
}

static inline bool page_pfn_allocated_in_boot_bitmap(pfn_t pfn) {
    return test_bit(pfn);
}

static inline bool buddy_page_pfn_free(pfn_t pfn) {
    if (pfn >= global.last_pfn)
        return false;

    struct page *p = &global.page_array[pfn];

    return buddy_page_is_free((struct buddy_page *) p);
}

static inline struct buddy_page *buddy_page_for_pfn(pfn_t pfn) {
    return PAGE_TO_BUDDY_PAGE(page_for_pfn(pfn));
}

static inline pfn_t buddy_page_get_pfn(struct buddy_page *bp) {
    return page_get_pfn((struct page *) bp);
}

static inline paddr_t buddy_page_get_paddr(struct buddy_page *bp) {
    return PFN_TO_PAGE(buddy_page_get_pfn(bp));
}

static inline void buddy_page_set_next(struct buddy_page *bp,
                                       struct buddy_page *next) {
    if (!next)
        return buddy_page_set_next_pfn(bp, 0);

    buddy_page_set_next_pfn(bp, buddy_page_get_pfn(next));
}

static inline struct buddy_page *buddy_page_get_next(struct buddy_page *bp) {
    pfn_t pfn = buddy_page_get_next_pfn(bp);
    if (pfn == 0)
        return NULL;

    return buddy_page_for_pfn(pfn);
}

struct buddy_free_link {
    struct buddy_page *prev;
};

static inline struct buddy_free_link *
buddy_page_free_link(struct buddy_page *bp) {
    return (struct buddy_free_link *) hhdm_paddr_to_ptr(
        buddy_page_get_paddr(bp));
}

static inline struct buddy_page *buddy_page_get_prev(struct buddy_page *bp) {
    return buddy_page_free_link(bp)->prev;
}

static inline void buddy_page_set_prev(struct buddy_page *bp,
                                       struct buddy_page *prev) {
    buddy_page_free_link(bp)->prev = prev;
}

static inline void buddy_page_tag(struct buddy_page *page) {
    if (page)
        page_set_tag(BUDDY_PAGE_TO_PAGE(page), PAGE_TAG_BUDDY);
}

static inline void buddy_page_untag(struct buddy_page *page) {
    if (page)
        page_set_tag(BUDDY_PAGE_TO_PAGE(page), PAGE_TAG_NONE);
}

static inline void buddy_page_assert_tag(struct buddy_page *page,
                                         enum page_tag tag) {
    if (page)
        page_assert_tag(BUDDY_PAGE_TO_PAGE(page), tag);
}

void buddy_remove_specific(struct buddy_free_area *area,
                           struct buddy_page *page);
