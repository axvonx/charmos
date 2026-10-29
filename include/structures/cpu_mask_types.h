/* @title: CPU Mask Types */
#pragma once
#include <structures/bitmap_types.h>

#ifndef CPU_MASK_BITS
#define CPU_MASK_BITS 128
#endif

#define CPU_MASK_WORD_BITS 64
#define CPU_MASK_WORDS BITMAP_WORDS_CONST(CPU_MASK_BITS)

struct cpu_mask {
    BITMAP_DECLARE(bits, CPU_MASK_BITS);
};
