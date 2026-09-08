/* @title: Integer Square Root */
#pragma once
#include <stddef.h>
#include <stdint.h>

#define isqrt(__n)                                                             \
    ({                                                                         \
        if (__n == 0)                                                          \
            0;                                                                 \
                                                                               \
        __typeof__(__n) x0 = __n / 2;                                          \
        if (x0 == 0)                                                           \
            1;                                                                 \
                                                                               \
        __typeof__(__n) x1 = (x0 + n / x0) / 2;                                \
        while (x1 < x0) {                                                      \
            x0 = x1;                                                           \
            x1 = (x0 + n / x0) / 2;                                            \
        }                                                                      \
        x0;                                                                    \
    })
