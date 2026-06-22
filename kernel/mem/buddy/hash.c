#include "internal.h"

/* hc->tail->next == head */

static inline size_t buddy_hash(struct buddy_page *bp) {
    uintptr_t baddr = (uintptr_t) bp;
    return hash_murmur3_32(&baddr, sizeof(uintptr_t), 0x9E3779B9) %
           BUDDY_FREE_AREA_HASH_TABLE_SIZE;
}

static inline void free_area_bitmap_set(uint64_t *bm, size_t idx) {
    size_t word_idx = idx / 64;
    size_t bit_idx = idx % 64;
    bm[word_idx] |= (1ULL << bit_idx);
}

static inline bool free_area_bitmap_clear(uint64_t *bm, size_t idx) {
    size_t word_idx = idx / 64;
    size_t bit_idx = idx % 64;
    bool was_set = (bm[word_idx] & (1ULL << bit_idx)) != 0;
    bm[word_idx] &= ~(1ULL << bit_idx);
    return was_set;
}

static inline bool free_area_bitmap_test(uint64_t *bm, size_t idx) {
    size_t word_idx = idx / 64;
    size_t bit_idx = idx % 64;
    return (bm[word_idx] & (1ULL << bit_idx)) != 0;
}

static inline size_t free_area_bitmap_ffs(uint64_t *bm) {
    for (size_t word_idx = 0; word_idx < BUDDY_FREE_AREA_BITMAP_SIZE;
         word_idx++) {
        if (bm[word_idx] != 0) {
            size_t bit_idx = __builtin_ctzll(bm[word_idx]);
            return word_idx * 64 + bit_idx;
        }
    }

    return SIZE_MAX;
}

static inline struct buddy_page *bhc_get_head(struct buddy_hash_chain *hc) {
    if (!hc->tail)
        return NULL;

    return buddy_page_get_next(hc->tail);
}

static inline bool bhc_empty(struct buddy_hash_chain *hc) {
    return hc->tail == NULL;
}

static inline void bhc_push_only_internal(struct buddy_hash_chain *hc,
                                          struct buddy_page *bp) {
    hc->tail = bp;
    buddy_page_set_next(bp, bp);
}

static void bhc_push_tail(struct buddy_hash_chain *hc, struct buddy_page *bp) {
    if (!hc->tail) {
        bhc_push_only_internal(hc, bp);
    } else {
        buddy_page_set_next(bp, buddy_page_get_next(hc->tail));
        buddy_page_set_next(hc->tail, bp);
        hc->tail = bp;
    }
}

static void bhc_push_head(struct buddy_hash_chain *hc, struct buddy_page *bp) {

    if (!hc->tail) {
        bhc_push_only_internal(hc, bp);
    } else {
        struct buddy_page *head = buddy_page_get_next(hc->tail);
        buddy_page_set_next(hc->tail, bp);
        buddy_page_set_next(bp, head);
    }
}

static inline struct buddy_page *
bhc_pop_only_internal(struct buddy_hash_chain *hc) {
    buddy_page_set_next(hc->tail, NULL);
    struct buddy_page *tail = hc->tail;
    hc->tail = NULL;
    return tail;
}

static struct buddy_page *bhc_pop_tail(struct buddy_hash_chain *hc) {
    if (!hc->tail)
        return NULL;

    if (hc->tail == buddy_page_get_next(hc->tail))
        return bhc_pop_only_internal(hc);

    struct buddy_page *head = buddy_page_get_next(hc->tail);
    struct buddy_page *tail_prev = NULL;
    while (buddy_page_get_next(tail_prev = buddy_page_get_next(head)) !=
           hc->tail)
        head = buddy_page_get_next(head);

    buddy_page_set_next(tail_prev, buddy_page_get_next(hc->tail));
    struct buddy_page *old_tail = hc->tail;
    hc->tail = tail_prev;
    buddy_page_set_next(old_tail, NULL);
    return old_tail;
}

static struct buddy_page *bhc_pop_head(struct buddy_hash_chain *hc) {
    if (!hc->tail)
        return NULL;

    if (hc->tail == buddy_page_get_next(hc->tail))
        return bhc_pop_only_internal(hc);

    struct buddy_page *head = buddy_page_get_next(hc->tail);
    buddy_page_set_next(hc->tail, buddy_page_get_next(head));
    buddy_page_set_next(head, NULL);
    return head;
}

