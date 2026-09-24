/* @title: Raw Spinlock */
#pragma once
#include <asm.h>
#include <atomic.h>
#include <compiler/core.h>
#include <stdbool.h>
#include <stdint.h>
#include <sync/lock_general.h>

struct TSA_CAPABILITY("spinlock") raw_spinlock {
    atomic_uint8_t state;
};

#define RAW_SPINLOCK_INIT ((struct raw_spinlock) {.state = ATOMIC_VAR_INIT(0)})
#define RAW_SPINLOCK_DEFINE(id) struct raw_spinlock id = RAW_SPINLOCK_INIT

static inline void raw_spinlock_init(struct raw_spinlock *lock) {
    atomic_store_relaxed(&lock->state, 0);
}

static inline cc_warn_unused_result bool
raw_spin_trylock(struct raw_spinlock *lock)
    TSA_TRY_ACQUIRES(true, lock) TSA_NO_ANALYSIS {
    uint8_t expected = 0;
    return atomic_cas_strong(&lock->state, &expected, 1, mo_acquire,
                             mo_relaxed);
}

static inline void raw_spin_lock(struct raw_spinlock *lock)
    TSA_ACQUIRES(lock) TSA_NO_ANALYSIS {
    while (true) {
        if (raw_spin_trylock(lock))
            return;

        while (atomic_load_relaxed(&lock->state) != 0)
            cpu_pause();
    }
}

static inline void raw_spin_unlock(struct raw_spinlock *lock)
    TSA_RELEASES(lock) TSA_NO_ANALYSIS {
    atomic_store_release(&lock->state, 0);
}

/* whether interrupts were enabled on entry */
static inline cc_warn_unused_result bool
raw_spin_lock_high(struct raw_spinlock *lock)
    TSA_ACQUIRES(lock) TSA_NO_ANALYSIS {
    bool irqs_were_enabled = irq_disable_save();
    raw_spin_lock(lock);
    return irqs_were_enabled;
}

static inline void raw_spin_unlock_irq_restore(struct raw_spinlock *lock,
                                               bool irqs_were_enabled)
    TSA_RELEASES(lock) TSA_NO_ANALYSIS {
    raw_spin_unlock(lock);
    if (irqs_were_enabled)
        irq_enable();
}
