/* @title: Bit Manipulation */
#pragma once
#define BIT(n) (1ull << (n))

#define BIT_SET(val, n) ((val) | BIT(n))
#define BIT_CLEAR(val, n) ((val) & ~BIT(n))
#define BIT_TEST(val, n) (((val) >> (n)) & 1ull)
#define BIT_TOGGLE(val, n) ((val) ^ BIT(n))

#define BIT_RANGE(val, lo, hi)                                                 \
    (((val) >> (lo)) & ((1ull << ((hi) - (lo) + 1ull)) - 1ull))

#define BIT_MASK(lo, hi) (((1ull << ((hi) - (lo) + 1ull)) - 1ull) << (lo))

#define SET_FIELD(val, field_val, lo, hi)                                      \
    (((val) & ~BIT_MASK(lo, hi)) | (((field_val) << (lo)) & BIT_MASK(lo, hi)))

#define BIT_ANY(val, mask) (((val) & (mask)) != 0)
#define BIT_ALL(val, mask) (((val) & (mask)) == (mask))

/* Count of bits in a range */
#define BIT_WIDTH(lo, hi) ((hi) - (lo) + 1u)
