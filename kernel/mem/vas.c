#include <console/panic.h>
#include <kassert.h>
#include <math/align.h>
#include <math/div.h>
#include <mem/address_range.h>
#include <mem/alloc.h>
#include <mem/alloc_or_die.h>
#include <mem/hhdm.h>
#include <mem/page.h>
#include <mem/pmm.h>
#include <mem/vas.h>
#include <smp/core.h>
#include <string.h>

static size_t vas_cpu_id() {
    if (global.current_bootstage < BOOTSTAGE_MID_MP)
        return 0;

    return smp_core_id();
}

static size_t vas_range_get_data(struct rbt_node *n) {
    return container_of(n, struct vas_range, node)->start;
}

static int32_t vas_range_cmp(const struct rbt_node *a,
                             const struct rbt_node *b) {
    vaddr_t l = vas_range_get_data((void *) a);
    vaddr_t r = vas_range_get_data((void *) b);
    return (l > r) - (l < r);
}

static struct vas_range *vasrange_alloc(struct vas_local_tree *lt) {
    return fixed_size_alloc(&lt->fsr);
}

static void vasrange_free(struct vas_local_tree *lt, struct vas_range *r) {
    return fixed_size_free(&lt->fsr, r);
}

void vas_reclaim_freelist_pages(struct vas_local_tree *lt) {
    fixed_size_reclaim_freelist_pages(&lt->fsr);
}

static void vas_range_fsr_init(void *n) {
    struct vas_range *r = n;
    rbt_init_node(&r->node);
    r->length = 0;
    r->start = 0;
}

static void vas_local_tree_init(struct vas_local_tree *lt) {
    spinlock_init(&lt->lock);
    rbt_init(&lt->tree, vas_range_get_data, vas_range_cmp);
    struct fixed_size_range_attributes attrs = {
        .init_obj = vas_range_fsr_init,
        .deinit_obj = NULL,
        .bootstrap_mode = true,
        .obj_align = _Alignof(struct vas_range),
        .obj_size = sizeof(struct vas_range),
    };

    fixed_size_range_init(&lt->fsr, &attrs);
}

static vaddr_t tree_alloc(struct vas_local_tree *lt, size_t size,
                          size_t align) {
    struct rbt_node *node = rbt_min(&lt->tree);

    while (node) {
        struct vas_range *gap = rbt_entry(node, struct vas_range, node);

        vaddr_t aligned = ALIGN_UP(gap->start, align);
        vaddr_t end = aligned + size;

        if (end <= gap->start + gap->length) {
            rbt_delete(&lt->tree, &gap->node);

            if (aligned > gap->start) {
                struct vas_range *left = vasrange_alloc(lt);
                if (!left)
                    goto fail;
                left->start = gap->start;
                left->length = aligned - gap->start;
                rbt_insert(&lt->tree, &left->node);
            }

            if (end < gap->start + gap->length) {
                struct vas_range *right = vasrange_alloc(lt);
                if (!right)
                    goto fail;
                right->start = end;
                right->length = (gap->start + gap->length) - end;
                rbt_insert(&lt->tree, &right->node);
            }

            vasrange_free(lt, gap);
            lt->total_free -= size;
            return aligned;

        fail:
            rbt_insert(&lt->tree, &gap->node);
            return 0;
        }

        node = rbt_next(node);
    }

    return 0;
}

static void tree_free(struct vas_local_tree *lt, vaddr_t addr, size_t size) {
    vaddr_t start = addr;
    vaddr_t end = addr + size;

    struct rbt_node *node = lt->tree.root;
    struct vas_range *prev = NULL;
    struct vas_range *next = NULL;

    while (node) {
        struct vas_range *g = rbt_entry(node, struct vas_range, node);

        if (end <= g->start) {
            next = g;
            node = node->left;
        } else if (start >= g->start + g->length) {
            prev = g;
            node = node->right;
        } else {
            panic("vas_free: overlap/double free at %p+%zu", (void *) addr,
                  size);
        }
    }

    struct vas_range *reuse = NULL;

    if (prev && prev->start + prev->length == start) {
        start = prev->start;
        size += prev->length;
        rbt_delete(&lt->tree, &prev->node);
        reuse = prev;
    }

    if (next && end == next->start) {
        size += next->length;
        rbt_delete(&lt->tree, &next->node);
        if (reuse)
            vasrange_free(lt, next);
        else
            reuse = next;
    }

    if (!reuse)
        reuse = vasrange_alloc(lt);

    reuse->start = start;
    reuse->length = size;
    rbt_insert(&lt->tree, &reuse->node);

    lt->total_free += (end - addr);
}

