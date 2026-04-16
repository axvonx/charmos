/* @title: Power of (integer) */
#pragma once
#include <stddef.h>
#include <stdint.h>

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
