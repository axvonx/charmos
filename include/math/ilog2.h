/* @title: ilog2 */
#pragma once
#include <stddef.h>
#include <stdint.h>

static inline uint8_t ilog2(uint64_t x) {
    uint8_t r = 0;
    while (x >>= 1)
        r++;
    return r;
}
