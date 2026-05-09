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
#include <smp/core.h>
#include <string.h>

#define VASRANGE_HDR_SIZE                                                      \
    ALIGN_UP(sizeof(struct vasrange_page_hdr), _Alignof(struct vas_range))
#define VASRANGE_PER_PAGE                                                      \
    ((PAGE_SIZE - VASRANGE_HDR_SIZE) / sizeof(struct vas_range))

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

static struct vasrange_page_hdr *vasrange_page_of(struct vas_range *r) {
    return (struct vasrange_page_hdr *) ALIGN_DOWN((uintptr_t) r, PAGE_SIZE);
}

static bool vasrange_refill(struct vas_local_tree *lt) {
    uintptr_t phys = pmm_alloc_page();
    if (!phys)
        return false;

    uintptr_t virt = hhdm_paddr_to_vaddr(phys);

    struct vasrange_page_hdr *hdr = (struct vasrange_page_hdr *) virt;
    hdr->total = VASRANGE_PER_PAGE;
    hdr->free_count = VASRANGE_PER_PAGE;
    INIT_LIST_HEAD(&hdr->page_list);
    list_add_tail(&hdr->page_list, &lt->fl_pages);

    struct vas_range *ranges = (struct vas_range *) (virt + VASRANGE_HDR_SIZE);

    for (uint32_t i = 0; i < VASRANGE_PER_PAGE; i++) {
        INIT_LIST_HEAD(&ranges[i].free_list_node);
        list_add_tail(&ranges[i].free_list_node, &lt->freelist);
    }

    return true;
}

static struct vas_range *vasrange_alloc(struct vas_local_tree *lt) {
    if (list_empty(&lt->freelist)) {
        if (!vasrange_refill(lt))
            return NULL;
    }

    struct list_head *pop = list_pop_front_init(&lt->freelist);
    struct vas_range *r = container_of(pop, struct vas_range, free_list_node);

    vasrange_page_of(r)->free_count--;
    return r;
}

static void vasrange_free(struct vas_local_tree *lt, struct vas_range *r) {
    vasrange_page_of(r)->free_count++;
    list_add_tail(&r->free_list_node, &lt->freelist);
}

void vas_reclaim_freelist_pages(struct vas_local_tree *lt) {
    struct list_head *pos, *tmp;

    list_for_each_safe(pos, tmp, &lt->fl_pages) {
        struct vasrange_page_hdr *hdr =
            container_of(pos, struct vasrange_page_hdr, page_list);

        if (hdr->free_count < hdr->total)
            continue;

        struct vas_range *ranges =
            (struct vas_range *) ((uintptr_t) hdr + VASRANGE_HDR_SIZE);

        for (uint32_t i = 0; i < hdr->total; i++)
            list_del_init(&ranges[i].free_list_node);

        list_del(&hdr->page_list);
        pmm_free_page(hhdm_vaddr_to_paddr((uintptr_t) hdr));
    }
}

static void vas_local_tree_init(struct vas_local_tree *lt) {
    spinlock_init(&lt->lock);
    rbt_init(&lt->tree, vas_range_get_data, vas_range_cmp);
    INIT_LIST_HEAD(&lt->freelist);
    INIT_LIST_HEAD(&lt->fl_pages);
    lt->total_free = 0;
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

static bool vas_pull_chunk(struct vas_space *vas, struct vas_local_tree *lt) {
    struct vas_local_tree *gl = &vas->global;
    size_t chunk = vas->chunk_size;

    enum irql irql = vas_local_tree_lock(gl);

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

            vas_local_tree_unlock(gl, irql);

            /* Insert into local tree */
            enum irql lirql = vas_local_tree_lock(lt);

            struct vas_range *nr = vasrange_alloc(lt);
            if (!nr) {
                vas_local_tree_unlock(lt, lirql);
                /* Return the chunk back to global */
                irql = vas_local_tree_lock(gl);
                tree_free(gl, base, take);
                vas_local_tree_unlock(gl, irql);
                return false;
            }

            nr->start = base;
            nr->length = take;
            rbt_insert(&lt->tree, &nr->node);
            lt->total_free += take;

            vas_local_tree_unlock(lt, lirql);
            return true;
        }

        vas_local_tree_unlock(gl, irql);
        return false;
    }

    vas_local_tree_unlock(gl, irql);

    /* Insert the chunk as a single gap into the local tree */
    enum irql lirql = vas_local_tree_lock(lt);

    struct vas_range *nr = vasrange_alloc(lt);
    if (!nr) {
        vas_local_tree_unlock(lt, lirql);
        /* Return chunk to global */
        irql = vas_local_tree_lock(gl);
        tree_free(gl, base, chunk);
        vas_local_tree_unlock(gl, irql);
        return false;
    }

    nr->start = base;
    nr->length = chunk;
    rbt_insert(&lt->tree, &nr->node);
    lt->total_free += chunk;

    vas_local_tree_unlock(lt, lirql);
    return true;
}

