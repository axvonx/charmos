#pragma once
#include <console/crash.h>
#include <sch/irql.h>
#include <sync/condvar_types.h>
#include <sync/spinlock_types.h>
#include <thread/wait.h>

enum condvar_wake_reason {
    CONDVAR_WAKE_REASON_NONE = 0,    /* No reason specified */
    CONDVAR_WAKE_REASON_SIGNAL = 1,  /* Signal from something */
    CONDVAR_WAKE_REASON_TIMEOUT = 2, /* Timeout */
};

typedef void (*thread_action_callback)(struct thread *woke);

enum condvar_wake_reason condvar_wait(struct condvar *cv, struct spinlock *lock,
                                      enum irql irql, enum irql *out);

void condvar_init(struct condvar *cv);
struct thread *condvar_signal(struct condvar *cv);
struct thread *condvar_signal_callback(struct condvar *cv,
                                       thread_action_callback cb);

void condvar_broadcast_callback(struct condvar *cv, thread_action_callback cb);

void condvar_broadcast(struct condvar *cv);

enum condvar_wake_reason condvar_wait_timeout(struct condvar *cv,
                                              struct spinlock *lock,
                                              time_ms_t timeout_ms,
                                              enum irql irql, enum irql *out);
