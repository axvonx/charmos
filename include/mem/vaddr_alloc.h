/* @title: Virtual address allocator */
#pragma once
#include <stdatomic.h>
#include <stdint.h>
#include <structures/list.h>
#include <structures/rbt.h>
#include <sync/spinlock.h>
#include <types/types.h>

struct address_range;

struct vas_range {
    vaddr_t start;
    size_t length;

    struct rbt_node node;
    struct list_head free_list_node;
};

struct vas_space {
    struct spinlock lock;
    struct rbt tree;
    vaddr_t base;
    vaddr_t limit;
    struct list_head freelist;
};

struct vas_space *vas_space_create(vaddr_t base, vaddr_t limit);
struct vas_space *vas_space_from_address_range(struct address_range *ar);
void vas_free(struct vas_space *vas, vaddr_t addr, size_t size);
vaddr_t vas_alloc(struct vas_space *vas, size_t size, size_t align);
struct vas_space *vas_space_bootstrap(vaddr_t base, vaddr_t limit);
struct vas_space *vas_space_bootstrap_internal(vaddr_t base, vaddr_t limit);
struct vas_space *vas_space_init_internal(vaddr_t base, vaddr_t limit);
void *vas_map(struct vas_space *vas, paddr_t paddr, size_t len, uint64_t flags,
              enum vmm_flags vflags);
void vas_unmap(struct vas_space *vas, void *vaddr, size_t len);
SPINLOCK_GENERATE_LOCK_UNLOCK_FOR_STRUCT(vas_space, lock);
