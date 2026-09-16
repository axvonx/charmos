/* @title: Integer Arithmetic Functions */
#pragma once
#include <stddef.h>
#include <stdint.h>

static inline size_t gcd(size_t a, size_t b) {
    while (b) {
        size_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static inline size_t lcm(size_t a, size_t b) {
    return (a / gcd(a, b)) * b;
}

static inline size_t ipow(size_t base, int32_t exp) {
    size_t result = 1;
    while (exp > 0) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

#define isqrt(__n)                                                             \
    ({                                                                         \
        __typeof__(__n) __val = (__n);                                         \
        __typeof__(__n) __res = 0;                                             \
        if (__val > 0) {                                                       \
            __typeof__(__n) __x0 = __val / 2;                                  \
            if (__x0 == 0) {                                                   \
                __res = 1;                                                     \
            } else {                                                           \
                __typeof__(__n) __x1 = (__x0 + __val / __x0) / 2;              \
                while (__x1 < __x0) {                                          \
                    __x0 = __x1;                                               \
                    __x1 = (__x0 + __val / __x0) / 2;                          \
                }                                                              \
                __res = __x0;                                                  \
            }                                                                  \
        }                                                                      \
        __res;                                                                 \
    })
