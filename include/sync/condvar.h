#pragma once
#include <sync/spinlock.h>
#include <thread/wait.h>
#include <time/timer.h>

typedef void (*thread_action_callback)(struct thread *woke);

struct condvar {
    struct thread_wait_header waiters;
};

enum wake_reason condvar_wait(struct condvar *cv, struct spinlock *lock,
                              enum irql irql, enum irql *out);

void condvar_init(struct condvar *cv);
struct thread *condvar_signal(struct condvar *cv);
struct thread *condvar_signal_callback(struct condvar *cv,
                                       thread_action_callback cb);

void condvar_broadcast_callback(struct condvar *cv, thread_action_callback cb);

void condvar_broadcast(struct condvar *cv);

enum wake_reason condvar_wait_timeout(struct condvar *cv, struct spinlock *lock,
                                      time_ms_t timeout_ms, enum irql irql,
                                      enum irql *out);
