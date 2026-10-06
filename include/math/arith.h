/* @title: Integer Arithmetic Functions */
#pragma once
#include <compiler/core.h>
#include <stddef.h>
#include <stdint.h>

static inline cc_constfn size_t gcd(size_t a, size_t b) {
    while (b) {
        size_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static inline cc_constfn size_t lcm(size_t a, size_t b) {
    return (a / gcd(a, b)) * b;
}

static inline cc_constfn size_t ipow(size_t base, int32_t exp) {
    size_t result = 1;
    while (exp > 0) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

#define isqrt(n_)                                                              \
    ({                                                                         \
        typeof(n_) __isqrt_val = (n_);                                         \
        typeof(n_) __isqrt_res = 0;                                            \
        if (__isqrt_val > 0) {                                                 \
            typeof(n_) __isqrt_x0 = __isqrt_val / 2;                           \
            if (__isqrt_x0 == 0) {                                             \
                __isqrt_res = 1;                                               \
            } else {                                                           \
                typeof(n_) __isqrt_x1 =                                        \
                    (__isqrt_x0 + __isqrt_val / __isqrt_x0) / 2;               \
                while (__isqrt_x1 < __isqrt_x0) {                              \
                    __isqrt_x0 = __isqrt_x1;                                   \
                    __isqrt_x1 = (__isqrt_x0 + __isqrt_val / __isqrt_x0) / 2;  \
                }                                                              \
                __isqrt_res = __isqrt_x0;                                      \
            }                                                                  \
        }                                                                      \
        __isqrt_res;                                                           \
    })
