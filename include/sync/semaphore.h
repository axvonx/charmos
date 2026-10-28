#pragma once
#include <stdint.h>
#include <sync/condvar.h>
#include <sync/spinlock.h>

struct semaphore {
    int32_t count;

    struct spinlock lock;
    struct condvar cv;
};

void semaphore_init(struct semaphore *s, int value);
void semaphore_wait(struct semaphore *s) TSA_MAY_SLEEP;
bool semaphore_timedwait(struct semaphore *s,
                         time_ms_t timeout_ms) TSA_MAY_SLEEP;
void semaphore_post(struct semaphore *s);
void semaphore_postn(struct semaphore *s, int n);
