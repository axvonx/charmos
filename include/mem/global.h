/* @title: Memory Globals */
#pragma once
#include <atomic.h>
#include <types/types.h>

struct domain_buddy;
struct page;

struct mem_globals {
    vaddr_t hhdm_offset;
    atomic_uint64_t pt_epoch;

    struct page *page_array;
    uint64_t total_pages;
    pfn_t last_pfn;

    struct domain_buddy *domain_buddies;
};

extern struct mem_globals mem_global;
