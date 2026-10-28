#include <sch/sched.h>
#include <sync/condvar.h>
#include <sync/semaphore.h>
#include <sync/spinlock.h>
#include <thread/thread_types.h>

LOCK_CHK_CLASS_DEFINE_LOCAL(semaphore_irq);

void semaphore_init(struct semaphore *s, int value) {
    s->count = value;
    spinlock_init_chk(&s->lock, LOCK_CHK_CLASS(semaphore_irq), LOCK_CHKD_FULL);
    condvar_init(&s->cv);
}

void semaphore_wait(struct semaphore *s) TSA_NO_ANALYSIS {
    enum irql irql = spin_lock_high(&s->lock);

    while (s->count == 0)
        condvar_wait(&s->cv, &s->lock, irql, &irql);

    s->count--;
    spin_unlock(&s->lock, irql);
}

bool semaphore_timedwait(struct semaphore *s,
                         time_ms_t timeout_ms) TSA_NO_ANALYSIS {
    enum irql irql = spin_lock_high(&s->lock);

    while (s->count == 0) {
        enum condvar_wake_reason wr =
            condvar_wait_timeout(&s->cv, &s->lock, timeout_ms, irql, &irql);
        if (wr == CONDVAR_WAKE_REASON_TIMEOUT && s->count == 0) {
            spin_unlock(&s->lock, irql);
            return false;
        }
    }

    s->count--;
    spin_unlock(&s->lock, irql);

    return true;
}

void semaphore_post_full(struct semaphore *s, int n) TSA_NO_ANALYSIS {
    enum irql irql = spin_lock_high(&s->lock);

    s->count += n;
    for (int i = 0; i < n; i++)
        condvar_signal(&s->cv);

    spin_unlock(&s->lock, irql);
}
