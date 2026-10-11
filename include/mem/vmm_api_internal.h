/* @title: VMM Mapping Macros */
#pragma once
#include <compiler/diagnostic.h>

/* Every API is (positional operands..., .designated = overrides...) */
#define vmm_request_with_defaults(...)                                         \
    cc_wno_override_init_expr(                                                 \
        struct vmm_request,                                                    \
        ((struct vmm_request) {.page_flags = PAGE_NO_FLAGS,                    \
                               .vmm_flags = VMM_FLAG_NONE,                     \
                               .page_size = VMM_MAP_PAGE_SIZE_4KB,             \
                               .tlb_mode = TLB_MODE_SYNC,                      \
                               ##__VA_ARGS__}))

/* vmm_map_page(virt, phys[, .page_flags, .vmm_flags, .page_size, .tlb_mode]) */
#define vmm_map_page(v, p, ...)                                                \
    vmm_map_page_full((v), (p), vmm_request_with_defaults(__VA_ARGS__))

#define vmm_map_page_user(pml4, v, p, ...)                                     \
    vmm_map_page_user_full((pml4), (v), (p),                                   \
                           vmm_request_with_defaults(__VA_ARGS__))

#define vmm_unmap_page(v, ...)                                                 \
    vmm_unmap_page_full((v), vmm_request_with_defaults(__VA_ARGS__))

#define vmm_get_phys(v, ...)                                                   \
    vmm_get_phys_full((v), vmm_request_with_defaults(__VA_ARGS__))

#define vmm_get_leaf_pte(v, ...)                                               \
    vmm_get_leaf_pte_full((v), vmm_request_with_defaults(__VA_ARGS__))

/* Multi-page helpers: .page_flags are OR'd onto PRESENT | WRITE */
#define vmm_map(paddr, vaddr, len, ...)                                        \
    vmm_map_full((paddr), (vaddr), (len),                                      \
                 vmm_request_with_defaults(__VA_ARGS__))

#define vmm_unmap(addr, len, ...)                                              \
    vmm_unmap_full((addr), (len), vmm_request_with_defaults(__VA_ARGS__))

#define vmm_map_bump(addr, len, ...)                                           \
    vmm_map_bump_full((addr), (len), vmm_request_with_defaults(__VA_ARGS__))

#define vmm_unmap_all_user_pages(pml4, ...)                                    \
    vmm_unmap_all_user_pages_full((pml4),                                      \
                                  vmm_request_with_defaults(__VA_ARGS__))

#define vmm_map_aliased(v, len, p, ...)                                        \
    vmm_map_aliased_full((v), (len), (p),                                      \
                         vmm_request_with_defaults(__VA_ARGS__))

/* leaf size to unshare down to is .page_size */
#define vmm_unshare_path(v, ...)                                               \
    vmm_unshare_path_full((v), vmm_request_with_defaults(__VA_ARGS__))

/* Demand pages take demand_page_flags positionally */
#define vmm_mark_demand_page(v, dflags, ...)                                   \
    vmm_mark_demand_page_full((v), (dflags),                                   \
                              vmm_request_with_defaults(__VA_ARGS__))

#define vmm_mark_demand_page_user(pml4, v, dflags, ...)                        \
    vmm_mark_demand_page_user_full((pml4), (v), (dflags),                      \
                                   vmm_request_with_defaults(__VA_ARGS__))

#define vmm_map_demand_page(v, p, dflags, ...)                                 \
    vmm_map_demand_page_full((v), (p), (dflags),                               \
                             vmm_request_with_defaults(__VA_ARGS__))
