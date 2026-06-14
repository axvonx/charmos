/* @title: VMM Mapping Macros */
#pragma once
#include <compiler.h>

/* Domain-first overloads for the page mapping API.
 *
 * Each "domain" whose signature genuinely differs (kernel, user, demand, ...)
 * is its own first-class _internal function. The two trailing knobs, by
 * contrast, are defaulted: vmm_flags defaults to VMM_FLAG_NONE and page size
 * to 4KB, so the common case stays terse and a caller appends one or both only
 * when it needs to (mirroring alloc_api_internal.h's flags/behavior tail). The
 * order is (..., vflags, size): supply vflags to reach size. */

/* vmm_map_page(virt, phys, flags[, vflags[, size]]) */
#define vmm_map_page_3(v, p, f)                                                \
    vmm_map_page_internal((v), (p), (f), VMM_FLAG_NONE, VMM_MAP_PAGE_SIZE_4KB)
#define vmm_map_page_4(v, p, f, vf)                                            \
    vmm_map_page_internal((v), (p), (f), (vf), VMM_MAP_PAGE_SIZE_4KB)
#define vmm_map_page_5(v, p, f, vf, sz)                                        \
    vmm_map_page_internal((v), (p), (f), (vf), (sz))
#define vmm_map_page(...)                                                      \
    _DISPATCH(vmm_map_page, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

/* vmm_map_page_user(pml4, virt, phys, flags[, vflags[, size]]) */
#define vmm_map_page_user_4(pml4, v, p, f)                                     \
    vmm_map_page_user_internal((pml4), (v), (p), (f), VMM_FLAG_NONE,           \
                               VMM_MAP_PAGE_SIZE_4KB)
#define vmm_map_page_user_5(pml4, v, p, f, vf)                                 \
    vmm_map_page_user_internal((pml4), (v), (p), (f), (vf),                    \
                               VMM_MAP_PAGE_SIZE_4KB)
#define vmm_map_page_user_6(pml4, v, p, f, vf, sz)                             \
    vmm_map_page_user_internal((pml4), (v), (p), (f), (vf), (sz))
#define vmm_map_page_user(...)                                                 \
    _DISPATCH(vmm_map_page_user, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

/* vmm_unmap_page(virt[, vflags[, size]]) */
#define vmm_unmap_page_1(v)                                                    \
    vmm_unmap_page_internal((v), VMM_FLAG_NONE, VMM_MAP_PAGE_SIZE_4KB)
#define vmm_unmap_page_2(v, vf)                                                \
    vmm_unmap_page_internal((v), (vf), VMM_MAP_PAGE_SIZE_4KB)
#define vmm_unmap_page_3(v, vf, sz) vmm_unmap_page_internal((v), (vf), (sz))
#define vmm_unmap_page(...)                                                    \
    _DISPATCH(vmm_unmap_page, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

/* vmm_get_leaf_pte(virt[, vflags]) - vflags defaults to VMM_FLAG_NONE */
#define vmm_get_leaf_pte_1(v) vmm_get_leaf_pte_internal((v), VMM_FLAG_NONE)
#define vmm_get_leaf_pte_2(v, vf) vmm_get_leaf_pte_internal((v), (vf))
#define vmm_get_leaf_pte(...)                                                  \
    _DISPATCH(vmm_get_leaf_pte, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

/* vmm_map_bump(addr, len, flags[, vflags]) */
#define vmm_map_bump_3(a, l, f)                                                \
    vmm_map_bump_internal((a), (l), (f), VMM_FLAG_NONE)
#define vmm_map_bump_4(a, l, f, vf) vmm_map_bump_internal((a), (l), (f), (vf))
#define vmm_map_bump(...)                                                      \
    _DISPATCH(vmm_map_bump, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

/* vmm_mark_demand_page(virt, flags[, size]) */
#define vmm_mark_demand_page_2(v, f)                                           \
    vmm_mark_demand_page_internal((v), (f), VMM_MAP_PAGE_SIZE_4KB)
#define vmm_mark_demand_page_3(v, f, sz)                                       \
    vmm_mark_demand_page_internal((v), (f), (sz))
#define vmm_mark_demand_page(...)                                              \
    _DISPATCH(vmm_mark_demand_page, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

/* vmm_mark_demand_page_user(pml4, virt, flags[, size]) */
#define vmm_mark_demand_page_user_3(pml4, v, f)                                \
    vmm_mark_demand_page_user_internal((pml4), (v), (f), VMM_MAP_PAGE_SIZE_4KB)
#define vmm_mark_demand_page_user_4(pml4, v, f, sz)                            \
    vmm_mark_demand_page_user_internal((pml4), (v), (f), (sz))
#define vmm_mark_demand_page_user(...)                                         \
    _DISPATCH(vmm_mark_demand_page_user, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

/* vmm_map_demand_page(virt, phys, flags[, size]) */
#define vmm_map_demand_page_3(v, p, f)                                         \
    vmm_map_demand_page_internal((v), (p), (f), VMM_MAP_PAGE_SIZE_4KB)
#define vmm_map_demand_page_4(v, p, f, sz)                                     \
    vmm_map_demand_page_internal((v), (p), (f), (sz))
#define vmm_map_demand_page(...)                                               \
    _DISPATCH(vmm_map_demand_page, PP_NARG(__VA_ARGS__))(__VA_ARGS__)
