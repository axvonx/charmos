/* @title: Page Table */
#pragma once
#pragma once
#include <asm.h>
#include <irq/irq.h>
#include <mem/page.h>
#include <sch/irql.h>
#include <stdatomic.h>
#include <stdint.h>

#define PT_LEVELS 4

#define PT_SHIFT_L1 12
#define PT_SHIFT_L2 21
#define PT_SHIFT_L3 30
#define PT_SHIFT_L4 39

#define PT_STRIDE 9

#define PTE_LOCK_BIT ((uint64_t) 1 << 9)
#define PTE_AVAIL2_BIT ((uint64_t) 1 << 11)

typedef _Atomic uint64_t pte_atomic_t;

static inline void pte_unlock_internal(pte_atomic_t *pte) {
    kassert(atomic_fetch_and_explicit(pte, ~PTE_LOCK_BIT, memory_order_release) & PTE_LOCK_BIT);
}

static inline void pte_lock_internal(pte_atomic_t *pte) {
    for (;;) {
        uint64_t old = atomic_load_explicit(pte, memory_order_relaxed);

        if (old & PTE_LOCK_BIT) {
            cpu_relax();
            continue;
        }

        if (atomic_compare_exchange_weak_explicit(pte, &old, old | PTE_LOCK_BIT,
                                                  memory_order_acquire,
                                                  memory_order_relaxed))
            return;

        cpu_relax();
    }
}

static inline enum irql pte_lock_irql(pte_atomic_t *pte) {
    enum irql old_irql = irql_raise(IRQL_DISPATCH_LEVEL);
    pte_lock_internal(pte);
    return old_irql;
}

static inline void pte_unlock_irql(pte_atomic_t *pte, enum irql old_irql) {
    pte_unlock_internal(pte);
    irql_lower(old_irql);
}
