#pragma once
#include <stdint.h>
#include <sync/condvar.h>
#include <sync/spinlock.h>

/* These are for the underlying spinlock */
#define SEMAPHORE_INIT_IRQ_DISABLE true
#define SEMAPHORE_INIT_NORMAL false

struct semaphore {
    atomic_int32_t count;
    bool irq_disable;

    struct spinlock lock;
    struct condvar cv;
};

void semaphore_init(struct semaphore *s, int value, bool irq_disable);
void semaphore_wait(struct semaphore *s) TSA_EXCLUDED(IRQL_RAISED);
bool semaphore_timedwait(struct semaphore *s, time_ms_t timeout_ms)
    TSA_EXCLUDED(IRQL_RAISED);
void semaphore_post(struct semaphore *s);
void semaphore_postn(struct semaphore *s, int n);
