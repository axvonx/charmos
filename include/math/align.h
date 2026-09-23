/* @title: Alignment and Rounding */
#pragma once
#include <compiler/core.h>
#include <compiler/intrinsic.h>
#include <kassert.h>
#include <stdbool.h>
#include <stdint.h>

/* Use UNCHECKED and WRAPPING when callers don't want to panic and want to
 * validate input, whereas the defaults panic */

#define _ALIGN_CAPTURE_UNCHECKED(x, align)                                     \
    __auto_type __x = (x);                                                     \
    __auto_type __source_align = (align);                                      \
    ct_typecheck_integer_as(__x, x);                                           \
    ct_typecheck_widenable_to(__x, align);                                     \
    __typeof__(__x) __align = (__typeof__(__x)) __source_align

#define _ALIGN_CAPTURE(x, align)                                               \
    _ALIGN_CAPTURE_UNCHECKED(x, align);                                        \
    (void) kassert(__align > 0 && (__align & (__align - 1)) == 0)

#define IS_POW2(x)                                                             \
    ({                                                                         \
        __auto_type __p2 = (x);                                                \
        ct_typecheck_integer_as(__p2, x);                                      \
        __p2 > 0 && (__p2 & (__p2 - 1)) == 0;                                  \
    })

#define ALIGN_DOWN(x, align)                                                   \
    ({                                                                         \
        _ALIGN_CAPTURE(x, align);                                              \
        __x & ~(__align - 1);                                                  \
    })

#define _ALIGN_UP_CAPTURE(x, align)                                            \
    _ALIGN_CAPTURE_UNCHECKED(x, align);                                        \
    __typeof__((__x) + 0) __mask = __align - 1;                                \
    __typeof__((__x) + 0) __sum;                                               \
    bool __overflow = ci_add_overflow(__x, __mask, &__sum)

/* Rounds up without validating the alignment, wraps on overflow */
#define ALIGN_UP_WRAPPING(x, align)                                            \
    ({                                                                         \
        _ALIGN_UP_CAPTURE(x, align);                                           \
        (void) __overflow;                                                     \
        (__typeof__(__x)) (__sum & ~__mask);                                   \
    })

#define ALIGN_UP(x, align)                                                     \
    ({                                                                         \
        _ALIGN_UP_CAPTURE(x, align);                                           \
        (void) kassert(__align > 0 && (__align & (__align - 1)) == 0 &&        \
                       !__overflow);                                           \
        (__typeof__(__x)) (__sum & ~__mask);                                   \
    })

#define IS_ALIGNED(x, align)                                                   \
    ({                                                                         \
        _ALIGN_CAPTURE(x, align);                                              \
        (__x & (__align - 1)) == 0;                                            \
    })

#define IS_ALIGNED_UNCHECKED(x, align)                                         \
    ({                                                                         \
        _ALIGN_CAPTURE_UNCHECKED(x, align);                                    \
        __align > 0 && (__align & (__align - 1)) == 0 &&                       \
            (__x & (__align - 1)) == 0;                                        \
    })

/* TODO: This only operates on unsigned values, I do want signed
 * support, so I might need to write that out */
#define DIV_ROUND_UP(n, d)                                                     \
    ({                                                                         \
        __auto_type __n = (n);                                                 \
        __auto_type __source_d = (d);                                          \
        ct_typecheck_unsigned_as(__n, n);                                      \
        ct_typecheck_widenable_to(__n, d);                                     \
        __typeof__(__n) __d = (__typeof__(__n)) __source_d;                    \
        (void) kassert(__d > 0);                                               \
        (__typeof__(__n)) (__n / __d + (__n % __d != 0));                      \
    })
