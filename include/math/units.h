/* @title: Memory Sizes and Unit Conversions */
#pragma once
#include <stddef.h>

#define KB(x) ((size_t) (x) << 10)
#define MB(x) ((size_t) (x) << 20)
#define GB(x) ((size_t) (x) << 30)
#define TB(x) ((size_t) (x) << 40)

static inline size_t to_bits(size_t bytes) {
    return bytes * 8;
}

static inline size_t to_bytes(size_t bits) {
    return (bits + 7) / 8;
}
