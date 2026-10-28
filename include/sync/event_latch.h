/* @title: Countdown latch (using events) */
#pragma once
#include <sync/event.h>
#include <sync/once_latch.h>
#include <time/time.h>

struct event_latch {
    struct once_latch latch;
    struct event ready;
};

#define EVENT_LATCH_INIT(name_, n)                                             \
    (struct event_latch) {                                                     \
        .latch = ONCE_LATCH_INIT(n),                                           \
        .ready = EVENT_INIT((name_).ready, EVENT_NOTIFICATION, false),         \
    }

static inline void event_latch_init(struct event_latch *el, uint64_t count) {
    once_latch_init(&el->latch, count);
    event_init(&el->ready, EVENT_NOTIFICATION, false);
}

static inline bool event_latch_count_down(struct event_latch *el) {
    if (once_latch_count_down(&el->latch)) {
        event_set(&el->ready);
        return true;
    }
    return false;
}

static inline bool event_latch_is_ready(const struct event_latch *el) {
    return once_latch_is_ready(&el->latch);
}

static inline uint64_t event_latch_count(const struct event_latch *el) {
    return once_latch_get_count(&el->latch);
}

static inline void event_latch_wait(struct event_latch *el) TSA_MAY_SLEEP {
    if (event_latch_is_ready(el))
        return;
    event_wait(&el->ready, THREAD_WAIT_UNINTERRUPTIBLE,
               THREAD_BLOCK_REASON_MANUAL);
}

static inline bool
event_latch_wait_timeout(struct event_latch *el,
                         time_ms_t timeout_ms) TSA_MAY_SLEEP {
    if (event_latch_is_ready(el))
        return true;
    return event_wait_timeout(&el->ready, THREAD_WAIT_UNINTERRUPTIBLE,
                              THREAD_BLOCK_REASON_MANUAL, timeout_ms)
               .status == THREAD_WAIT_SATISFIED;
}
