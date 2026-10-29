#pragma once
#include <sync/condvar_types.h>
#include <sync/spinlock_types.h>
#include <tsa.h>
#include <types/types.h>

struct semaphore {
    int32_t count;

    struct spinlock lock;
    struct condvar cv;
};

void semaphore_init(struct semaphore *s, int value);
void semaphore_wait(struct semaphore *s) TSA_MAY_SLEEP;
bool semaphore_timedwait(struct semaphore *s,
                         time_ms_t timeout_ms) TSA_MAY_SLEEP;
void semaphore_post_full(struct semaphore *s, int n);

#define semaphore_post_1(s) semaphore_post_full(s, 1)
#define semaphore_post_2(s, n) semaphore_post_full(s, n)
#define semaphore_post(...) PP_CALL(semaphore_post, __VA_ARGS__)
