/* @title: Allocator Parameters */
#pragma once
#include <compiler/core.h>
#include <compiler/wrapper.h>
#include <console/printf.h>
#include <log.h>
#include <mem/alloc_api_internal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * TL;DR:
 *
 *   FLAGS: TELL THE ALLOCATOR WHAT
 *          KIND OF MEMORY YOU WANT
 *
 *   BEHAVIORS:  TELL THE ALLOCATOR WHAT IT IS ALLOWED
 *               TO DO TO GET THAT MEMORY
 *
 *   PRIORITIES: TELL THE ALLOCATOR HOW MUCH YOU
 *               WANT YOUR ALLOCATION TO SUCCEED
 */

/* ─────────────────────────── ALLOC FLAGS ─────────────────────────── */

#define ALLOC_LOCALITY_SHIFT 25
#define ALLOC_CLASS_SHIFT 28
#define ALLOC_CLASS_MASK 0xF

/* The larger the locality, the closer it must be */
#define ALLOC_LOCALITY_MAX 7
#define ALLOC_LOCALITY_MIN 0
#define ALLOC_LOCALITY_MASK 0x7

#define ALLOC_LOCALITY_FROM_FLAGS(flags)                                       \
    (((flags) >> ALLOC_LOCALITY_SHIFT) & ALLOC_LOCALITY_MASK)

#define ALLOC_LOCALITY_TO_FLAGS(locality)                                      \
    (((locality) & ALLOC_LOCALITY_MASK) << ALLOC_LOCALITY_SHIFT)

#define ALLOC_FLAG_TEST(flags, mask) (flags & mask)
#define ALLOC_FLAG_CLASS(flags)                                                \
    ((flags >> ALLOC_CLASS_SHIFT) & ALLOC_CLASS_MASK)

/* Bits 16..23 are available, this gives the 0-indexed Nth available bit.
 *
 * Do note that this is a compile-time constant, not an expression, because
 * it should be usable in enum definitions and other places, thus,
 * bounds safety is not enforceable (unfortunate) */
#define ALLOC_FLAG_AVAIL_BIT(n) (1 << (ALLOC_CLASS_SHIFT - 4 + (n)))

#define ALLOC_FLAG_EX_JOIN(subsys, flag) ALLOC_FLAG_EX_##subsys##_##flag
#define ALLOC_FLAG_EX(subsys, flag) ALLOC_FLAG_EX_JOIN(subsys, flag)

/* alloc_flags: 32 bit bitflags
 *
 *      ┌──────────────────────────────────────────────────────┐
 * Bits │ 31..28 27..24 23..20 19..16 15..12 11..8  7..4  3..0 │
 * Use  │  %%%%   ###*   AAAA   AAAA   ****   ****  ***Z  MPFC │
 *      └──────────────────────────────────────────────────────┘
 *
 * C - "Prefer cache alignment"
 *
 * F - "Allow flexible NUMA locality"
 *
 * P - "Allow memory to be pageable"
 *
 * M - "Allow memory to be movable"
 *
 * Z - Zero on alloc, memory is zeroed
 *
 * ### - Locality bits
 *
 * %%%% - Allocation class bits
 *
 * A - Unused (available)
 * * - Unused (unavailable)
 *
 */

/* Flags define properties regarding
 * the memory the allocator will return */
enum alloc_flags : uint32_t {
    /* Cache alignment */
    ALLOC_FLAG_PREFER_CACHE_ALIGNED = (1 << 0),
    ALLOC_FLAG_NO_CACHE_ALIGN = 0,

    /* Flexible locality */
    ALLOC_FLAG_FLEXIBLE_LOCALITY = (1 << 1),
    ALLOC_FLAG_STRICT_LOCALITY = 0,

    /* Pageable */
    ALLOC_FLAG_PAGEABLE = (1 << 2),
    ALLOC_FLAG_NONPAGEABLE = 0,

    /* Movable */
    ALLOC_FLAG_MOVABLE = (1 << 3),
    ALLOC_FLAG_NONMOVABLE = 0,

    /* Zero on alloc */
    ALLOC_FLAG_ZERO_ON_ALLOC = (1 << 4),
    ALLOC_FLAG_NON_ZERO = 0,

    /* Allocation classes */
    ALLOC_FLAG_CLASS_DEFAULT = (1 << ALLOC_CLASS_SHIFT),
    ALLOC_FLAG_CLASS_INTERLEAVED = (2 << ALLOC_CLASS_SHIFT),
    ALLOC_FLAG_CLASS_HIGH_BANDWIDTH = (3 << ALLOC_CLASS_SHIFT),
};

struct alloc_flag_ex_desc {
    const char *subsystem;
    const char *flag_name;
    enum alloc_flags flag; /* Just that flag */
};