static void bhc_remove(struct buddy_hash_chain *hc, struct buddy_page *bp) {
    kassert(hc->tail);
    if (hc->tail == buddy_page_get_next(hc->tail)) {
        kassert(hc->tail == bp);
        bhc_pop_only_internal(hc);
        return;
    }

    if (hc->tail == bp) {
        kassert(bhc_pop_tail(hc) == bp);
        return;
    }

    struct buddy_page *head = buddy_page_get_next(hc->tail);
    struct buddy_page *prev = NULL;
    while (buddy_page_get_next(prev = buddy_page_get_next(head)) != bp)
        head = buddy_page_get_next(head);

    kassert(buddy_page_get_next(prev) == bp);
    buddy_page_set_next(prev, buddy_page_get_next(bp));
    buddy_page_set_next(bp, NULL);
}

void buddy_hash_table_insert(struct buddy_hash_table *ht,
                             struct buddy_page *bp) {
    size_t idx = buddy_hash(bp);
    struct buddy_hash_chain *hc = &ht->chains[idx];
    bool zeroed = buddy_page_is_zeroed(bp);
    kassert(buddy_page_get_next_pfn(bp) == 0);

    if (zeroed) {
        free_area_bitmap_set(ht->zeroed_hc_bitmap, idx);
    } else {
        free_area_bitmap_set(ht->nonzero_hc_bitmap, idx);
    }

    if (zeroed) {
        bhc_push_head(hc, bp);
    } else {
        bhc_push_tail(hc, bp);
    }
}

static void buddy_free_area_clear_after_removal(struct buddy_hash_table *ht,
                                                struct buddy_hash_chain *hc,
                                                struct buddy_page *bp,
                                                size_t idx) {
    bool zeroed = buddy_page_is_zeroed(bp);
    if (bhc_empty(hc)) {
        if (zeroed) {
            kassert(free_area_bitmap_clear(ht->zeroed_hc_bitmap, idx));
        } else {
            kassert(free_area_bitmap_clear(ht->nonzero_hc_bitmap, idx));
        }
    } else if (zeroed) {
        if (!buddy_page_is_zeroed(bhc_get_head(hc))) {
            kassert(free_area_bitmap_clear(ht->zeroed_hc_bitmap, idx));
            kassert(free_area_bitmap_test(ht->nonzero_hc_bitmap, idx));
        }
    } else {
        if (buddy_page_is_zeroed(bhc_get_head(hc))) {
            kassert(free_area_bitmap_clear(ht->nonzero_hc_bitmap, idx));
            kassert(free_area_bitmap_test(ht->zeroed_hc_bitmap, idx));
        }
    }
}

void buddy_hash_table_remove(struct buddy_hash_table *ht,
                             struct buddy_page *bp) {
    kassert(buddy_page_get_next_pfn(bp));
    size_t idx = buddy_hash(bp);
    struct buddy_hash_chain *hc = &ht->chains[idx];
    bhc_remove(hc, bp);

    buddy_free_area_clear_after_removal(ht, hc, bp, idx);
}

struct buddy_page *buddy_hash_table_get_any(struct buddy_hash_table *ht) {
    size_t avail_z = free_area_bitmap_ffs(ht->zeroed_hc_bitmap);
    size_t avail_nz = free_area_bitmap_ffs(ht->nonzero_hc_bitmap);
    size_t avail = avail_nz;
    if (avail == SIZE_MAX)
        avail = avail_z;

    if (avail == SIZE_MAX)
        return NULL;

    struct buddy_hash_chain *hc = &ht->chains[avail];
    struct buddy_page *got = bhc_pop_tail(hc);
    buddy_free_area_clear_after_removal(ht, hc, got, avail);
    return got;
}

struct buddy_page *buddy_hash_table_get_zeroed(struct buddy_hash_table *ht) {
    size_t avail_z = free_area_bitmap_ffs(ht->zeroed_hc_bitmap);
    if (avail_z == SIZE_MAX)
        return NULL;

    struct buddy_hash_chain *hc = &ht->chains[avail_z];
    struct buddy_page *got = bhc_pop_head(hc);
    buddy_free_area_clear_after_removal(ht, hc, got, avail_z);
    return got;
}
