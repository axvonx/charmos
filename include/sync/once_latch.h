/* @title: One-time atomic countdown latches */
#pragma once
#include <asm.h>
#include <atomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <time/time.h>

struct once_latch {
    atomic_uint64_t count;
};

#define ONCE_LATCH_INIT(n) {.count = ATOMIC_VAR_INIT(n)}

static inline void once_latch_init(struct once_latch *latch, uint64_t count) {
    atomic_init(&latch->count, count);
}

static inline bool once_latch_count_down(struct once_latch *latch) {
    return atomic_dec_and_test(&latch->count);
}

static inline bool once_latch_is_ready(const struct once_latch *latch) {
    return atomic_load_acq(&latch->count) == 0;
}

static inline uint64_t once_latch_count(const struct once_latch *latch) {
    return atomic_load_acq(&latch->count);
}

static inline void once_latch_spin_wait(const struct once_latch *latch) {
    while (!once_latch_is_ready(latch))
        cpu_pause();
}

static inline bool once_latch_spin_timeout(const struct once_latch *latch,
                                           time_ms_t timeout_ms) {
    time_ms_t deadline = time_get_ms() + timeout_ms;
    while (!once_latch_is_ready(latch)) {
        if (time_get_ms() >= deadline)
            return once_latch_is_ready(latch);
        cpu_pause();
    }
    return true;
}
