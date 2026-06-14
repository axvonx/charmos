#include <math/bit_ops.h>
#include <math/pow.h>
#include <mem/domain.h>
#include <mem/pmm.h>

#include "internal.h"

bool slab_can_resize_to(struct slab *slab, size_t new_size_pages) {
    size_t cap = next_pow2(slab->page_count);
    size_t min = (cap >> 1) + 1;
    return new_size_pages >= min && new_size_pages <= cap;
}

static void slab_shrink(struct slab *slab, size_t start, size_t end,
                        bool assert_nonnull) {
    for (size_t i = start; i < end; i++) {
        struct page *page = slab->backing_pages[i];
        if (assert_nonnull)
            kassert(page);

        if (!page)
            return;

        paddr_t phys = page_get_paddr(page);
        vaddr_t virt = (vaddr_t) slab + i * PAGE_SIZE;
        vmm_unmap_page(virt);
        pmm_free_page(phys);
        slab->backing_pages[i] = NULL;
    }

    slab->page_count = start;
}

bool slab_resize(struct slab *slab, size_t new_size_pages) {
    kassert(slab_can_resize_to(slab, new_size_pages));
    size_t old = slab->page_count;

    if (old == new_size_pages)
        return true;

    struct slab_domain *parent = slab->parent_cache->parent_domain;

    /* Grow */
    if (old < new_size_pages) {
        for (size_t i = old; i < new_size_pages; i++)
            slab->backing_pages[i] = NULL;

        for (size_t i = old; i < new_size_pages; i++) {
            paddr_t phys = domain_alloc_from_domain(parent->domain, 1);
            if (!phys)
                goto grow_err;

            vaddr_t virt = (vaddr_t) slab + i * PAGE_SIZE;
            uint64_t flags = slab_page_flags(slab->type);
            if (unlikely(vmm_map_page(virt, phys, flags) < 0)) {
                pmm_free_page(phys); /* not yet recorded, free directly */
                goto grow_err;
            }

            slab->backing_pages[i] = page_for_paddr(phys);
        }

        slab->page_count = new_size_pages;
        return true;

    grow_err:
        slab_shrink(slab, old, new_size_pages, /* assert_nonnull = */ false);
        return false;
    }

    slab_shrink(slab, new_size_pages, old, /* assert_nonnull = */ true);
    return true;
}
