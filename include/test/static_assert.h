/* @title: Static Assertions & Compile-Time Checks */
#pragma once
#include <compiler.h>
#include <stddef.h>
#include <stdint.h>

#define static_assert_size(type, expected_size)                                \
    ct_assert_size(type, expected_size)
#define static_assert_align(type, expected_align)                              \
    ct_assert_align(type, expected_align)
#define static_assert_offset(type, member, expected_offset)                    \
    ct_assert_offset(type, member, expected_offset)

#define ct_assert_disjoint_masks(mask1, mask2)                                 \
    _Static_assert(((mask1) & (mask2)) == 0,                                   \
                   "masks " #mask1 " and " #mask2 " overlap")

#define static_assert_disjoint_masks(mask1, mask2)                             \
    ct_assert_disjoint_masks(mask1, mask2)
