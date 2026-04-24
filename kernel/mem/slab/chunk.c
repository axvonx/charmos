#include <math/bit_ops.h>
#include <math/div.h>
#include <math/ilog2.h>
#include <math/to_bits_bytes.h>
#include <mem/hhdm.h>
#include <mem/pmm.h>
#include <mem/vaddr_alloc.h>

#include "internal.h"

static inline vaddr_t vaddr_to_base_addr(vaddr_t vaddr) {
    return vaddr >> PAGE_4K_SHIFT;
}

static inline vaddr_t base_addr_to_vaddr(vaddr_t base_addr) {
    return base_addr << PAGE_4K_SHIFT;
}

static void validate_ptr_in_chunk(struct slab_chunk *chunk, void *ptr) {
    vaddr_t vaddr = (vaddr_t) ptr;
    vaddr_t min = base_addr_to_vaddr(chunk->base_addr);
    vaddr_t max = base_addr_to_vaddr(chunk->base_addr) + SLAB_CHUNK_SIZE;
    kassert(vaddr >= min && vaddr <= max);
}

static bool chunks_refill(struct slab_chunks *sc, enum irql *lirql) {
    spin_unlock(&sc->lock, *lirql);
    uintptr_t phys = pmm_alloc_page();
    if (!phys) {
        *lirql = spin_lock(&sc->lock);
        return false;
    }

    uintptr_t virt = hhdm_paddr_to_vaddr(phys);
    uint8_t *chunks_base = (uint8_t *) virt;
    size_t stride = sizeof(struct slab_chunk) + sc->bitmap_bytes;

    for (uint64_t i = 0; i < slab_chunks_per_page(sc); i++) {
        struct slab_chunk *chunk =
            (struct slab_chunk *) (chunks_base + i * stride);
        INIT_LIST_HEAD(&chunk->list);
        chunk->state = SLAB_CHUNK_FREE;
        chunk->base_addr = 0x0;
        memset(chunk->bitmap, 0, sc->bitmap_bytes);
    }

    *lirql = spin_lock(&sc->lock);
    for (uint64_t i = 0; i < slab_chunks_per_page(sc); i++) {
        struct slab_chunk *chunk =
            (struct slab_chunk *) (chunks_base + i * stride);
        list_add_tail(&chunk->list, &sc->freelist);
    }

    return true;
}

static void move_to(struct slab_chunks *sc, struct slab_chunk *c,
                    enum slab_chunk_state s) {
    kassert(c->state != s);
    list_del_init(&c->list);
    c->state = s;
    list_add_tail(&c->list, &sc->lists[s]);
    if (s == SLAB_CHUNK_FREE) {
        vaddr_t vaddr = base_addr_to_vaddr(c->base_addr);
        uint8_t curr = slab_order_map_get(vaddr);
        kassert(curr != SLAB_POW2_ORDER_EMPTY && curr != SLAB_POW2_ORDER_NONE);

        vas_free(slab_global.vas, vaddr, PAGE_2MB);
        slab_order_map_set(vaddr, SLAB_POW2_ORDER_EMPTY);
        c->base_addr = 0x0;
        memset(c->bitmap, 0, sc->bitmap_bytes);
    }
}

static struct slab_chunk *alloc_chunk(struct slab_chunks *sc,
                                      enum irql *lirql) {
    vaddr_t base = vas_alloc(slab_global.vas, PAGE_2MB, PAGE_2MB);

    if (!base)
        return NULL;

    uint8_t curr = slab_order_map_get(base);
    kassert(curr == SLAB_POW2_ORDER_EMPTY || curr == SLAB_POW2_ORDER_NONE);
    slab_order_map_set(base, sc->pow2_order);
    if (list_empty(&sc->freelist)) {
        if (!chunks_refill(sc, lirql)) {
            vas_free(slab_global.vas, base, PAGE_2MB);
            return NULL;
        }
    }

    struct list_head *pop = list_pop_front_init(&sc->freelist);
    struct slab_chunk *ret = container_of(pop, struct slab_chunk, list);
    kassert(!ret->base_addr);

    move_to(sc, ret, SLAB_CHUNK_PARTIAL);
    ret->base_addr = vaddr_to_base_addr(base);
    return ret;
}

