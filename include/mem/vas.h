/* @title: Virtual address allocator */
#pragma once
#include <mem/page.h>
#include <mem/vmm.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

struct address_range;
struct vas;
struct vas_arena;

#define VAS_CHUNK_SHIFT 26
#define VAS_CHUNK_SIZE (1ULL << VAS_CHUNK_SHIFT)

/* [base, limit) */
struct vas *vas_bootstrap(vaddr_t base, vaddr_t limit);
struct vas *vas_create(vaddr_t base, vaddr_t limit);
struct vas *vas_from(struct address_range *ar);
struct vas *vas_bootstrap_from(struct address_range *ar);

/* No touching in interrupts or above DISPATCH */
vaddr_t vas_alloc(struct vas *vas, size_t size, size_t align);

/* Giving the wrong size panics. TODO: we only need to pass in addr,
 * we can update the APIs for that later on */
void vas_free(struct vas *vas, vaddr_t addr, size_t size);

void *vas_map(struct vas *vas, paddr_t paddr, size_t len, uint64_t flags,
              enum vmm_flags vflags);
void vas_unmap(struct vas *vas, void *vaddr, size_t len);

/* Drain cached reservations */
void vas_reclaim(struct vas *vas);
void vas_reclaim_freelist_pages(struct vas_arena *arena);

/* TODO: virtual address spaces are NOT refcounted */
bool vas_destroy(struct vas *vas);
void vas_space_dump(struct vas *vas);

bool vas_vaddr_in_vas(struct vas *vas, vaddr_t addr);
/* Includes interior byte addresses */
bool vas_vaddr_is_allocated(struct vas *vas, vaddr_t addr);