#ifdef DEBUG_ALLOC_PARAM
#define ALLOC_FLAG_EX_REGISTER(subsys, name)                                   \
    static LINKER_SECTION_OBJECT(struct alloc_flag_ex_desc,                    \
                                 alloc_flag_ex_descs)                          \
        __alloc_flag_desc_##subsys##_##name = {                                \
            .subsystem = #subsys,                                              \
            .flag_name = #name,                                                \
            .flag = ALLOC_FLAG_EX(subsys, name)}
#else
#define ALLOC_FLAG_EX_REGISTER(subsys, name)
#endif

/* ────────────── ALLOC FLAG WRAPPERS  ────────────── */

#define ALLOC_FLAGS_NONE 0

/* Bits 6..15 and 24 */
#define ALLOC_FLAGS_UNAVAILABLE_BITS ((1 << 24) | (0x3FF << 16))

#define ALLOC_FLAGS_DEFAULT                                                    \
    (ALLOC_FLAG_CLASS_DEFAULT | ALLOC_FLAG_FLEXIBLE_LOCALITY |                 \
     ALLOC_FLAG_NONMOVABLE | ALLOC_FLAG_NONPAGEABLE |                          \
     ALLOC_FLAG_NO_CACHE_ALIGN | ALLOC_LOCALITY_TO_FLAGS(ALLOC_LOCALITY_MIN))
#define ALLOC_FLAGS_ZERO (ALLOC_FLAGS_DEFAULT | ALLOC_FLAG_ZERO_ON_ALLOC)
#define ALLOC_ZERO .flags = ALLOC_FLAGS_ZERO

#define ALLOC_FLAGS_PAGEABLE                                                   \
    ALLOC_FLAG_PAGEABLE | ALLOC_FLAG_CLASS_DEFAULT |                           \
        ALLOC_FLAG_FLEXIBLE_LOCALITY

static inline bool alloc_flags_valid(enum alloc_flags flags) {
    /* If an unavailable bit is set, it is not valid */
    return !(flags & ALLOC_FLAGS_UNAVAILABLE_BITS);
}

/* ─────────────────────────── ALLOC BEHAVIORS ─────────────────────────── */

#define ALLOC_BEHAVIOR_FLAG_SHIFT 4
#define ALLOC_BEHAVIOR_MASK (0xF)
#define ALLOC_BEHAVIOR_AVAILABLE_SHIFT 12

/* Bits 12..15 are available, this gives the 0-indexed Nth available bit */
#define ALLOC_BEHAVIOR_AVAIL_BIT(n) (1 << (ALLOC_BEHAVIOR_AVAILABLE_SHIFT + n))

/* alloc_behavior: 16 bits for a behavior and flags
 *
 *      ┌───────────────────────────┐
 * Bits │ 15..12  11..8  7..4  3..0 │
 * Use  │  AAAA    ****  **MF  %%%% │
 *      └───────────────────────────┘
 *
 * %%%% - Allocation behavior bits
 *
 * F - "Prefer fast allocation" -- may fail fast/early
 * M - Perform "minimal" allocation -- e.g. do not bother with
 *                                     new slab cache construction
 * A - Unused (Available)
 * * - Unused (Unavailable)
 *
 */

/* Behaviors define what the allocator is
 * allowed to do in a given invocation. */

/* Allocation behavior restricts flags. If non-faulting behaviors
 * are selected, then the allocator cannot allocate pageable memory */
enum alloc_behavior : uint16_t {
    ALLOC_BEHAVIOR_NORMAL,
    ALLOC_BEHAVIOR_NMI_SAFE,
    ALLOC_BEHAVIOR_IRQ_SAFE,
    ALLOC_BEHAVIOR_FAULT_SAFE,
    ALLOC_BEHAVIOR_NO_BLOCK,
    ALLOC_BEHAVIOR_NO_APC,
    ALLOC_BEHAVIOR_MAX,
    ALLOC_BEHAVIOR_FLAG_FAST = 1 << ALLOC_BEHAVIOR_FLAG_SHIFT,

    /* Used by various allocators to prevent
     * recursion in things they interact with */
    ALLOC_BEHAVIOR_FLAG_MINIMAL = 1 << (ALLOC_BEHAVIOR_FLAG_SHIFT + 1),

};
#define ALLOC_BEHAVIOR_DEFAULT ALLOC_BEHAVIOR_NORMAL

/* ─────────────────────────── ALLOC PRIORITIES ─────────────────────────── */

#define ALLOC_PRIORITY_STEP 8
#define ALLOC_PRIORITY_HALFSTEP (ALLOC_PRIORITY_STEP / 2)
#define ALLOC_PRIORITY_ABOVE(p) ((p) + ALLOC_PRIORITY_HALFSTEP)
#define ALLOC_PRIORITY_BELOW(p) ((p) - ALLOC_PRIORITY_HALFSTEP)

