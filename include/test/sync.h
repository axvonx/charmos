/* @title: Test coordination primitives */
#pragma once
#include <compiler/core.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <sync/completion.h>
#include <sync/completion_latch.h>
#include <sync/condvar.h>
#include <sync/lock_general.h>
#include <sync/spinlock.h>
#include <time/time.h>

/* Just a wrapper around completion_latch. Someday we could
 *
 * TODO: add metadata and tracking around this.
 *
 * But we only really need a completion_latch for "correctness" */
struct test_latch {
    struct completion_latch cl;
};

/* This effectively serves as the
 *
 * while (atomic_load(&v->phase) != n)
 *     scheduler_yield();
 *
 * pattern that tests like, with a poison bool to tell it to end */
struct test_phase {
    struct spinlock lock;
    struct condvar cv;
    _Atomic uint32_t phase;
    _Atomic bool poisoned;
};

bool test_phase_wait(struct test_phase *p, uint32_t expected,
                     time_ms_t timeout_ms) TSA_EXCLUDED(IRQL_RAISED);

static inline void test_latch_init(struct test_latch *l) {
    completion_latch_init(&l->cl, 1, COMPLETION_INIT_NORMAL);
}

static inline void test_latch_set(struct test_latch *l) {
    completion_latch_count_down(&l->cl);
}

static inline bool test_latch_test(const struct test_latch *l) {
    return completion_latch_is_ready(&l->cl);
}

static inline bool test_latch_wait_timeout(struct test_latch *l,
                                           time_ms_t timeout_ms)
    TSA_EXCLUDED(IRQL_RAISED) {
    return completion_latch_wait_timeout(&l->cl, timeout_ms);
}

static inline bool test_latch_spin_timeout(struct test_latch *l,
                                           time_ms_t timeout_ms) {
    return once_latch_spin_timeout(&l->cl.latch, timeout_ms);
}

static inline void test_phase_init(struct test_phase *p) {
    spinlock_init(&p->lock);
    condvar_init(&p->cv, CONDVAR_INIT_NORMAL);
    atomic_store_explicit(&p->phase, 0, memory_order_relaxed);
    atomic_store_explicit(&p->poisoned, false, memory_order_release);
}

static inline uint32_t test_phase_get(const struct test_phase *p) {
    return atomic_load_explicit(&p->phase, memory_order_acquire);
}

static inline void test_phase_publish(struct test_phase *p, uint32_t phase) {
    enum irql irql = spin_lock(&p->lock);
    atomic_store_explicit(&p->phase, phase, memory_order_release);
    condvar_broadcast(&p->cv);
    spin_unlock(&p->lock, irql);
}

static inline void test_phase_set(struct test_phase *p, uint32_t phase) {
    test_phase_publish(p, phase);
}

static inline void test_phase_advance(struct test_phase *p) {
    enum irql irql = spin_lock(&p->lock);
    atomic_fetch_add_explicit(&p->phase, 1, memory_order_release);
    condvar_broadcast(&p->cv);
    spin_unlock(&p->lock, irql);
}

static inline bool test_phase_is_poisoned(const struct test_phase *p) {
    return atomic_load_explicit(&p->poisoned, memory_order_acquire);
}

static inline void test_phase_poison(struct test_phase *p) {
    enum irql irql = spin_lock(&p->lock);
    atomic_store_explicit(&p->poisoned, true, memory_order_release);
    condvar_broadcast(&p->cv);
    spin_unlock(&p->lock, irql);
}
