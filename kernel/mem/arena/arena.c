#include <math/fixed.h>
#include <math/hash.h>
#include <math/sort.h>
#include <mem/alloc.h>
#include <mem/arena_provider.h>
#include <structures/list_rcu.h>
#include <sync/rcu.h>

#include "internal.h"

struct arena_globals arena_global = {0};
enum err arena_budget_prio_scale(sz_b_t used, int scale,
                                 ALLOC_PRIORITY_BITMAP_DECLARE(prio_map_out)) {
    fx32_32_t y = fx_pow_i32(FX_E, (int) used + scale);
    if (y > fx_from_int(ALLOC_PRIORITY_MAX) || y < FX_ZERO)
        return ERR_OVERFLOW;

    bitmap_zero(prio_map_out, ALLOC_PRIORITY_MAX);
    uint16_t base = (uint16_t) fx_to_int(y);
    for (uint16_t i = base; i < ALLOC_PRIORITY_MAX; i++)
        bitmap_set(prio_map_out, i);

    return ERR_OK;
}

static void arena_desc_table_free_cb(struct rcu_cb *cb) {
    struct arena_desc_table *tbl =
        container_of(cb, struct arena_desc_table, rcu_cb);
    kfree(tbl->descs);
    kfree(tbl);
}

static int32_t arena_desc_cmp(const void *a, const void *b) {
    const struct arena_desc *ada = a;
    const struct arena_desc *adb = b;
    return ada->strategy - adb->strategy;
}

enum err arena_desc_register(struct arena_desc *desc) {
    enum err err = ERR_OK;

    enum irql irql = spin_lock(&arena_global.tbl_lock);

    struct arena_desc_table *old_tbl = rcu_dereference(arena_global.desc_table);
    kassert(old_tbl);

    size_t old_n_descs = old_tbl->n_descs;
    size_t n_descs = old_n_descs + 1;
    struct arena_desc_table *new_tbl =
        kmalloc(sizeof(struct arena_desc_table), ALLOC_ZERO);
    if (!new_tbl) {
        err = ERR_NO_MEM;
        goto out;
    }

    struct arena_desc **new_descs =
        kmalloc(sizeof(struct arena_desc *) * n_descs, ALLOC_ZERO);
    if (!new_descs) {
        err = ERR_NO_MEM;
        kfree(new_tbl);
        goto out;
    }

    if (old_n_descs && old_tbl->descs)
        memcpy(new_descs, old_tbl->descs,
               old_n_descs * sizeof(struct arena_desc *));

    new_descs[old_n_descs] = desc;
    new_tbl->n_descs = n_descs;
    heapsort(new_descs, sizeof(struct arena_desc *), n_descs, arena_desc_cmp);

    rcu_assign_pointer(arena_global.desc_table, new_tbl);
    rcu_defer(&old_tbl->rcu_cb, arena_desc_table_free_cb);

out:
    spin_unlock(&arena_global.tbl_lock, irql);
    return err;
}

struct arena_desc *arena_desc_for(enum arena_strategy strat) {
    rcu_read_lock();

    struct arena_desc_table *tbl = rcu_dereference(arena_global.desc_table);

    struct arena_desc *found = bsearch(&strat, tbl, sizeof(struct arena_desc *),
                                       tbl->n_descs, arena_desc_cmp);

    rcu_read_unlock();

    return found;
}

void arena_global_init(void) {
    spinlock_init(&arena_global.tbl_lock);
    struct arena_desc_table *tbl =
        kmalloc(sizeof(struct arena_desc_table), ALLOC_ZERO);

    rcu_assign_pointer(arena_global.desc_table, tbl);
}

static uint16_t size_for_id(struct arena_seg_desc *seg_descs, size_t n_segs,
                            uint16_t id) {
    for (size_t i = 0; i < n_segs; i++) {
        if (seg_descs[i].id == id)
            return seg_descs[i].size;
    }

    unreachable("invalid id");
}

