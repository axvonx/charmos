/* @title: Reverse map */
#pragma once
#include <types/types.h>
struct folio;
struct mm;
struct vma_range;

/* The way that rmap goes is:
 *
 * page -> folio -> mapping object (file_vma, anon_vma) ->
 * traverse interval tree -> for each matching avc -> vma_range -> addr
 */

/* visitor invoked for each confirmed (mm, va) mapping the folio */
typedef void (*rmap_visit_fn)(struct mm *mm, vaddr_t va, struct folio *f,
                              void *private);

void rmap_walk_anon(struct folio *f, rmap_visit_fn visit, void *private);

/* fault path: first PTE to map this folio */
void folio_add_anon_rmap(struct folio *f, struct vma_range *vm_area,
                         vaddr_t va);

/* additional mapper */
void folio_add_anon_rmap_shared(struct folio *f, struct vma_range *vm_area);

/* a PTE stops mapping it */
void folio_remove_rmap(struct folio *f, struct vma_range *vm_area);

void folio_add_anon_rmap_new(struct folio *f, struct vma_range *vm_area,
                             vaddr_t va);
