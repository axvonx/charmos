/* @title: Memory Arenas */
#pragma once
#include <err.h>
#include <mem/alloc.h>
#include <stddef.h>
#include <stdint.h>

/*
 * This subsystem allows the kernel to provide generic memory arenas that
 * explicitly declare their capabilities and accept generic parameters
 * and have extensible extra functions, for whatever purposes they desire.
 *
 * Broadly, everything (or almost everything) is built with substantial
 * room for extensibility, which allows for very different arenas that
 * all funnel through this API, which exists at all to enforce safety
 * and provide a unified substrate.
 *
 * The STRATEGY is the underlying allocation strategy, such as freelists,
 * bumps, etc. Strategies cannot arbitrarily change, and an explicit
 * strategy change function must be used (TODO: reconsider)
 *
 * The STRATEGY has CAPABILITIES that state what exactly a strategy supports.
 * For instance, some strategies are thread safe, whereas other strategies
 * maybe don't support object-granularity deallocations, etc.
 *
 * Capabilities govern the errors that the arena allocator might return,
 * and the checks that it might perform around the allocator.
 *
 * For instance, if an allocator does not proclaim to be thread safe,
 * then the arena allocator, in debug mode, might insert checks
 * that verify that the caller is always a single thread,
 * and panic if any entry comes from another.
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

/* arena_strategy_flags: 64 bit bitflags
 *
 * The entire upper dword is available for whomever and whatever wants extra
 * flags to define.
 *      ┌─────────────────────────────────────────────────────────────┐
 * Bits │ 63..32 31..28 27..24 23..20 19..16 15..12 11..8  7..4  3..0 │
 * Use  │  A--A   AAAA   AAAA   AAAA   AAAA   AAAA   AAAA  AAAA  AAAA │
 *      └─────────────────────────────────────────────────────────────┘
 *
 * A - Unused (available)
 * * - Unused (unavailable)
 *
 */
enum arena_strategy_flags : uint64_t {
    /* NOTE: IRQ safety merely permits the irq_in_interrupt context to enter
     * the arena allocator. It does NOT enforce any safety, largely because
     * this is not possible in the ISR.
     *
     * IRQ safe arenas are not necessarily thread safe: one can imagine
     * a static struct arena *a; that is only ever accessed from within
     * an ISR, and only has one IRQ that could possibly access it from
     * that ISR. Such an arena is IRQ safe but not thread safe. */
    ARENA_STRATEGY_FLAG_IRQ_SAFE = 1 << 0,
    ARENA_STRATEGY_FLAG_THREAD_SAFE = 1 << 1,
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
     * than the full slab allocator, but not supporting
     * things like concurrency and locking */
    ARENA_STRATEGY_MINI_SLAB,

    ARENA_STRATEGY_BUDDY,

    ARENA_STRATEGY_RESERVE,

    ARENA_STRATEGY_MAX = 32
};

enum arena_tag { ARENA_TAG_NONE = 0 };

struct arena_strategy_capabilities {};

/* These are generally passed into the arena allocation creation
 * as a generic way of giving parameters. Notably NOT
 * arena_attributes, as these are merely the param words.
 *
 * Also usable in arena_alloc_special in substitution of the size
 * parameter, which is relevant to some allocators */

/* TODO: Revise and expand this past 4 arguments, making generic
 * void *arg and whatnot */
struct arena_params {
    void *arg;
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

extern struct err_facility arena_err_facility;
void *arena_alloc_internal(struct arena *arena, size_t size, enum arena_tag tag,
                           enum alloc_flags flags, enum alloc_behavior bh)
    cw_alloc(2);

enum err arena_free_internal(struct arena *arena, void *ptr,
                             enum alloc_behavior bh) cc_warn_unused_result;

void *arena_alloc_special_internal(struct arena *arena,
                                   struct arena_params *params,
                                   enum arena_tag tag, enum alloc_flags flags,
                                   enum alloc_behavior bh) cw_alloc();