/* Errors panic because the provider calls this */
struct arena *arena_create_full(struct arena_seg_desc *seg_descs,
                                size_t n_segs) {
    size_t seg_data_size_total = 0;
    struct arena_seg_desc *large_desc = NULL;
    BITMAP_DECLARE(seen_id_bitmap, ARENA_MAX_SEG) = {0};
    for (size_t i = 0; i < n_segs; i++) {
        kassert(seg_descs[i].size);

        /* It's fine if a large segment is smaller than ARENA_SEG_MAX_SIZE */
        if (seg_descs[i].large) {
            kassert(!large_desc, "more than one large segment exists");
            large_desc = &seg_descs[i];
        }

        if (seg_descs[i].size >= ARENA_SEG_MAX_SIZE)
            kassert(seg_descs[i].large, "large segment id %u must set .large",
                    seg_descs[i].id);

        kassert(!bitmap_test_and_set(seen_id_bitmap, seg_descs[i].id),
                "duplicate id %hu", seg_descs[i].id);

        seg_data_size_total += ALIGN_UP(seg_descs[i].size, ARENA_SEG_ALIGN);
    }

    size_t inmem_size_raw = n_segs * sizeof(struct arena_seg_inmem_desc);
    size_t inmem_size = ALIGN_UP(inmem_size_raw, ARENA_SEG_ALIGN);
    size_t seg_size_total = inmem_size + seg_data_size_total;
    size_t size_total = sizeof(struct arena) + seg_size_total;

    struct arena *arena = kmalloc(size_total, ALLOC_ZERO);
    if (!arena)
        return NULL;

#ifdef DEBUG_ARENA
    arena->n_segs = n_segs;
    arena->total_size = size_total;
#endif

    struct arena_seg_inmem_desc *inmem_descs = arena_get_inmem_descs(arena);

    if (large_desc) {
        inmem_descs[n_segs - 1].id = large_desc->id;

#ifdef DEBUG_ARENA
        inmem_descs[n_segs - 1].present = true;
        inmem_descs[n_segs - 1].type = large_desc->type;
        inmem_descs[n_segs - 1].size = large_desc->size;
#endif
    }

    size_t cursor = 0;
    for (size_t i = 0; i < n_segs; i++) {
        if (!seg_descs[i].large) {

#ifdef DEBUG_ARENA
            inmem_descs[cursor].present = true;
            inmem_descs[cursor].size = seg_descs[i].size;
            inmem_descs[cursor].type = seg_descs[i].type;
#endif

            inmem_descs[cursor].id = seg_descs[i].id;
            cursor++;
        }
    }

    size_t cursor_offset = 0;
    for (uint16_t i = 0; i < n_segs; i++) {
        uint16_t size;

        if (i == 0) {
            size = inmem_size;
        } else {
            uint16_t id = inmem_descs[i - 1].id;
            size =
                ALIGN_UP(size_for_id(seg_descs, n_segs, id), ARENA_SEG_ALIGN);
        }

        cursor_offset += size;
#ifdef DEBUG_ARENA
        inmem_descs[i].seg =
            (struct arena_seg *) &arena->payload[cursor_offset];
#endif

        inmem_descs[i].offset_bump = size;
    }

    arena_check_assert(arena);
    return arena;
}

/* We very much expect that seg_id is valid. If it isn't, UB is possible
 * as this lookup could read out of bounds. */
struct arena_seg *arena_seg_lookup(struct arena *a, uint16_t seg_id) {
    struct arena_seg_inmem_desc *inmem_descs = arena_get_inmem_descs(a);
#ifdef DEBUG_ARENA
    bool found = false;
    for (uint16_t i = 0; i < a->n_segs; i++) {
        if (inmem_descs[i].id == seg_id)
            found = true;
    }

    kassert(found, "segment %u not found", seg_id);
#endif

    size_t cursor = 0;
    for (int i = 0; i < ARENA_MAX_SEG; i++) {
        cursor += inmem_descs[i].offset_bump;
        if (inmem_descs[i].id == seg_id)
            return (struct arena_seg *) &a->payload[cursor];
    }

    panic("segment %u not found, likely UB during traversal", seg_id);
}
