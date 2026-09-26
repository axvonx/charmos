/* @title: Allocation Macros */
#pragma once
#include <compiler/diagnostic.h>

#define alloc_params_with_defaults(_flags, _behavior, ...)                     \
    cc_wno_override_init_expr(                                                 \
        struct alloc_params,                                                   \
        ((struct alloc_params) {.flags = (_flags),                             \
                                .behavior = (_behavior),                       \
                                .priority = ALLOC_PRIORITY_DEFAULT,            \
                                ##__VA_ARGS__}))

#define kfree_1(ptr) kfree_full((ptr), ALLOC_BEHAVIOR_DEFAULT)
#define kfree_2(ptr, bh) kfree_full((ptr), (bh))

#define kfree(...) PP_CALL(kfree, __VA_ARGS__)

#define kmalloc(size, ...)                                                     \
    kmalloc_full((size), alloc_params_with_defaults(ALLOC_FLAGS_DEFAULT,       \
                                                    ALLOC_BEHAVIOR_DEFAULT,    \
                                                    ##__VA_ARGS__))

#define kmalloc_aligned(size, align, ...)                                      \
    kmalloc_aligned_full((size), (align),                                      \
                         alloc_params_with_defaults(ALLOC_FLAGS_DEFAULT,       \
                                                    ALLOC_BEHAVIOR_DEFAULT,    \
                                                    ##__VA_ARGS__))

#define kfree_aligned_1(ptr) kfree_aligned_full((ptr), ALLOC_BEHAVIOR_DEFAULT)
#define kfree_aligned_2(ptr, bh) kfree_aligned_full((ptr), (bh))
#define kfree_aligned(...) PP_CALL(kfree_aligned, __VA_ARGS__)

#define krealloc(ptr, size, ...)                                               \
    krealloc_full((ptr), (size),                                               \
                  alloc_params_with_defaults(ALLOC_FLAGS_DEFAULT,              \
                                             ALLOC_BEHAVIOR_DEFAULT,           \
                                             ##__VA_ARGS__))

#define knew(ptr, ...) ((ptr) = kmalloc(sizeof(*(ptr)), ##__VA_ARGS__))