static bool vas_pull_chunk(struct vas *vas, struct vas_local_tree *lt) {
    struct vas_local_tree *gl = &vas->global;
    size_t chunk = vas->chunk_size;

    enum irql irql = spin_lock(&gl->lock);

    /* Try to allocate a full chunk from the global tree */
    vaddr_t base = tree_alloc(gl, chunk, chunk);

    if (!base) {
        struct rbt_node *node = rbt_min(&gl->tree);
        struct vas_range *best = NULL;

        while (node) {
            struct vas_range *g = rbt_entry(node, struct vas_range, node);
            if (!best || g->length > best->length)
                best = g;
            node = rbt_next(node);
        }

        if (best && best->length >= PAGE_SIZE) {
            /* Take the whole gap */
            size_t take = best->length;
            base = best->start;

            rbt_delete(&gl->tree, &best->node);
            gl->total_free -= take;
            vasrange_free(gl, best);

            spin_unlock(&gl->lock, irql);

            /* Insert into local tree */
            enum irql lirql = spin_lock(&lt->lock);

            struct vas_range *nr = vasrange_alloc(lt);
            if (!nr) {
                spin_unlock(&lt->lock, lirql);
                /* Return the chunk back to global */
                irql = spin_lock(&gl->lock);
                tree_free(gl, base, take);
                spin_unlock(&gl->lock, irql);
                return false;
            }

            nr->start = base;
            nr->length = take;
            rbt_insert(&lt->tree, &nr->node);
            lt->total_free += take;

            spin_unlock(&lt->lock, lirql);
            return true;
        }

        spin_unlock(&gl->lock, irql);
        return false;
    }

    spin_unlock(&gl->lock, irql);

    /* Insert the chunk as a single gap into the local tree */
    enum irql lirql = spin_lock(&lt->lock);

    struct vas_range *nr = vasrange_alloc(lt);
    if (!nr) {
        spin_unlock(&lt->lock, lirql);
        /* Return chunk to global */
        irql = spin_lock(&gl->lock);
        tree_free(gl, base, chunk);
        spin_unlock(&gl->lock, irql);
        return false;
    }

    nr->start = base;
    nr->length = chunk;
    rbt_insert(&lt->tree, &nr->node);
    lt->total_free += chunk;

    spin_unlock(&lt->lock, lirql);
    return true;
}

/* faster approach: maintain a sorted interval list per-CPU of chunk
 * boundaries and binary-search it. But the simple version works first. */
static ssize_t vas_find_owner(struct vas *vas, vaddr_t addr) {
    size_t i;
    for_each_cpu_id(i) {
        struct vas_local_tree *lt = &vas->local[i];
        enum irql irql = spin_lock(&lt->lock);

        struct rbt_node *node = lt->tree.root;
        bool found = false;

        while (node) {
            struct vas_range *g = rbt_entry(node, struct vas_range, node);

            if (addr < g->start) {
                /* addr could be just below this gap */
                if (addr + PAGE_SIZE >= g->start) {
                    /* this tree likely owns it */
                    found = true;
                    break;
                }
                node = node->left;
            } else if (addr >= g->start + g->length) {
                /* addr is above this gap */
                if (addr < g->start + g->length + PAGE_SIZE) {
                    found = true;
                    break;
                }
                node = node->right;
            } else {
                /* double free */
                found = true;
                break;
            }
        }

        spin_unlock(&lt->lock, irql);

        if (found)
            return (ssize_t) i;
    }

    return -1;
}

vaddr_t vas_alloc(struct vas *vas, size_t size, size_t align) {
    uint32_t cpu = vas_cpu_id();
    struct vas_local_tree *lt = &vas->local[cpu];
    vaddr_t result;

    enum irql irql = spin_lock(&lt->lock);
    result = tree_alloc(lt, size, align);
    spin_unlock(&lt->lock, irql);

    if (result)
        return result;

    if (!vas_pull_chunk(vas, lt))
        return 0;

    irql = spin_lock(&lt->lock);
    result = tree_alloc(lt, size, align);
    spin_unlock(&lt->lock, irql);

    return result;
}

void vas_free(struct vas *vas, vaddr_t addr, size_t size) {

    ssize_t owner = vas_find_owner(vas, addr);
    struct vas_local_tree *lt;

    if (owner >= 0) {
        lt = &vas->local[owner];
    } else {
        lt = &vas->local[vas_cpu_id()];
    }

    enum irql irql = spin_lock(&lt->lock);
    tree_free(lt, addr, size);
    spin_unlock(&lt->lock, irql);
}