/* faster approach: maintain a sorted interval list per-CPU of chunk
 * boundaries and binary-search it. But the simple version works first. */
static ssize_t vas_find_owner(struct vas_space *vas, vaddr_t addr) {
    size_t i;
    for_each_cpu_id(i) {
        struct vas_local_tree *lt = &vas->local[i];
        enum irql irql = vas_local_tree_lock(lt);

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

        vas_local_tree_unlock(lt, irql);

        if (found)
            return (ssize_t) i;
    }

    return -1;
}

vaddr_t vas_alloc(struct vas_space *vas, size_t size, size_t align) {
    uint32_t cpu = vas_cpu_id();
    struct vas_local_tree *lt = &vas->local[cpu];
    vaddr_t result;

    enum irql irql = vas_local_tree_lock(lt);
    result = tree_alloc(lt, size, align);
    vas_local_tree_unlock(lt, irql);

    if (result)
        return result;

    if (!vas_pull_chunk(vas, lt))
        return 0;

    irql = vas_local_tree_lock(lt);
    result = tree_alloc(lt, size, align);
    vas_local_tree_unlock(lt, irql);

    return result;
}

void vas_free(struct vas_space *vas, vaddr_t addr, size_t size) {

    ssize_t owner = vas_find_owner(vas, addr);
    struct vas_local_tree *lt;

    if (owner >= 0) {
        lt = &vas->local[owner];
    } else {
        lt = &vas->local[vas_cpu_id()];
    }

    enum irql irql = vas_local_tree_lock(lt);
    tree_free(lt, addr, size);
    vas_local_tree_unlock(lt, irql);
}

static void vas_space_init_common(struct vas_space *vas, vaddr_t base,
                                  vaddr_t limit) {
    vas->base = base;
    vas->limit = limit;
    vas->chunk_size = VAS_CHUNK_SIZE;

    /* Init global tree */
    vas_local_tree_init(&vas->global);

    enum irql irql = vas_local_tree_lock(&vas->global);

    struct vas_range *g = vasrange_alloc(&vas->global);
    if (!g)
        panic("OOM: cannot create initial VAS gap");

    g->start = base;
    g->length = limit - base;
    rbt_insert(&vas->global.tree, &g->node);
    vas->global.total_free = limit - base;

    vas_local_tree_unlock(&vas->global, irql);

    for (uint32_t i = 0; i < global.core_count; i++)
        vas_local_tree_init(&vas->local[i]);
}

struct vas_space *vas_space_bootstrap(vaddr_t base, vaddr_t limit) {
    if (global.current_bootstage >= BOOTSTAGE_LATE)
        log_warn_once("vas_space_bootstrap called after bootstage %s",
                      bootstage_str[BOOTSTAGE_LATE]);

    uint32_t ncpus = global.core_count;
    size_t total_size =
        sizeof(struct vas_space) + ncpus * sizeof(struct vas_local_tree);
    size_t pages_needed = DIV_ROUND_UP(total_size, PAGE_SIZE);

    kassert(pages_needed == 1);
    uintptr_t phys = pmm_alloc_page();
    if (!phys)
        panic("OOM creating vas_space\n");

    uintptr_t virt = hhdm_paddr_to_vaddr(phys);
    memset((void *) virt, 0, PAGE_SIZE);

    struct vas_space *vas = (struct vas_space *) virt;
    vas->local =
        (struct vas_local_tree *) ((uint8_t *) vas + sizeof(struct vas_space));

    vas_space_init_common(vas, base, limit);
    return vas;
}

struct vas_space *vas_space_create(vaddr_t base, vaddr_t limit) {
    uint32_t ncpus = global.core_count;
    size_t total_size =
        sizeof(struct vas_space) + ncpus * sizeof(struct vas_local_tree);

    struct vas_space *vas =
        kzalloc(total_size, ALLOC_FLAGS_DEFAULT,
                ALLOC_BEHAVIOR_NORMAL | ALLOC_BEHAVIOR_FLAG_MINIMAL);
    if (!vas)
        return NULL;

    vas->local =
        (struct vas_local_tree *) ((uint8_t *) vas + sizeof(struct vas_space));

    vas_space_init_common(vas, base, limit);
    return vas;
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
