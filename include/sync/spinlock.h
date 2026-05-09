#pragma once
#include <asm.h>
#include <bootstage.h>
#include <console/panic.h>
#include <irq/irq.h>
#include <kassert.h>
#include <sch/irql.h>
#include <smp/core.h>
#include <stdatomic.h>
#include <stdbool.h>

#ifdef DEBUG_LOCK
#define SPINLOCK_COOKIE_MAGIC 0xBEEB00
#endif

struct spinlock {
    _Atomic uint8_t state;

#ifdef DEBUG_LOCK
    bool acquired_high;
    uint32_t initialized_magic;
#endif
};

#ifdef DEBUG_LOCK
#define SPINLOCK_INIT                                                          \
    {.state = ATOMIC_VAR_INIT(0),                                              \
     .acquired_high = false,                                                   \
     .initialized_magic = SPINLOCK_COOKIE_MAGIC}
#else
#define SPINLOCK_INIT {ATOMIC_VAR_INIT(0)}
#endif

static inline void spinlock_init(struct spinlock *lock) {

#ifdef DEBUG_LOCK
    lock->initialized_magic = SPINLOCK_COOKIE_MAGIC;
    lock->acquired_high = false;
#endif

    atomic_store(&lock->state, 0);
}

static inline bool spin_trylock_raw(struct spinlock *lock) {

#ifdef DEBUG_LOCK
    kassert(lock->initialized_magic == SPINLOCK_COOKIE_MAGIC);
#endif

    uint8_t expected = 0;
    return atomic_compare_exchange_strong_explicit(
        &lock->state, &expected, 1, memory_order_acquire, memory_order_relaxed);
}

static inline void spin_raw(struct spinlock *lock) {
    while (atomic_load_explicit(&lock->state, memory_order_relaxed) != 0)
        cpu_relax();
}

static inline void spin_lock_raw(struct spinlock *lock) {
    while (true) {
        if (spin_trylock_raw(lock))
            return;

        spin_raw(lock);
    }
}

static inline void spin_unlock_raw(struct spinlock *lock) {
    atomic_store_explicit(&lock->state, 0, memory_order_release);
}

static inline void spin_unlock(struct spinlock *lock, enum irql old) {
    atomic_exchange_explicit(&lock->state, 0, memory_order_release);
    irql_lower(old);
}

static inline enum irql spin_lock(struct spinlock *lock) {
    if (bootstage_get() >= BOOTSTAGE_MID_MP && irq_in_interrupt())
        panic("Attempted to take non-ISR safe spinlock from an ISR!\n");

#ifdef DEBUG_LOCK
    kassert(!lock->acquired_high);
#endif

    enum irql irql = irql_raise(IRQL_DISPATCH_LEVEL);
    spin_lock_raw(lock);
    return irql;
}

static inline enum irql spin_lock_irq_disable(struct spinlock *lock) {

#ifdef DEBUG_LOCK
    lock->acquired_high = true;
#endif

    enum irql irql = irql_raise(IRQL_HIGH_LEVEL);
    spin_lock_raw(lock);
    return irql;
}

static inline bool spin_trylock(struct spinlock *lock, enum irql *out) {

#ifdef DEBUG_LOCK
    kassert(!lock->acquired_high);
#endif

    *out = irql_raise(IRQL_DISPATCH_LEVEL);
    if (spin_trylock_raw(lock))
        return true;

    irql_lower(*out);
    return false;
}

static inline bool spin_trylock_irq_disable(struct spinlock *lock,
                                            enum irql *out) {

#ifdef DEBUG_LOCK
    lock->acquired_high = true;
#endif

    *out = irql_raise(IRQL_HIGH_LEVEL);
    if (spin_trylock_raw(lock))
        return true;

    irql_lower(*out);
    return false;
}

static inline bool spinlock_held(struct spinlock *lock) {
    return atomic_load(&lock->state);
}
#define SPINLOCK_ASSERT_HELD(l) kassert(spinlock_held(l))
