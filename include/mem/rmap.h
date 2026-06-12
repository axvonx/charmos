/* @title: Reverse map */
#pragma once
#include <types/types.h>
struct folio;
struct mm;
struct vma_range;

/* visitor invoked for each confirmed (mm, va) mapping the folio */
typedef void (*rmap_visit_fn)(struct mm *mm, vaddr_t va, struct folio *f);

/* folio -> object -> itree query -> per-VMA address -> PTE confirm */
void rmap_walk_anon(struct folio *f, rmap_visit_fn visit);

/* fault path: first PTE to map this folio */
void folio_add_anon_rmap(struct folio *f, struct vma_range *vm_area,
                         vaddr_t va);

/* additional mapper */
void folio_add_anon_rmap_shared(struct folio *f, struct vma_range *vm_area);

/* a PTE stops mapping it */
void folio_remove_rmap(struct folio *f, struct vma_range *vm_area);

void folio_add_anon_rmap_new(struct folio *f, struct vma_range *vm_area,
                             vaddr_t va);
