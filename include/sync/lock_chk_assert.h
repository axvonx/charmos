/* @title: Lock Validator Assertions */
#pragma once
#include <compiler/core.h>
#include <sync/lock_general.h>
#include <sync/mutex.h>
#include <sync/mutex_simple.h>
#include <sync/qspinlock.h>
#include <sync/rwlock.h>
#include <sync/spinlock.h>

/* Assert that the current thread holds the lock. When DEBUG_LOCK_CHK
 * is on and the lock is checked, this actually gets to use
 * the provable engine. But otherwise, it falls back to a naive
 * read of the lock word. RWLOCK needs
 * RWLOCK_READ/RWLOCK_WRITE, which is why we have
 * _1 and _2, so all locks regardless of type can funnel through
 * this callsite */

#define LOCK_CHK_ASSERT_HELD_1(l)                                              \
    _Generic((l),                                                              \
        struct spinlock *: spinlock_assert_held_internal,                      \
        struct qspinlock *: qspin_assert_held_internal,                        \
        struct mutex *: mutex_assert_held_internal,                            \
        struct mutex_simple *: mutex_simple_assert_held_internal)(             \
        (l), LOCK_CHK_SITE_HERE())

#define LOCK_CHK_ASSERT_HELD_2(l, mode)                                        \
    _Generic((l), struct rwlock *: rwlock_assert_held_internal)(               \
        (l), (mode), LOCK_CHK_SITE_HERE())

#define LOCK_CHK_ASSERT_HELD(...) PP_CALL(LOCK_CHK_ASSERT_HELD, __VA_ARGS__)

/* Inverse of above */
#define LOCK_CHK_ASSERT_NOT_HELD(l)                                            \
    _Generic((l),                                                              \
        struct spinlock *: spinlock_assert_not_held_internal,                  \
        struct qspinlock *: qspin_assert_not_held_internal,                    \
        struct mutex *: mutex_assert_not_held_internal,                        \
        struct mutex_simple *: mutex_simple_assert_not_held_internal,          \
        struct rwlock *: rwlock_assert_not_held_internal)(                     \
        (l), LOCK_CHK_SITE_HERE())

#define LOCK_CHK_ASSERT_HELD_STATE(l, m)                                       \
    do {                                                                       \
        if ((m) == LOCK_HELD) {                                                \
            LOCK_CHK_ASSERT_HELD((l));                                         \
        } else if ((m) == LOCK_NOT_HELD) {                                     \
            LOCK_CHK_ASSERT_NOT_HELD((l));                                     \
        } else {                                                               \
            panic("impossible mode %d", (m));                                  \
        }                                                                      \
    } while (0)
