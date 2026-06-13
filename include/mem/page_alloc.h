/* @title: Page Allocation */
#pragma once
#include <compiler.h>
#include <mem/alloc.h>
#include <stddef.h>
#include <stdint.h>

#define page_alloc_1(n_pages)                                                  \
    page_alloc_internal((n_pages), ALLOC_FLAGS_NONE, ALLOC_BEHAVIOR_NORMAL)
#define page_alloc_2(n_pages, flags)                                           \
    page_alloc_internal((n_pages), (flags), ALLOC_BEHAVIOR_NORMAL)
#define page_alloc_3(n_pages, flags, bh)                                       \
    page_alloc_internal((n_pages), (flags), (bh))

#define page_alloc(...) _DISPATCH(page_alloc, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

#define page_free_2(ptr, n_pages)                                              \
    page_free_internal((ptr), (n_pages), ALLOC_BEHAVIOR_NORMAL)
#define page_free_3(ptr, n_pages, bh) page_free_internal((ptr), (n_pages), (bh))

#define page_free(...) _DISPATCH(page_free, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

void *page_alloc_internal(size_t n_pages, enum alloc_flags flags,
                          enum alloc_behavior bh);
void page_free_internal(void *ptr, size_t n_pages,
                        enum alloc_behavior behavior);
bool page_alloc_vaddr_in_vas(vaddr_t vaddr);
void page_alloc_init();
