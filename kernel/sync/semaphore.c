#include <sch/sched.h>
#include <sync/condvar.h>
#include <sync/semaphore.h>
#include <sync/spinlock.h>
#include <thread/thread_types.h>

LOCK_CHK_CLASS_DECLARE_LOCAL(semaphore_irq);
LOCK_CHK_CLASS_DECLARE_LOCAL(semaphore_disp);

void semaphore_init(struct semaphore *s, int value, bool irq_disable) {
    atomic_init(&s->count, value);
    s->irq_disable = irq_disable;
    if (irq_disable) {
        spinlock_init_chk(&s->lock, LOCK_CHK_CLASS(semaphore_irq),
                          LOCK_CHKD_FULL);
    } else {
        spinlock_init_chk(&s->lock, LOCK_CHK_CLASS(semaphore_disp),
                          LOCK_CHKD_FULL);
    }
    condvar_init(&s->cv, irq_disable);
}

static enum irql
semaphore_lock_internal(struct semaphore *sem) TSA_NO_ANALYSIS {
    if (sem->irq_disable)
        return spin_lock_irq_disable(&sem->lock);

    return spin_lock(&sem->lock);
}

void semaphore_wait(struct semaphore *s) TSA_NO_ANALYSIS {
    enum irql irql = semaphore_lock_internal(s);

    while (atomic_load_relaxed(&s->count) == 0)
        condvar_wait(&s->cv, &s->lock, irql, &irql);

    atomic_dec_relaxed(&s->count);
    spin_unlock(&s->lock, irql);
}

bool semaphore_timedwait(struct semaphore *s,
                         time_ms_t timeout_ms) TSA_NO_ANALYSIS {
    enum irql irql = semaphore_lock_internal(s);

    while (atomic_load_relaxed(&s->count) == 0) {
        enum wake_reason wr =
            condvar_wait_timeout(&s->cv, &s->lock, timeout_ms, irql, &irql);
        if (wr == WAKE_REASON_TIMEOUT && atomic_load_relaxed(&s->count) == 0) {
            spin_unlock(&s->lock, irql);
            return false;
        }
    }

    atomic_dec_relaxed(&s->count);
    spin_unlock(&s->lock, irql);

    return true;
}

void semaphore_post(struct semaphore *s) TSA_NO_ANALYSIS {
    enum irql irql = semaphore_lock_internal(s);

    atomic_inc_relaxed(&s->count);

    condvar_signal(&s->cv);

    spin_unlock(&s->lock, irql);
}

void semaphore_postn(struct semaphore *s, int n) TSA_NO_ANALYSIS {
    enum irql irql = semaphore_lock_internal(s);

    atomic_fetch_add_relaxed(&s->count, n);
    for (int i = 0; i < n; i++)
        condvar_signal(&s->cv);

    spin_unlock(&s->lock, irql);
}
