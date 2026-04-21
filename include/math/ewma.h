/* @title: Exponentially Weighted Moving Averages */
#pragma once
#include <math/fixed.h>
#include <stddef.h>
#include <stdint.h>

struct ewma {
    fx32_32_t alpha;
    size_t saved;
    fx32_32_t ewma;
};

static inline fx32_32_t ewma_update(struct ewma *ewma, size_t new) {
    fx32_32_t p1 = fx_mul(fx_from_int(ewma->saved), (FX_ONE - ewma->alpha));
    fx32_32_t p2 = fx_mul(fx_from_int(new), ewma->alpha);
    return (ewma->ewma = p1 + p2);
}
