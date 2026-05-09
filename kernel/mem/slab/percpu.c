#include <sch/sched.h>
#include <smp/domain.h>

#include "internal.h"

bool slab_magazine_push(struct slab_magazine *mag, vaddr_t obj) {
    if (mag->count < SLAB_MAG_ENTRIES) {
        mag->objs[mag->count++] = obj;
        return true;
    }
    return false;
}

vaddr_t slab_magazine_pop(struct slab_magazine *mag) {

    vaddr_t ret = 0x0;

    if (mag->count > 0) {
        ret = mag->objs[--mag->count];
        mag->objs[mag->count] = 0x0; /* Reset it */
    }

    return ret;
}

static size_t slab_cache_bulk_alloc(struct slab_cache *cache,
                                    vaddr_t *addr_array, size_t num_objects,
                                    enum alloc_behavior behavior) {
    size_t total_allocated = 0;

    for (size_t i = 0; i < num_objects; i++) {

        /* We don't allow allocations of new slabs - that
         * is not the point of our percpu caches */
        kassert(!(behavior & SLAB_ALLOC_BEHAVIOR_FROM_ALLOC));
        void *obj = slab_alloc(cache, behavior);
        if (!obj)
            break;

        total_allocated++;
        addr_array[i] = (vaddr_t) obj;
    }

    return total_allocated;
}

static void slab_cache_bulk_free(struct slab_domain *domain,
                                 vaddr_t *addr_array, size_t num_objects) {
    for (size_t i = 0; i < num_objects; i++) {
        vaddr_t addr = addr_array[i];

        if (!slab_free_queue_ringbuffer_enqueue(&domain->free_queue, addr))
            slab_free(domain, (void *) addr);
    }

    return;
}

void slab_percpu_flush(struct slab_domain *dom, struct slab_percpu_cache *pc,
                       size_t class_idx, vaddr_t overflow_obj) {
    struct slab_magazine *mag = &pc->mag[class_idx];

    /* capture how many valid items we have */
    size_t valid = mag->count;

    /* copy only valid objects */
    for (size_t i = 0; i < valid; i++)
        pc->shadow_objs[i] = mag->objs[i];

    /* reset magazine */
    mag->count = 0;
    for (size_t i = 0; i < valid; i++)
        mag->objs[i] = 0x0;

    /* add overflow object to the end and free (valid + 1) objects */
    pc->shadow_objs[valid] = overflow_obj;
    slab_cache_bulk_free(dom, pc->shadow_objs, valid + 1);
}

static vaddr_t slab_percpu_refill_class(struct slab_domain *dom,
                                        struct slab_percpu_cache *pc,
                                        size_t class_idx,
                                        enum alloc_behavior behavior) {
    struct slab_cache *cache = &dom->local_nonpageable_cache->caches[class_idx];
    struct slab_magazine *mag = &pc->mag[class_idx];

    size_t space = SLAB_MAG_ENTRIES - mag->count;

    if (space == 0)
        space = 1; /* we still want 1 so we can return an object */

    kassert(space <= SLAB_MAG_ENTRIES);

    size_t want = space;
    size_t got = slab_cache_bulk_alloc(cache, pc->shadow_objs, want, behavior);
    if (got == 0)
        return slab_magazine_pop(mag);

    size_t can_insert = SLAB_MAG_ENTRIES - mag->count;
    size_t to_insert = (got > 0) ? (got - 1) : 0;
    if (to_insert > can_insert)
        to_insert = can_insert;

    for (size_t i = 1; i <= to_insert; i++)
        mag->objs[mag->count++] = pc->shadow_objs[i];

    return pc->shadow_objs[0];
}

void slab_percpu_refill(struct slab_domain *dom,
                        struct slab_percpu_cache *cache,
                        enum alloc_behavior behavior) {
    /* This flushes a portion of the freequeue into the percpu cache */
    slab_free_queue_drain_limited(cache, dom, /* pct = */ 100);
    for (size_t class = 0; class < slab_global.num_sizes; class++)
        slab_percpu_refill_class(dom, cache, class, behavior);
}

/* TODO: memory locality */
void slab_domain_percpu_init(struct slab_domain *domain) {
    size_t cpus = domain->domain->num_cores;
    domain->percpu_caches = kzalloc(sizeof(struct slab_percpu_cache *) * cpus);
    if (!domain->percpu_caches)
        panic("Could not allocate domain's percpu caches\n");

    for (size_t i = 0; i < cpus; i++) {
        domain->percpu_caches[i] = kzalloc(sizeof(struct slab_percpu_cache));
        domain->percpu_caches[i]->mag =
            kzalloc(sizeof(struct slab_magazine) * slab_global.num_sizes);

        if (!domain->percpu_caches[i] || !domain->percpu_caches[i]->mag)
            panic("Could not allocate domain's percpu caches\n");

        domain->percpu_caches[i]->domain = domain;
        for (size_t j = 0; j < slab_global.num_sizes; j++) {
            struct slab_magazine *mag = &domain->percpu_caches[i]->mag[j];
            mag->count = 0;
        }
    }
}
