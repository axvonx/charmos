/* @title: Memory Arena Types */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <types/types.h>

struct arena_result;
struct arena_params;
typedef uint32_t arena_tag_t;
typedef struct arena_result (*arena_ext_fn_t)(struct arena_params *p);
