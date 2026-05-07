#include <console/panic.h>
#include <kassert.h>
#include <math/align.h>
#include <math/div.h>
#include <mem/address_range.h>
#include <mem/alloc.h>
#include <mem/hhdm.h>
#include <mem/page.h>
#include <mem/pmm.h>
#include <mem/vaddr_alloc.h>
#include <string.h>

#define VASRANGE_PER_PAGE (PAGE_SIZE / sizeof(struct vas_range))

static size_t vas_range_get_data(struct rbt_node *n) {
    return container_of(n, struct vas_range, node)->start;
}

static int32_t vas_range_cmp(const struct rbt_node *a,
                             const struct rbt_node *b) {
    vaddr_t l = vas_range_get_data((void *) a);
    vaddr_t r = vas_range_get_data((void *) b);
    return (l > r) - (l < r);
}

static bool vasrange_refill(struct vas_space *space) {
    SPINLOCK_ASSERT_HELD(&space->lock);

    uintptr_t phys = pmm_alloc_page();

    if (!phys)
        return false;

    uintptr_t virt = hhdm_paddr_to_vaddr(phys);
    struct vas_range *ranges = (struct vas_range *) virt;

    for (uint64_t i = 0; i < VASRANGE_PER_PAGE; i++) {
        INIT_LIST_HEAD(&ranges[i].free_list_node);
        list_add_tail(&ranges[i].free_list_node, &space->freelist);
    }

    return true;
}

static struct vas_range *vasrange_alloc(struct vas_space *space) {
    SPINLOCK_ASSERT_HELD(&space->lock);

    if (list_empty(&space->freelist)) {
        if (!vasrange_refill(space))
            return NULL;
    }

    struct list_head *pop = list_pop_front_init(&space->freelist);
    struct vas_range *r = container_of(pop, struct vas_range, free_list_node);
    return r;
}

static void vasrange_free(struct vas_space *space, struct vas_range *r) {
    SPINLOCK_ASSERT_HELD(&space->lock);

    list_add_tail(&r->free_list_node, &space->freelist);
}

struct vas_space *vas_space_bootstrap(vaddr_t base, vaddr_t limit) {
    if (global.current_bootstage >= BOOTSTAGE_LATE)
        log_warn_once("vas_space_bootstrap called after bootstage %s",
                      bootstage_str[BOOTSTAGE_LATE]);

    uintptr_t phys = pmm_alloc_page();
    if (!phys)
        panic("OOM creating vas_space\n");

    uintptr_t virt = hhdm_paddr_to_vaddr(phys);
    struct vas_space *vas = (struct vas_space *) virt;

    memset(vas, 0, sizeof(*vas));
    vas->base = base;
    vas->limit = limit;
    INIT_LIST_HEAD(&vas->freelist);

    rbt_init(&vas->tree, vas_range_get_data, vas_range_cmp);
    spinlock_init(&vas->lock);

    /* initial full gap */
    enum irql irql = vas_space_lock(vas);

    struct vas_range *g = vasrange_alloc(vas);
    if (!g)
        panic("OOM\n");

    g->start = base;
    g->length = limit - base;

    rbt_insert(&vas->tree, &g->node);

    vas_space_unlock(vas, irql);
    return vas;
}

struct vas_space *vas_space_create(vaddr_t base, vaddr_t limit) {
    struct vas_space *vas =
        kzalloc(sizeof(struct vas_space), ALLOC_FLAGS_DEFAULT,
                /* Prevent recursing into ourselves */
                ALLOC_BEHAVIOR_NORMAL | ALLOC_BEHAVIOR_FLAG_MINIMAL);
    if (!vas)
        return NULL;

    vas->base = base;
    vas->limit = limit;
    INIT_LIST_HEAD(&vas->freelist);

    rbt_init(&vas->tree, vas_range_get_data, vas_range_cmp);
    spinlock_init(&vas->lock);

    enum irql irql = vas_space_lock(vas);

    struct vas_range *g = vasrange_alloc(vas);
    if (!g)
        return NULL;

