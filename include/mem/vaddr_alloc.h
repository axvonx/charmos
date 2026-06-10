/* @title: Virtual address allocator */
#pragma once
#include <mem/fixed_size_alloc.h>
#include <mem/page.h>
#include <stdatomic.h>
#include <stdint.h>
#include <structures/list.h>
#include <structures/rbt.h>
#include <sync/spinlock.h>
#include <types/types.h>

struct address_range;

#define VAS_CHUNK_SIZE PAGE_1GB

struct vas_range {
    vaddr_t start;
    size_t length;
    struct rbt_node node;
};

struct vas_local_tree {
    struct spinlock lock;
    struct rbt tree;
    struct fixed_size_range fsr;
    size_t total_free;
};

struct vas_space {
    struct vas_local_tree global;

    vaddr_t base;
    vaddr_t limit;
    size_t chunk_size;

    struct vas_local_tree *local;
};

struct vas_space *vas_space_bootstrap(vaddr_t base, vaddr_t limit);
struct vas_space *vas_space_create(vaddr_t base, vaddr_t limit);
struct vas_space *vas_space_from_address_range(struct address_range *ar);

vaddr_t vas_alloc(struct vas_space *vas, size_t size, size_t align);
void vas_free(struct vas_space *vas, vaddr_t addr, size_t size);

void *vas_map(struct vas_space *vas, paddr_t paddr, size_t len, uint64_t flags,
              enum vmm_flags vflags);
void vas_unmap(struct vas_space *vas, void *vaddr, size_t len);

void vas_reclaim_freelist_pages(struct vas_local_tree *lt);
