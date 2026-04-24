/* @title: Slab Allocator Hint Mechanism */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum slab_hint_type {
    SLAB_HINT_ALLOCS_SOON,
    SLAB_HINT_FREES_SOON,
};

enum slab_hint_error { SLAB_HINT_OK };

struct slab_hint_param {
    uint64_t param1, param2, param3, param4;
};

enum slab_hint_error slab_hint(enum slab_hint_type, struct slab_hint_param);
