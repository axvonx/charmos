/* @title: Memory Arenas */
#pragma once
#include <err.h>
#include <mem/alloc_param.h>
#include <mem/arena_types.h>
#include <stddef.h>
#include <stdint.h>

/*
 * This subsystem allows the kernel to provide generic memory arenas that
 * explicitly declare their capabilities and accept generic parameters
 * and have extensible extra functions, for whatever purposes they desire.
 *
 * Broadly, everything (or almost everything) is built with substantial
 * room for extensibility, which allows for very different arenas that
 * all funnel through this API, which exists to enforce safety
 * and provide a unified substrate.
 *
 * The STRATEGY is the underlying allocation strategy, such as freelists,
 * bumps, etc. Strategies cannot arbitrarily change, and an explicit
 * strategy change function must be used (TODO: reconsider)
 *
 * Then, there are arena PARAMS and ATTRIBUTES. Parameters are arbitrary
 * opaque data that can be passed into arena allocators (meaning whatever),
 * whereas attributes are generic, named fields that can be interpreted
 * in a variety of ways (embedding parameters) to give arenas data like
 * size (in bytes), capacity, etc.
 *
 */

/* This is the opaque structure for arenas:
 *
 * Various arena allocator implementations use many different
 * mechanisms and components, with much of the structure being
 * merely for tracking metadata rather than any outward API-facing properties.
 */
struct arena;

/* arena_flags: 64 bit bitflags
 *
 * These flags denote the subset of globally recognized arena capabilities
 *
 * Upper 32 bits are available for whatever needs it in the arena
 * allocator internally
 *
 *      ┌─────────────────────────────────────────────────────────────┐
 * Bits │ 63--32 31..28 27..24 23..20 19..16 15..12 11..8  7..4  3..0 │
 * Use  │  AAAA   AAAA   AAAA   AAAA   AAAA   AAAA   AAAA  AAAA  AAAA │
 *      └─────────────────────────────────────────────────────────────┘
 *
 * A - Unused (available)
 * * - Unused (unavailable)
 *
 */
enum arena_flags : uint64_t {
    /* "tagging at the per-object granularity */
    ARENA_FLAG_TAGGED = 1 << 0,

    /* "stack trace at the per-object granularity" */
    ARENA_FLAG_TRACED = 1 << 1,

    /* Has a log site */
    ARENA_FLAG_LOGGED = 1 << 2,

    /* Use locks */
    ARENA_FLAG_LOCKED = 1 << 3,
};

/* There are 31 default arena policies (1-31, 0 is omitted
 * to avoid uninitialized issues and bugs), and 32
 * marks the point where anything at or greater than this
 * is a strategy registered by other code, not by default */
enum arena_strategy {
    /* Bump allocator */
    ARENA_STRATEGY_BUMP = 1,

    /* All allocations are the same size,
     * managed at initialization time */
    ARENA_STRATEGY_FIXED,

    ARENA_STRATEGY_FREELIST,

    /* TODO: this should ideally reuse
     * slab allocator code, which might
     * mean a little porting will need to happen.
     *
     * Hopefully not much, since this is ideally
     * just a wrapper around objects */
    ARENA_STRATEGY_SLAB,

    /* DIFFERENT. Notably significantly lighter-weight
     * than the full slab allocator */
    ARENA_STRATEGY_MINI_SLAB,

    ARENA_STRATEGY_BUDDY,

    /* Reserve allocators are explicitly fed memory in,
     * so they never perform an underlying allocation operation */
    ARENA_STRATEGY_RESERVE,

    ARENA_STRATEGY_MAX = 32
};

/* These are generally passed into the arena allocation creation
 * as a generic way of giving parameters. Notably NOT
 * arena_attributes, as these are merely the param words.
 *
 * Also usable in arena_alloc_special in substitution of the size
 * parameter, which is relevant to some allocators */
struct arena_params {
    void *ptr;
    union {
        struct {
            uint64_t param1;
            uint64_t param2;
            uint64_t param3;
            uint64_t param4;
        };

        uint64_t params[4];
    };
};

struct arena_attributes {
    /* For the underlying arena, to describe whatever */
    struct arena_params params;
};

/* The output of the extended arena functions */
struct arena_result {
    enum err err;
    struct arena_params params;
};

extern struct err_facility arena_err_facility;
void *arena_alloc_internal(struct arena *arena, size_t size, arena_tag_t tag,
                           struct alloc_params params) cw_alloc(2);

enum err arena_free_internal(struct arena *arena, void *ptr,
                             enum alloc_behavior bh) cc_warn_unused_result;

void *arena_alloc_special_internal(struct arena *arena,
                                   struct arena_params *params, arena_tag_t tag,
                                   struct alloc_params alloc_params) cw_alloc();
