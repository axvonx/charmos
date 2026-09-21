/* @title: Memory Sizes and Unit Conversions */
#pragma once
#include <stddef.h>

/* Binary (IEC) units: powers of 1024 */
#define KIB(x) ((size_t) (x) << 10)
#define MIB(x) ((size_t) (x) << 20)
#define GIB(x) ((size_t) (x) << 30)
#define TIB(x) ((size_t) (x) << 40)

/* Decimal (SI) units: powers of 1000 */
#define KB(x) ((size_t) (x) * 1000ULL)
#define MB(x) ((size_t) (x) * 1000000ULL)
#define GB(x) ((size_t) (x) * 1000000000ULL)
#define TB(x) ((size_t) (x) * 1000000000000ULL)

static inline size_t to_bits(size_t bytes) {
    return bytes * 8;
}

static inline size_t to_bytes(size_t bits) {
    return (bits + 7) / 8;
}
