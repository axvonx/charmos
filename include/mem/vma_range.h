/* @title: Virtual Memory Area */
#pragma once
#include <mem/page.h>
#include <structures/list.h>
#include <structures/rbit.h>
#include <types/types.h>

enum vma_range_protection {
    VMA_PROT_READ = 1,
    VMA_PROT_WRITE = 1 << 1,
    VMA_PROT_EX = 1 << 2,
};

struct vma_range {
    struct rbit_node mm_node;

    /* min_low = lowest start in the subtree */
    vaddr_t min_low;

    /* max_gap = largest free gap between consecutive VMAs in the subtree */
    size_t max_gap;
    pgoff_t pgoff;                  /* object-space offset of `start` */
    enum vma_range_protection prot; /* r/w/x, etc. */
    struct anon_vma *anon_vma;
    struct mm *mm;                   /* whose page tables */
    struct list_head anon_vma_chain; /* AVCs: own object + every ancestor */
};

static inline vaddr_t vma_range_start(const struct vma_range *vma_range) {
    return vma_range->mm_node.interval.low;
}
static inline vaddr_t vma_range_end(const struct vma_range *vma_range) {
    return vma_range->mm_node.interval.high + 1;
}

struct vma_range *vma_range_alloc(struct mm *mm, vaddr_t start, vaddr_t end,
                                  enum vma_range_protection prot);
void vma_range_free(struct vma_range *vma_range);

void vma_range_init(struct vma_range *vma_range, struct mm *mm, vaddr_t start,
                    vaddr_t end, enum vma_range_protection prot);

struct vma_range *vma_range_find(struct mm *mm, vaddr_t addr);
struct vma_range *vma_range_find_intersection(struct mm *mm, vaddr_t s,
                                              vaddr_t e);
struct vma_range *vma_range_next(struct vma_range *vma_range);
struct vma_range *vma_range_prev(struct vma_range *vma_range);

/* fault path helper: write fault on an anon VMA, allocate anon_vma
 * and AVC, link, stash on vma_range->anon_vma */
enum errno vma_range_anon_prepare(struct vma_range *vma_range);

enum errno vma_range_expand(struct vma_range *vma_range, vaddr_t new_start,
                            vaddr_t new_end);
struct vma_range *vma_range_merge(struct mm *mm, struct vma_range *prev,
                                  struct vma_range *vma_range);
enum errno vma_range_set_prot(struct vma_range *vma_range,
                              enum vma_range_protection prot);

void vma_range_unmap_range(struct vma_range *vma_range, vaddr_t s, vaddr_t e);
void vma_range_teardown(struct vma_range *vma_range);

static inline bool vma_range_is_anonymous(const struct vma_range *vma_range) {
    return vma_range->anon_vma != NULL;
}
static inline size_t vma_range_pages(const struct vma_range *vma_range) {
    return (vma_range_end(vma_range) - vma_range_start(vma_range)) >>
           PAGE_4K_SHIFT;
}

/* split one VMA into two at `addr`, recompute pgoff + tree intervals */
struct vma_range *vma_range_split(struct vma_range *vma_range, vaddr_t addr);

/* fresh VMA in dst_mm with src's attributes but no linkages */
struct vma_range *vma_range_dup(struct mm *dst_mm, const struct vma_range *src);

/* The index to VA translation. THE VMA's job, not the anon_vma's. */
static inline vaddr_t vma_range_address(const struct vma_range *vma_range,
                                        pgoff_t index) {
    return vma_range_start(vma_range) +
           ((index - vma_range->pgoff) << PAGE_4K_SHIFT);
}
