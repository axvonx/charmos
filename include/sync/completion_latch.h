/* @title: Countdown latch */
#pragma once
#include <sync/completion.h>
#include <sync/once_latch.h>
#include <time/time.h>

struct completion_latch {
    struct once_latch latch;
    struct completion comp;
};

#define COMPLETION_LATCH_INIT(name_, n, irq_dis)                               \
    (struct completion_latch) {                                                \
        .latch = ONCE_LATCH_INIT(n),                                           \
        .comp = COMPLETION_INIT((name_).comp, irq_dis),                        \
    }

static inline void completion_latch_init(struct completion_latch *cl,
                                         uint64_t count, bool irq_disable) {
    once_latch_init(&cl->latch, count);
    completion_init(&cl->comp, irq_disable);
}

static inline bool completion_latch_count_down(struct completion_latch *cl) {
    if (once_latch_count_down(&cl->latch)) {
        complete_all(&cl->comp);
        return true;
    }
    return false;
}

static inline bool
completion_latch_is_ready(const struct completion_latch *cl) {
    return once_latch_is_ready(&cl->latch);
}

static inline uint64_t
completion_latch_count(const struct completion_latch *cl) {
    return once_latch_count(&cl->latch);
}

static inline void completion_latch_wait(struct completion_latch *cl)
    TSA_EXCLUDED(IRQL_RAISED) {
    if (completion_latch_is_ready(cl))
        return;
    completion_wait(&cl->comp);
}

static inline bool completion_latch_wait_timeout(struct completion_latch *cl,
                                                 time_ms_t timeout_ms)
    TSA_EXCLUDED(IRQL_RAISED) {
    if (completion_latch_is_ready(cl))
        return true;
    return completion_wait_timeout(&cl->comp, timeout_ms);
}
