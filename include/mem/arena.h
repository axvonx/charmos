/* @title: Memory Arenas */
#pragma once
#include <mem/alloc.h>
#include <stddef.h>
#include <stdint.h>

/* This is the opaque structure for arenas:
 *
 * Various arena allocator implementations use many different
 * mechanisms and components, with much of the structure being
 * merely for tracking metadata rather than any outward API-facing properties.
 */
struct arena;

/* There are 31 default arena policies (1-31, 0 is omitted
 * to avoid uninitialized issues and bugs), and 32
 * marks the point where anything at or greater than this
 * is a policy registered by other code, not by default */
enum arena_policy { ARENA_POLICY_MAX = 32 };

enum arena_tag { ARENA_TAG_NONE = 0 };

void *arena_alloc_internal(struct arena *arena, size_t size, enum arena_tag tag,
                           enum alloc_flags flags, enum alloc_behavior bh)
    cw_alloc(2) cc_warn_unused_result;