static void vas_space_init_common(struct vas *vas, vaddr_t base,
                                  vaddr_t limit) {
    vas->base = base;
    vas->limit = limit;
    vas->chunk_size = VAS_CHUNK_SIZE;

    /* Init global tree */
    vas_local_tree_init(&vas->global);

    enum irql irql = spin_lock(&vas->global.lock);

    struct vas_range *g = alloc_or_die(vasrange_alloc(&vas->global));

    g->start = base;
    g->length = limit - base;
    rbt_insert(&vas->global.tree, &g->node);
    vas->global.total_free = limit - base;

    spin_unlock(&vas->global.lock, irql);

    for (uint32_t i = 0; i < global.core_count; i++)
        vas_local_tree_init(&vas->local[i]);
}

struct vas *vas_bootstrap(vaddr_t base, vaddr_t limit) {
    if (global.current_bootstage >= BOOTSTAGE_LATE)
        log_warn_once("vas_space_bootstrap called after bootstage %s",
                      bootstage_str[BOOTSTAGE_LATE]);

    uint32_t ncpus = global.core_count;
    size_t total_size =
        sizeof(struct vas) + ncpus * sizeof(struct vas_local_tree);
    size_t pages_needed = DIV_ROUND_UP(total_size, PAGE_SIZE);

    uintptr_t phys = alloc_or_die(pmm_alloc_page(pages_needed));
    uintptr_t virt = hhdm_paddr_to_vaddr(phys);
    memset((void *) virt, 0, PAGE_SIZE);

    struct vas *vas = (struct vas *) virt;
    vas->local =
        (struct vas_local_tree *) ((uint8_t *) vas + sizeof(struct vas));

    vas_space_init_common(vas, base, limit);
    return vas;
}

struct vas *vas_create(vaddr_t base, vaddr_t limit) {
    uint32_t ncpus = global.core_count;
    size_t total_size =
        sizeof(struct vas) + ncpus * sizeof(struct vas_local_tree);

    struct vas *vas =
        kmalloc(total_size, ALLOC_FLAGS_ZERO,
                ALLOC_BEHAVIOR_NORMAL | ALLOC_BEHAVIOR_FLAG_MINIMAL);
    if (!vas)
        return NULL;

    vas->local =
        (struct vas_local_tree *) ((uint8_t *) vas + sizeof(struct vas));

    vas_space_init_common(vas, base, limit);
    return vas;
}

void *vas_map(struct vas *vas, paddr_t paddr, size_t len, uint64_t flags,
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

void vas_unmap(struct vas *vas, void *vaddr, size_t len) {
    size_t pages = DIV_ROUND_UP(len, PAGE_SIZE);
    vmm_unmap(vaddr, PAGE_SIZE * pages, VMM_FLAG_NONE);
    vas_free(vas, (vaddr_t) vaddr, PAGE_SIZE * pages);
}

struct vas *vas_from(struct address_range *ar) {
    return vas_create(ar->base, ar->base + ar->size);
}

struct vas *vas_bootstrap_from(struct address_range *ar) {
    return vas_bootstrap(ar->base, ar->base + ar->size);
}

static void vas_dump_local_tree(struct vas_local_tree *lt, bool take_lock) {
    enum irql irql = 0;
    if (take_lock)
        irql = spin_lock(&lt->lock);

    size_t count = 0;
    size_t largest = 0;
    vaddr_t largest_start = 0;

    for (struct rbt_node *n = rbt_min(&lt->tree); n; n = rbt_next(n)) {
        struct vas_range *r = rbt_entry(n, struct vas_range, node);
        printf("    [%zu] %p .. %p  len=%zu (%zu KiB)\n", count,
               (void *) r->start, (void *) (r->start + r->length), r->length,
               r->length >> 10);
        if (r->length > largest) {
            largest = r->length;
            largest_start = r->start;
        }
        count++;
    }

    printf(
        "    %zu free range(s), total_free=%zu (%zu KiB), largest=%zu @ %p\n",
        count, lt->total_free, lt->total_free >> 10, largest,
        (void *) largest_start);

    if (take_lock)
        spin_unlock(&lt->lock, irql);
}

void vas_space_dump(struct vas *vas) {
    size_t span = (size_t) (vas->limit - vas->base);
    printf("vas_space %p: base=%p limit=%p span=%zu (%zu MiB) chunk_size=%zu\n",
           (void *) vas, (void *) vas->base, (void *) vas->limit, span,
           span >> 20, vas->chunk_size);

    printf("  global tree:\n");
    vas_dump_local_tree(&vas->global, true);

    for (uint32_t i = 0; i < global.core_count; i++) {
        printf("  cpu[%u] tree:\n", i);
        vas_dump_local_tree(&vas->local[i], true);
    }
}