    g->start = base;
    g->length = limit - base;

    rbt_insert(&vas->tree, &g->node);

    vas_space_unlock(vas, irql);
    return vas;
}

vaddr_t vas_alloc(struct vas_space *vas, size_t size, size_t align) {
    enum irql irql = vas_space_lock(vas);

    struct rbt_node *node = rbt_min(&vas->tree);

    while (node) {
        struct vas_range *gap = rbt_entry(node, struct vas_range, node);

        vaddr_t aligned = ALIGN_UP(gap->start, align);
        vaddr_t end = aligned + size;

        if (end <= gap->start + gap->length) {
            /* remove current gap */
            rbt_delete(&vas->tree, &gap->node);

            /* left split */
            if (aligned > gap->start) {
                struct vas_range *left = vasrange_alloc(vas);
                if (!left)
                    goto out;

                left->start = gap->start;
                left->length = aligned - gap->start;
                rbt_insert(&vas->tree, &left->node);
            }

            /* right split */
            if (end < gap->start + gap->length) {
                struct vas_range *right = vasrange_alloc(vas);
                if (!right)
                    goto out;

                right->start = end;
                right->length = (gap->start + gap->length) - end;
                rbt_insert(&vas->tree, &right->node);
            }

            vasrange_free(vas, gap);

            vas_space_unlock(vas, irql);
            return aligned;

        out:
            rbt_insert(&vas->tree, &gap->node);
            vas_space_unlock(vas, irql);
            return 0;
        }

        node = rbt_next(node);
    }

    vas_space_unlock(vas, irql);
    return 0;
}

void vas_free(struct vas_space *vas, vaddr_t addr, size_t size) {
    enum irql irql = vas_space_lock(vas);

    vaddr_t start = addr;
    vaddr_t end = addr + size;

    struct rbt_node *node = vas->tree.root;
    struct vas_range *prev = NULL;
    struct vas_range *next = NULL;

    /* find neighbors */
    while (node) {
        struct vas_range *g = rbt_entry(node, struct vas_range, node);

        if (end <= g->start) {
            next = g;
            node = node->left;
        } else if (start >= g->start + g->length) {
            prev = g;
            node = node->right;
        } else {
            panic("vas_free: overlap/double free");
        }
    }

    /* merge with previous */
    struct vas_range *reuse = NULL;
    if (prev && prev->start + prev->length == start) {
        start = prev->start;
        size += prev->length;

        rbt_delete(&vas->tree, &prev->node);
        reuse = prev;
    }

    /* merge with next */
    if (next && end == next->start) {
        size += next->length;

        rbt_delete(&vas->tree, &next->node);

        if (reuse) {
            vasrange_free(vas, next);
        } else {
            reuse = next;
        }
    }

    if (!reuse)
        reuse = vasrange_alloc(vas);

    /* insert merged gap */
    reuse->start = start;
    reuse->length = size;

    rbt_insert(&vas->tree, &reuse->node);

    vas_space_unlock(vas, irql);
}

void *vas_map(struct vas_space *vas, paddr_t paddr, size_t len, uint64_t flags,
              enum vmm_flags vflags) {
    size_t pages = DIV_ROUND_UP(len, PAGE_SIZE);
    vaddr_t vaddr = vas_alloc(vas, PAGE_SIZE * pages, PAGE_SIZE);
    if (!vaddr)
        return NULL;

    void *ret = vmm_map(paddr, vaddr, len, flags, vflags);
    if (!ret)
        vas_free(vas, vaddr, PAGE_SIZE * pages);

    return ret;
}

void vas_unmap(struct vas_space *vas, void *vaddr, size_t len) {
    size_t pages = DIV_ROUND_UP(len, PAGE_SIZE);
    vmm_unmap(vaddr, PAGE_SIZE * pages, VMM_FLAG_NONE);
    vas_free(vas, (vaddr_t) vaddr, PAGE_SIZE * pages);
}

struct vas_space *vas_space_from_address_range(struct address_range *ar) {
    return vas_space_create(ar->base, ar->base + ar->size);
}
