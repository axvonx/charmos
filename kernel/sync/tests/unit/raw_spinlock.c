#include "sync/tests/test_internal.h"

#include <asm.h>
#include <sync/raw_spinlock.h>

TEST_GROUP_DECLARE(raw_spinlock);

static void raw_spin_test_body(bool *rejected_out,
                               bool *acquired_out) TSA_NO_ANALYSIS {
    struct raw_spinlock lock = RAW_SPINLOCK_INIT;

    raw_spin_lock(&lock);
    *rejected_out = !raw_spin_trylock(&lock);
    raw_spin_unlock(&lock);

    *acquired_out = raw_spin_trylock(&lock);
    if (*acquired_out)
        raw_spin_unlock(&lock);
}

TEST_DECLARE_UNIT(raw_spinlock, physical_operations) {
    bool rejected_while_held = false;
    bool acquired_after_release = false;
    raw_spin_test_body(&rejected_while_held, &acquired_after_release);

    TEST_ASSERT(rejected_while_held);
    TEST_ASSERT(acquired_after_release);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(raw_spinlock, irq_restore) {
    struct raw_spinlock lock = RAW_SPINLOCK_INIT;
    bool entry_irqs_enabled = irqs_enabled();

    bool saved_irqs_enabled = raw_spin_lock_irq_disable(&lock);
    bool disabled_while_held = !irqs_enabled();
    raw_spin_unlock_irq_restore(&lock, saved_irqs_enabled);
    bool restored_entry_state = irqs_enabled() == entry_irqs_enabled;

    irq_disable();
    bool saved_disabled_state = raw_spin_lock_irq_disable(&lock);
    raw_spin_unlock_irq_restore(&lock, saved_disabled_state);
    bool remained_disabled = !irqs_enabled();

    if (entry_irqs_enabled)
        irq_enable();

    TEST_ASSERT_EQ(saved_irqs_enabled, entry_irqs_enabled);
    TEST_ASSERT(disabled_while_held);
    TEST_ASSERT(restored_entry_state);
    TEST_ASSERT(!saved_disabled_state);
    TEST_ASSERT(remained_disabled);
    return TEST_SUCCESS;
}
