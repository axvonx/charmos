/* @title: Alignment and Rounding */
#pragma once

#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1ULL))
#define ALIGN_UP(x, align) (((x) + ((align) - 1ULL)) & ~((align) - 1ULL))
#define IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define ROUND_UP(x, y) (((x) + (y) - 1) / (y))
#define ROUND_DOWN(x, y) ((x) / (y))
#define ROUND(x, y) (((x) + ((y) / 2)) / (y))
#define ROUND_UP_TO_POWER_OF_2(x) (1 << (32 - __builtin_clz(x - 1)))
#define ROUND_DOWN_TO_POWER_OF_2(x) (1 << (31 - __builtin_clz(x)))
