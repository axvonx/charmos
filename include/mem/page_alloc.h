/* @title: Page Allocation */
#pragma once
#include <compiler/core.h>
#include <mem/alloc.h>
#include <stddef.h>
#include <stdint.h>

#define ALLOC_FLAG_EX_PG_CONTIGUOUS ALLOC_FLAG_AVAIL_BIT(0)

#define page_alloc(n_pages, ...)                                               \
    page_alloc_internal((n_pages), alloc_params_with_defaults(                 \
                                       ALLOC_FLAGS_NONE,                       \
                                       ALLOC_BEHAVIOR_NORMAL, ##__VA_ARGS__))

#define page_alloc_demand(n_pages, ...)                                        \
    page_alloc_demand_internal(                                                \
        (n_pages),                                                             \
        alloc_params_with_defaults(ALLOC_FLAGS_NONE, ALLOC_BEHAVIOR_NORMAL,    \
                                   ##__VA_ARGS__))

#define page_free_2(ptr, n_pages)                                              \
    page_free_internal((ptr), (n_pages), ALLOC_BEHAVIOR_NORMAL)
#define page_free_3(ptr, n_pages, bh) page_free_internal((ptr), (n_pages), (bh))

#define page_free(...) PP_CALL(page_free, __VA_ARGS__)

void *page_alloc_internal(size_t n_pages, struct alloc_params params)
    cw_alloc();
void *page_alloc_demand_internal(size_t n_pages, struct alloc_params params)
    cw_alloc();
void page_free_internal(void *ptr, size_t n_pages,
                        enum alloc_behavior behavior);
bool page_alloc_vaddr_in_vas(vaddr_t vaddr);
void page_alloc_init();