/* alloc_priority: 16 bit bitflags
 *
 * Do note that with allocation priorities, the default ones given are
 * in increments of 8. This gives allocators room to interpret the priorities
 * as a continuous curve instead of discrete steps.
 *
 *      ┌──────────────────────────┐
 * Bits │ 15..12 11..8  7..4  3..0 │
 * Use  │  AAAA   AAAA  %%%%  %%%% │
 *      └──────────────────────────┘
 *
 * %%%% - Priority
 * A - Unused (available)
 * * - Unused (unavailable)
 *
 */
enum alloc_priority : uint16_t {
    ALLOC_PRIORITY_MIN = 0,
    ALLOC_PRIORITY_LOW = 8,
    ALLOC_PRIORITY_NORMAL = 16,
    ALLOC_PRIORITY_HIGH = 24,
    ALLOC_PRIORITY_CRITICAL = 32,
    ALLOC_PRIORITY_MAX,
};
static_assert(ALLOC_PRIORITY_MAX < 256);

/* TODO: */
#define ALLOC_PRIORITY_DEFAULT ALLOC_PRIORITY_MIN

struct alloc_params {
    enum alloc_flags flags;
    enum alloc_behavior behavior;
    enum alloc_priority priority;
};

struct alloc_capabilities {
    enum alloc_flags flags;             /* A bitmask */
    enum alloc_behavior behaviors;      /* Bitmask 1 << base */
    enum alloc_behavior behavior_flags; /* Bitmask for remaining flags */
    enum alloc_priority priorities; /* Supported priorities, 1 << base bitmap */
};

#define ALLOC_FLAG_EX_DESC_MAX 8
size_t
alloc_flag_ex_get_desc(enum alloc_flags flag,
                       struct alloc_flag_ex_desc out[ALLOC_FLAG_EX_DESC_MAX]);

/* Extract base behavior (mask out flags) */
static inline enum alloc_behavior alloc_behavior_base(enum alloc_behavior raw) {
    return raw & ALLOC_BEHAVIOR_MASK;
}

/* Does this behavior allow page faults? */
static inline bool alloc_behavior_may_fault(enum alloc_behavior raw) {
    switch (alloc_behavior_base(raw)) {
    case ALLOC_BEHAVIOR_IRQ_SAFE:
    case ALLOC_BEHAVIOR_FAULT_SAFE: return false;
    default: return true;
    }
}

/* Does this behavior allow blocking or waiting? */
static inline bool alloc_behavior_may_block(enum alloc_behavior raw) {
    switch (alloc_behavior_base(raw)) {
    case ALLOC_BEHAVIOR_IRQ_SAFE:
    case ALLOC_BEHAVIOR_NO_BLOCK: return false;
    default: return true;
    }
}

/* Is this behavior ISR-safe? */
static inline bool alloc_behavior_is_isr_safe(enum alloc_behavior raw) {
    return alloc_behavior_base(raw) == ALLOC_BEHAVIOR_IRQ_SAFE ||
           alloc_behavior_base(raw) == ALLOC_BEHAVIOR_NMI_SAFE;
}

/* Fast hint: should this allocation prefer short paths? */
static inline bool alloc_behavior_is_fast(enum alloc_behavior raw) {
    return (raw & ALLOC_BEHAVIOR_FLAG_FAST);
}

static inline bool alloc_flag_behavior_verify(struct alloc_params params) {
    bool may_fault = alloc_behavior_may_fault(params.behavior);
    bool flag_requires_residency = !(params.flags & ALLOC_FLAG_PAGEABLE);
    bool flag_can_fault = (params.flags & ALLOC_FLAG_MOVABLE) ||
                          (params.flags & ALLOC_FLAG_PAGEABLE);

    /* Non-faulting behavior cannot tolerate pageable or movable allocations */
    if (!may_fault && flag_can_fault)
        return false;

    /* ISR-safe behavior must use nonpageable memory */
    if (alloc_behavior_is_isr_safe(params.behavior) && !flag_requires_residency)
        return false;

    return true;
}

/* TODO: Rework these into capabilities */
static inline void alloc_request_sanitize(struct alloc_params *params) {
    if (!alloc_flag_behavior_verify(*params)) {
        if (alloc_behavior_is_isr_safe(params->behavior) ||
            !alloc_behavior_may_fault(params->behavior)) {
            params->flags &= ~(ALLOC_FLAG_PAGEABLE | ALLOC_FLAG_MOVABLE);
            params->flags |= ALLOC_FLAG_NONPAGEABLE;
        }
    }
}
