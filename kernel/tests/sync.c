#include <asm.h>
#include <test/sync.h>
#include <thread/thread_types.h>
#include <time/time.h>

#ifdef TEST_ENABLED

bool test_phase_wait(struct test_phase *p, uint32_t expected,
                     time_ms_t timeout_ms) TSA_NO_ANALYSIS {
    time_ms_t deadline = time_get_ms() + timeout_ms;
    enum irql irql = spin_lock(&p->lock);

    while (atomic_load_acq(&p->phase) != expected) {
        if (atomic_load_acq(&p->poisoned)) {
            spin_unlock(&p->lock, irql);
            return false;
        }

        time_ms_t now = time_get_ms();
        if (now >= deadline) {
            spin_unlock(&p->lock, irql);
            return false;
        }

        condvar_wait_timeout(&p->cv, &p->lock, deadline - now, irql, &irql);
    }

    bool poisoned = atomic_load_acq(&p->poisoned);
    spin_unlock(&p->lock, irql);
    return !poisoned;
}

#endif /* TEST_ENABLED */
