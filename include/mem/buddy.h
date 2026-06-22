/* @title: Buddy allocator */
#pragma once
#include <mem/alloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

#define MAX_ORDER 23
#define BUDDY_FREE_AREA_HASH_TABLE_SIZE 512
#define BUDDY_FREE_AREA_BITMAP_SIZE (BUDDY_FREE_AREA_HASH_TABLE_SIZE / 64)

struct page;
struct buddy_page;
struct limine_memmap_entry;

/* Protected under the domain_buddy spinlock */
struct buddy_hash_chain {
    struct buddy_page *tail; /* tail->next == head */
};

struct buddy_hash_table {
    struct buddy_hash_chain chains[BUDDY_FREE_AREA_HASH_TABLE_SIZE];
    uint64_t zeroed_hc_bitmap[BUDDY_FREE_AREA_BITMAP_SIZE];
    uint64_t nonzero_hc_bitmap[BUDDY_FREE_AREA_BITMAP_SIZE];
};

/* This is a hashmap where the hash head data takes up a page
 * and the counter word goes just over that page */
struct buddy_free_area {
    struct buddy_hash_table hash_table;
    uint64_t nr_free;
};

void buddy_add_to_free_area(struct buddy_page *page,
                            struct buddy_free_area *area);
struct buddy_page *buddy_remove_from_free_area(struct buddy_free_area *area);
paddr_t buddy_alloc_pages_global(size_t count, enum alloc_flags flags);
void buddy_free_pages_global(paddr_t addr, uint64_t count);
void buddy_add_entry(struct page *page_array, struct limine_memmap_entry *entry,
                     struct buddy_free_area *farea);
void buddy_reserve_range(uint64_t pfn, uint64_t pages);

paddr_t buddy_alloc_pages(struct buddy_free_area *free_area, size_t count);
void buddy_free_pages(paddr_t addr, size_t count,
                      struct buddy_free_area *free_area, size_t total_pages);
void buddy_init(void);