static vaddr_t alloc_from(struct slab_chunks *chunks,
                          struct slab_chunk *chunk) {
    uint64_t *bm = (uint64_t *) chunk->bitmap;
    size_t nwords = DIV_ROUND_UP(chunks->bitmap_bytes * 8, 64);

    for (size_t w = 0; w < nwords; w++) {
        if (bm[w] != UINT64_MAX) {
            uint64_t free_bits = ~bm[w];
            uint64_t bit = __builtin_ctzll(free_bits);
            uint64_t i = w * 64 + bit;

            if (i >= chunks->bitmap_bytes * 8)
                break;

            SLAB_BITMAP_SET(bm[w], 1ULL << bit);
            chunk->used++;

            return base_addr_to_vaddr(chunk->base_addr) +
                   i * chunks->page_stride * PAGE_SIZE;
        }
    }

    kassert_unreachable();
}

static void free_to(struct slab_chunks *chunks, struct slab_chunk *chunk,
                    vaddr_t v) {
    validate_ptr_in_chunk(chunk, (void *) v);
    uint64_t offset = v - base_addr_to_vaddr(chunk->base_addr);
    kassert(offset % chunks->page_stride == 0);
    uint64_t i = offset / chunks->page_stride;

    uint64_t *bm = (uint64_t *) chunk->bitmap;
    size_t nwords = DIV_ROUND_UP(chunks->bitmap_bytes * 8, 64);
    kassert(i < nwords * 64);

    size_t w = i / 64;
    size_t b = i % 64;
    kassert(SLAB_BITMAP_TEST(bm[w], 1ULL << b));
    SLAB_BITMAP_UNSET(bm[w], 1ULL << b);
    chunk->used--;
}

static vaddr_t chunk_alloc(struct slab_chunks *chunks,
                           struct slab_chunk *chunk) {
    size_t max = chunks->bitmap_bytes * 8;
    vaddr_t ret = alloc_from(chunks, chunk);
    if (chunk->used == max)
        move_to(chunks, chunk, SLAB_CHUNK_USED);

    return ret;
}

static void chunk_free(struct slab_chunks *chunks, struct slab_chunk *chunk,
                       vaddr_t v) {
    enum slab_chunk_state last = chunk->state;
    kassert(last != SLAB_CHUNK_FREE);
    free_to(chunks, chunk, v);

    if (last == SLAB_CHUNK_USED) {
        move_to(chunks, chunk, SLAB_CHUNK_PARTIAL);
    } else if (chunk->used == 0) {
        move_to(chunks, chunk, SLAB_CHUNK_FREE);
    }
}

static struct slab_chunk *try_pop(struct list_head *lh) {
    struct list_head *pop = list_pop_front_init(lh);
    if (!pop)
        return NULL;

    return container_of(pop, struct slab_chunk, list);
}

vaddr_t slab_chunks_alloc(struct slab_chunks *sc, struct slab_chunk **out) {
    vaddr_t ret = 0x0;
    enum irql irql = spin_lock(&sc->lock);

    *out = try_pop(&sc->partial_list);
    if (!*out) {
        if (!(*out = alloc_chunk(sc, &irql)))
            goto out;
    }

    ret = chunk_alloc(sc, *out);

out:
    spin_unlock(&sc->lock, irql);
    return ret;
}

void slab_chunks_free(struct slab_chunks *sc, struct slab_chunk *chunk,
                      vaddr_t addr) {
    enum irql irql = spin_lock(&sc->lock);

    chunk_free(sc, chunk, addr);

    spin_unlock(&sc->lock, irql);
}

void slab_chunks_init(struct slab_chunks *sc, struct slab_cache *parent) {
    sc->parent = parent;
    sc->page_stride = next_pow2(parent->pages_per_slab);
    size_t page_count = 1 << (PAGE_2M_SHIFT - PAGE_4K_SHIFT);
    size_t bitmap_bits = DIV_ROUND_UP(page_count, sc->page_stride);
    sc->bitmap_bytes = to_bytes(bitmap_bits);
    for (size_t i = 0; i < SLAB_CHUNK_MAX; i++) {
        INIT_LIST_HEAD(&sc->lists[i]);
    }

    spinlock_init(&sc->lock);
    sc->pow2_order = ilog2(sc->page_stride);
}
