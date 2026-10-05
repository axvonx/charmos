/* @title: General Locking */
#pragma once
#include <compiler/name.h>
#include <stdint.h>

/* The reason we have this is because code around the kernel uses a function
 * parameter to indicate the held state of a lock. Using this enum rather than
 * booleans avoids ad-hoc annotations at callsites, assigns explicit names, and
 * improves readability and searchability. */
enum lock_held_state {
    /* We're intentionally using truthy and falsy compatible values here:
     * much code assumes true == locked, and this will remain stable so
     * only the signatures and types change, and code can still go
     *
     * `if (held) do_thing();` */
    LOCK_NOT_HELD = 0,
    LOCK_HELD = 1,
};

/* for breaking ordering cycles between locks of the same class
 * max is LOCK_CHK_MAX_SUBCLASSES (8) */
enum lock_subclass : uint8_t {
    LOCK_SUBCLASS_PRIMARY = 0,
    LOCK_SUBCLASS_SECONDARY = 1,
    LOCK_SUBCLASS_TERTIARY = 2,

    /* aliases */
    LOCK_SUBCLASS_PARENT = 0,
    LOCK_SUBCLASS_CHILD = 1,

    LOCK_SUBCLASS_SRC_CPU = 0,
    LOCK_SUBCLASS_DST_CPU = 1,
};

enum lock_acquire_policy {
    LOCK_ACQUIRE_BLOCKING = 0,
    LOCK_ACQUIRE_NONBLOCKING = 1,
};

#if defined(cn_id_clang)

#define TSA_CAPABILITY(kind) __attribute__((capability(kind)))
#define TSA_GUARDED_BY(lock) __attribute__((guarded_by(lock)))
#define TSA_MUST_HOLD(lock) __attribute__((requires_capability(lock)))
#define TSA_ACQUIRES(lock) __attribute__((acquire_capability(lock)))
#define TSA_RELEASES(lock) __attribute__((release_capability(lock)))
#define TSA_TRY_ACQUIRES(ret, lock)                                            \
    __attribute__((try_acquire_capability(ret, lock)))
#define TSA_ASSERT_CAPABILITY(lock) __attribute__((assert_capability(lock)))
#define TSA_EXCLUDED(lock) __attribute__((locks_excluded(lock)))
#define TSA_NO_ANALYSIS __attribute__((no_thread_safety_analysis))
#define TSA_CAPABILITY_DECLARE(name, symbol)                                   \
    struct TSA_CAPABILITY(name) __##symbol##_ctx {};                           \
    extern struct __##symbol##_ctx symbol

/* IRQL tracking. Reentrant capabilities give us nested raises and spinlocks,
 * and we exclude IRQL_RAISED for sleeping primitives like mutexes */
#if cn_has_attribute(reentrant_capability)
#define TSA_IRQL_TRACKS_SPINLOCKS 1
#define TSA_REENTRANT_CAPABILITY_DECLARE(name, symbol)                         \
    struct TSA_CAPABILITY(name)                                                \
        __attribute__((reentrant_capability)) __##symbol##_ctx {};             \
    extern struct __##symbol##_ctx symbol
#else
#define TSA_IRQL_TRACKS_SPINLOCKS 0
#define TSA_REENTRANT_CAPABILITY_DECLARE(name, symbol)                         \
    TSA_CAPABILITY_DECLARE(name, symbol)
#endif

#if TSA_IRQL_TRACKS_SPINLOCKS
#define TSA_ACQUIRES_SPIN(lock)                                                \
    __attribute__((acquire_capability(lock, IRQL_RAISED)))
#define TSA_RELEASES_SPIN(lock)                                                \
    __attribute__((release_capability(lock, IRQL_RAISED)))
#define TSA_TRY_ACQUIRES_SPIN(ret, lock)                                       \
    __attribute__((try_acquire_capability(ret, lock, IRQL_RAISED)))
#define TSA_ACQUIRES_SPIN_IRQL TSA_ACQUIRES(IRQL_RAISED)
#define TSA_RELEASES_SPIN_IRQL TSA_RELEASES(IRQL_RAISED)
#else
#define TSA_ACQUIRES_SPIN(lock) TSA_ACQUIRES(lock)
#define TSA_RELEASES_SPIN(lock) TSA_RELEASES(lock)
#define TSA_TRY_ACQUIRES_SPIN(ret, lock) TSA_TRY_ACQUIRES(ret, lock)
#define TSA_ACQUIRES_SPIN_IRQL
#define TSA_RELEASES_SPIN_IRQL
#endif

/* Interrupt primitives, just like IRQL raises, no
 * irq sleeping with MAY_SLEEP */
#if TSA_IRQL_TRACKS_SPINLOCKS
#define TSA_ACQUIRES_IRQS TSA_ACQUIRES(IRQS_DISABLED)
#define TSA_RELEASES_IRQS TSA_RELEASES(IRQS_DISABLED)
#define TSA_ACQUIRES_IRQS_LOCK(lock)                                           \
    __attribute__((acquire_capability(lock, IRQS_DISABLED)))
#define TSA_RELEASES_IRQS_LOCK(lock)                                           \
    __attribute__((release_capability(lock, IRQS_DISABLED)))
#define TSA_MAY_SLEEP                                                          \
    __attribute__((locks_excluded(IRQL_RAISED, IRQS_DISABLED)))
#else
#define TSA_ACQUIRES_IRQS
#define TSA_RELEASES_IRQS
#define TSA_ACQUIRES_IRQS_LOCK(lock) TSA_ACQUIRES(lock)
#define TSA_RELEASES_IRQS_LOCK(lock) TSA_RELEASES(lock)
#define TSA_MAY_SLEEP TSA_EXCLUDED(IRQL_RAISED)
#endif

#else

#define TSA_CAPABILITY(kind)
#define TSA_GUARDED_BY(lock)
#define TSA_MUST_HOLD(lock)
#define TSA_ACQUIRES(lock)
#define TSA_RELEASES(lock)
#define TSA_TRY_ACQUIRES(ret, lock)
#define TSA_ASSERT_CAPABILITY(lock)
#define TSA_EXCLUDED(lock)
#define TSA_NO_ANALYSIS
#define TSA_CAPABILITY_DECLARE(name, symbol)
#define TSA_IRQL_TRACKS_SPINLOCKS 0
#define TSA_REENTRANT_CAPABILITY_DECLARE(name, symbol)
#define TSA_ACQUIRES_SPIN(lock)
#define TSA_RELEASES_SPIN(lock)
#define TSA_TRY_ACQUIRES_SPIN(ret, lock)
#define TSA_ACQUIRES_SPIN_IRQL
#define TSA_RELEASES_SPIN_IRQL
#define TSA_ACQUIRES_IRQS
#define TSA_RELEASES_IRQS
#define TSA_ACQUIRES_IRQS_LOCK(lock)
#define TSA_RELEASES_IRQS_LOCK(lock)
#define TSA_MAY_SLEEP

#endif
