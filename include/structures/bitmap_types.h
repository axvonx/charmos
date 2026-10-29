/* @title: Bitmap Types */
#pragma once
#include <types/types.h>

typedef uint64_t bitmap_word_t;

#define BITMAP_BITS_PER_WORD 64

/* Integer constant expression variant */
#define BITMAP_WORDS_CONST(nbits)                                              \
    (((nbits) / BITMAP_BITS_PER_WORD) + !!((nbits) % BITMAP_BITS_PER_WORD))
#define BITMAP_DECLARE(name, nbits)                                            \
    bitmap_word_t name[BITMAP_WORDS_CONST(nbits)]
