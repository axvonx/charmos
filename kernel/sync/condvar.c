#include <sch/irql.h>
#include <sync/condvar.h>
#include <sync/spinlock.h>
#include <thread/thread.h>
#include <thread/thread_types.h>
#include <thread/wait.h>
#include <tsa.h>

static void condvar_prepare_wait(struct condvar *cv) {
    thread_wait_prepare_one(&cv->waiters, cv, THREAD_WAIT_UNINTERRUPTIBLE,
                            THREAD_BLOCK_REASON_MANUAL);
}

static enum condvar_wake_reason
condvar_finish_wait(struct spinlock *lock, enum irql irql, enum irql *out,
                    bool timed, time_ms_t timeout_ms) TSA_NO_ANALYSIS {
    enum irql sirql = irql_get();

    spin_unlock(lock, irql);
    struct thread_wait_result result =
        timed ? thread_wait_complete_timeout(timeout_ms)
              : thread_wait_complete();
    enum condvar_wake_reason reason =
        result.reason == THREAD_WAKE_REASON_SLEEP_TIMEOUT
            ? CONDVAR_WAKE_REASON_TIMEOUT
            : CONDVAR_WAKE_REASON_SIGNAL;

    *out = sirql == IRQL_HIGH_LEVEL ? spin_lock_high(lock) : spin_lock(lock);
    return reason;
}

enum condvar_wake_reason condvar_wait(struct condvar *cv, struct spinlock *lock,
                                      enum irql irql,
                                      enum irql *out) TSA_NO_ANALYSIS {
    condvar_prepare_wait(cv);
    return condvar_finish_wait(lock, irql, out, false, 0);
}

void condvar_init(struct condvar *cv) {
    thread_wait_header_init(&cv->waiters);
}

static void nop_callback(struct thread *unused) {
    cc_ignore(unused);
}

struct thread *condvar_signal_callback(struct condvar *cv,
                                       thread_action_callback tac) {
    struct thread *t = thread_wait_header_satisfy(
        &cv->waiters, THREAD_WAKE_REASON_SLEEP_MANUAL, tac);

    if (!t)
        tac(NULL);
    return t;
}

struct thread *condvar_signal(struct condvar *cv) {
    return condvar_signal_callback(cv, nop_callback);
}

void condvar_broadcast_callback(struct condvar *cv,
                                thread_action_callback tac) {
    thread_wait_header_satisfy_all(
        &cv->waiters, THREAD_WAKE_REASON_SLEEP_MANUAL, tac, NULL, NULL);
}

void condvar_broadcast(struct condvar *cv) {
    condvar_broadcast_callback(cv, nop_callback);
}

enum condvar_wake_reason condvar_wait_timeout(struct condvar *cv,
                                              struct spinlock *lock,
                                              time_ms_t timeout_ms,
                                              enum irql irql,
                                              enum irql *out) TSA_NO_ANALYSIS {
    condvar_prepare_wait(cv);
    return condvar_finish_wait(lock, irql, out, true, timeout_ms);
}
