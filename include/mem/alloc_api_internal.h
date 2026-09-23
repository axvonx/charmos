/* @title: Allocation Macros */
#pragma once
#define kfree_1(ptr) kfree_internal((ptr), ALLOC_BEHAVIOR_DEFAULT)
#define kfree_2(ptr, bh) kfree_internal((ptr), (bh))

#define kfree(...) PP_CALL(kfree, __VA_ARGS__)

#define kmalloc_1(sz)                                                          \
    kmalloc_internal(                                                          \
        (sz), ((struct alloc_params) {.flags = ALLOC_FLAGS_DEFAULT,            \
                                      .behavior = ALLOC_BEHAVIOR_DEFAULT}))
#define kmalloc_2(sz, fl)                                                      \
    kmalloc_internal(                                                          \
        (sz), ((struct alloc_params) {.flags = (fl),                           \
                                      .behavior = ALLOC_BEHAVIOR_DEFAULT}))
#define kmalloc_3(sz, fl, bh)                                                  \
    kmalloc_internal(                                                          \
        (sz), ((struct alloc_params) {.flags = (fl), .behavior = (bh)}))
#define kmalloc(...) PP_CALL(kmalloc, __VA_ARGS__)

#define kmalloc_aligned_2(sz, al)                                              \
    kmalloc_aligned_internal(                                                  \
        (sz), (al),                                                            \
        ((struct alloc_params) {.flags = ALLOC_FLAGS_DEFAULT,                  \
                                .behavior = ALLOC_BEHAVIOR_DEFAULT}))
#define kmalloc_aligned_3(sz, al, fl)                                          \
    kmalloc_aligned_internal(                                                  \
        (sz), (al),                                                            \
        ((struct alloc_params) {.flags = (fl),                                 \
                                .behavior = ALLOC_BEHAVIOR_DEFAULT}))
#define kmalloc_aligned_4(sz, al, fl, bh)                                      \
    kmalloc_aligned_internal(                                                  \
        (sz), (al), ((struct alloc_params) {.flags = (fl), .behavior = (bh)}))
#define kmalloc_aligned(...) PP_CALL(kmalloc_aligned, __VA_ARGS__)

#define kfree_aligned_1(ptr)                                                   \
    kfree_aligned_internal((ptr), ALLOC_BEHAVIOR_DEFAULT)
#define kfree_aligned_2(ptr, bh) kfree_aligned_internal((ptr), (bh))
#define kfree_aligned(...) PP_CALL(kfree_aligned, __VA_ARGS__)

#define krealloc_2(ptr, sz)                                                    \
    krealloc_internal(                                                         \
        (ptr), (sz),                                                           \
        ((struct alloc_params) {.flags = ALLOC_FLAGS_DEFAULT,                  \
                                .behavior = ALLOC_BEHAVIOR_DEFAULT}))
#define krealloc_3(ptr, sz, fl)                                                \
    krealloc_internal((ptr), (sz),                                             \
                      ((struct alloc_params) {                                 \
                          .flags = (fl), .behavior = ALLOC_BEHAVIOR_DEFAULT}))
#define krealloc_4(ptr, sz, fl, bh)                                            \
    krealloc_internal(                                                         \
        (ptr), (sz),                                                           \
        ((struct alloc_params) {.flags = (fl), .behavior = (bh)}))
#define krealloc(...) PP_CALL(krealloc, __VA_ARGS__)

#define knew(ptr, ...) ((ptr) = kmalloc(sizeof(*(ptr)), ##__VA_ARGS__))
