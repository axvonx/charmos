/* @title: One-time atomic counters */
#pragma once
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

struct once_counter {
    _Atomic uint64_t count;
};

#define ONCE_COUNTER_INIT {.count = ATOMIC_VAR_INIT(0)}

static inline void once_counter_init(struct once_counter *c) {
    atomic_init(&c->count, 0);
}

static inline bool once_counter_claim(struct once_counter *c) {
    return atomic_fetch_add_explicit(&c->count, 1, memory_order_acq_rel) == 0;
}

static inline bool once_counter_claim_ticket(struct once_counter *c,
                                             uint64_t *out_ticket) {
    uint64_t ticket =
        atomic_fetch_add_explicit(&c->count, 1, memory_order_acq_rel);

    if (out_ticket)
        *out_ticket = ticket;

    return ticket == 0;
}

static inline uint64_t once_counter_count(const struct once_counter *c) {
    return atomic_load_explicit(&c->count, memory_order_acquire);
}
