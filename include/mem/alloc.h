/* @title: General Purpose Memory Allocator API */
#pragma once
#include <compiler/core.h>
#include <compiler/wrapper.h>
#include <log.h>
#include <mem/alloc_api_internal.h>
#include <mem/alloc_param.h>
#include <types/types.h>

LOG_SITE_EXTERN(slab);
LOG_HANDLE_EXTERN(slab_flags);

/* Verify the contract */
#define ALLOC_PARAMS_CONTRACT(params)                                          \
    ct_precond(ALLOC_FLAGS_VALID((params).flags),                              \
               "alloc flags set unavailable bits")                             \
        ct_precond(ALLOC_BEHAVIOR_MAY_FAULT((params).behavior) ||              \
                       !ALLOC_FLAGS_CAN_FAULT((params).flags),                 \
                   "non-faulting alloc behavior cannot request pageable or "   \
                   "movable memory")                                           \
            ct_precond(!ALLOC_BEHAVIOR_IS_ISR_SAFE((params).behavior) ||       \
                           !((params).flags & ALLOC_FLAG_PAGEABLE),            \
                       "ISR-safe alloc behavior cannot request pageable "      \
                       "memory")

#define ALLOC_SIZE_CONTRACT(size)                                              \
    cc_diagnose_if((size) == 0, "zero-size allocation", "warning")

#define ALLOC_ALIGN_CONTRACT(align)                                            \
    ct_precond((align) != 0 && ((align) & ((align) - 1)) == 0,                 \
               "alignment must be a nonzero power of two")

void *cc_nullable valloc_new(size_t size, struct alloc_params params)
    cw_alloc(1) cc_must_check ALLOC_PARAMS_CONTRACT(params)
        ALLOC_SIZE_CONTRACT(size);
void vfree_new(void *ptr, enum alloc_behavior behavior);

void vfree_full(void *ptr, enum alloc_behavior behavior)
    cc_owner_takes(valloc, 1);
void vfree_defer_irq(void *ptr) cc_owner_takes(valloc, 1);
void vfree_aligned_full(void *ptr, enum alloc_behavior behavior)
    cc_owner_takes(valloc_aligned, 1);

void *cc_nullable valloc_from_domain(domain_id_t domain, size_t size)
    cw_alloc(2) cc_dealloc(vfree_full, 1) cc_dealloc(vfree_defer_irq, 1)
    cc_owner_returns(valloc, 2) cc_must_check;

void *cc_nullable valloc_full(size_t size, struct alloc_params params)
    cw_alloc(1) cc_dealloc(vfree_full, 1) cc_dealloc(vfree_defer_irq, 1)
    cc_owner_returns(valloc, 1) cc_must_check ALLOC_PARAMS_CONTRACT(params)
        ALLOC_SIZE_CONTRACT(size);

void *cc_nullable vrealloc_full(void *ptr, size_t size,
                                struct alloc_params params) cc_alloc_size(2)
cc_must_check ALLOC_PARAMS_CONTRACT(params);
size_t vsize(void *ptr);

void *cc_nullable valloc_aligned_full(size_t size, size_t align,
                                      struct alloc_params params) cw_alloc(1, 2)
    cc_dealloc(vfree_aligned_full, 1) cc_owner_returns(valloc_aligned, 1)
        cc_must_check ALLOC_PARAMS_CONTRACT(params) ALLOC_SIZE_CONTRACT(size)
            ALLOC_ALIGN_CONTRACT(align);

void *cc_nullable valloc_pages(size_t page_count, enum alloc_flags flags)
    cw_alloc() cc_must_check ct_precond(ALLOC_FLAGS_VALID(flags),
                                        "alloc flags set unavailable bits")
        ALLOC_SIZE_CONTRACT(page_count);
bool valloc_ptr_in_slab_validate(void *ptr);
