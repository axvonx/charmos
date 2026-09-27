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
