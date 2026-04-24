/* @title: Higher Half Direct Mapping */
#pragma once
#include <global.h>
#include <types/types.h>

static inline vaddr_t hhdm_paddr_to_vaddr(paddr_t p) {
    return p + global.hhdm_offset;
}

static inline paddr_t hhdm_vaddr_to_paddr(vaddr_t v) {
    return v - global.hhdm_offset;
}

static inline paddr_t hhdm_ptr_to_paddr(const void *ptr) {
    return hhdm_vaddr_to_paddr((vaddr_t) ptr);
}

static inline void *hhdm_paddr_to_ptr(paddr_t p) {
    return (void *) hhdm_paddr_to_vaddr(p);
}
