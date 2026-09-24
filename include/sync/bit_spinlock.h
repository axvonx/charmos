/* @title: Bit Spinlock */
#pragma once
#include <asm.h>
#include <atomic.h>
#include <compiler/core.h>
#include <compiler/intrinsic.h>
#include <kassert.h>
#include <math/bit.h>
#include <sch/irql.h>
#include <stdbool.h>
#include <stdint.h>

/* bit_spinlock: not an actual struct, but a series of macros for locking any
 * single bit within a byte, word, dword, and qword with comptime bounds check
 */

#define BIT_SPINLOCK_MASK(bit, ptr) ((typeof(*(ptr))) BIT(bit))

#define BIT_SPINLOCK_CHECK(bit, ptr)                                           \
    do {                                                                       \
        static_assert(                                                         \
            ci_constant_p(bit) ? ((size_t) (bit) < sizeof(*(ptr)) * 8) : 1,    \
            "bit index exceeds type width");                                   \
        kassert((size_t) (bit) < sizeof(*(ptr)) * 8,                           \
                "bit index out of bounds for pointer type");                   \
    } while (0)

#define bit_spin_is_locked_raw(bit, ptr)                                       \
    ({                                                                         \
        BIT_SPINLOCK_CHECK(bit, ptr);                                          \
        atomic_test_bit_relaxed((atomic(typeof(*(ptr))) *) (ptr), bit);        \
    })

#define bit_spin_trylock_raw(bit, ptr)                                         \
    ({                                                                         \
        BIT_SPINLOCK_CHECK(bit, ptr);                                          \
        typeof(*(ptr)) __m = BIT_SPINLOCK_MASK(bit, ptr);                      \
        typeof(*(ptr)) __old =                                                 \
            atomic_load_relaxed((atomic(typeof(*(ptr))) *) (ptr));             \
        bool __acquired = false;                                               \
        if (!(__old & __m)) {                                                  \
            __acquired = atomic_cas_weak(                                      \
                (atomic(typeof(*(ptr))) *) (ptr), &__old,                      \
                (typeof(*(ptr))) (__old | __m), mo_acquire, mo_relaxed);       \
        }                                                                      \
        __acquired;                                                            \
    })

#define bit_spin_lock_raw(bit, ptr)                                            \
    do {                                                                       \
        BIT_SPINLOCK_CHECK(bit, ptr);                                          \
        typeof(*(ptr)) __m = BIT_SPINLOCK_MASK(bit, ptr);                      \
        while (true) {                                                         \
            typeof(*(ptr)) __old =                                             \
                atomic_load_relaxed((atomic(typeof(*(ptr))) *) (ptr));         \
            if (__old & __m) {                                                 \
                cpu_pause();                                                   \
                continue;                                                      \
            }                                                                  \
            if (atomic_cas_weak((atomic(typeof(*(ptr))) *) (ptr), &__old,      \
                                (typeof(*(ptr))) (__old | __m), mo_acquire,    \
                                mo_relaxed))                                   \
                break;                                                         \
            cpu_pause();                                                       \
        }                                                                      \
    } while (0)

#define bit_spin_unlock_raw(bit, ptr)                                          \
    do {                                                                       \
        BIT_SPINLOCK_CHECK(bit, ptr);                                          \
        kassert(atomic_test_and_clear_bit_release(                             \
                    (atomic(typeof(*(ptr))) *) (ptr), bit),                    \
                "bit spinlock unlock on unheld lock");                         \
    } while (0)

#define bit_spin_lock(bit, ptr)                                                \
    ({                                                                         \
        enum irql __old_irql = irql_raise(IRQL_DISPATCH_LEVEL);                \
        bit_spin_lock_raw(bit, ptr);                                           \
        __old_irql;                                                            \
    })

#define bit_spin_unlock(bit, ptr, old_irql)                                    \
    do {                                                                       \
        bit_spin_unlock_raw(bit, ptr);                                         \
        irql_lower(old_irql);                                                  \
    } while (0)

#define bit_spin_lock_high(bit, ptr)                                           \
    ({                                                                         \
        enum irql __old_irql = irql_raise(IRQL_HIGH_LEVEL);                    \
        bit_spin_lock_raw(bit, ptr);                                           \
        __old_irql;                                                            \
    })

#define bit_spin_unlock_irq_restore(bit, ptr, old_irql)                        \
    do {                                                                       \
        bit_spin_unlock_raw(bit, ptr);                                         \
        irql_lower(old_irql);                                                  \
    } while (0)

#define bit_spin_trylock(bit, ptr, out_irql)                                   \
    ({                                                                         \
        *(out_irql) = irql_raise(IRQL_DISPATCH_LEVEL);                         \
        bool __ok = bit_spin_trylock_raw(bit, ptr);                            \
        if (!__ok)                                                             \
            irql_lower(*(out_irql));                                           \
        __ok;                                                                  \
    })

#define bit_spin_trylock_high(bit, ptr, out_irql)                              \
    ({                                                                         \
        *(out_irql) = irql_raise(IRQL_HIGH_LEVEL);                             \
        bool __ok = bit_spin_trylock_raw(bit, ptr);                            \
        if (!__ok)                                                             \
            irql_lower(*(out_irql));                                           \
        __ok;                                                                  \
    })

#define BIT_SPIN_LOCK_ASSERT_HELD(bit, ptr)                                    \
    kassert(bit_spin_is_locked_raw(bit, ptr), "bitlock not held")
