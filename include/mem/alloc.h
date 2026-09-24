/* @title: General Purpose Memory Allocator API */
#pragma once
#include <compiler/core.h>
#include <compiler/wrapper.h>
#include <mem/alloc_param.h>
#include <stddef.h>
#include <stdint.h>

LOG_SITE_EXTERN(slab);
LOG_HANDLE_EXTERN(slab_flags);

void *kmalloc_new(size_t size, struct alloc_params params) cw_alloc(1)
    cc_warn_unused_result;
void kfree_new(void *ptr, enum alloc_behavior behavior);

void *kmalloc_from_domain(domain_id_t domain, size_t size) cw_alloc(2)
    cc_warn_unused_result;

void *kmalloc_internal(size_t size, struct alloc_params params) cw_alloc(1)
    cc_warn_unused_result;

void *krealloc_internal(void *ptr, size_t size, struct alloc_params params)
    cc_alloc_size(2) cc_warn_unused_result;
void kfree_internal(void *ptr, enum alloc_behavior behavior);
size_t ksize(void *ptr);

void *kmalloc_aligned_internal(size_t size, size_t align,
                               struct alloc_params params) cw_alloc(1, 2)
    cc_warn_unused_result;
void kfree_aligned_internal(void *ptr, enum alloc_behavior behavior);
void kfree_defer_irq(void *ptr);

void *kmalloc_pages(size_t n_pages, enum alloc_flags flags) cw_alloc()
    cc_warn_unused_result;
bool kmalloc_ptr_in_slab_validate(void *ptr);
