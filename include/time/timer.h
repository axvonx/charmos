/* @title: Timer */
#pragma once
#include <structures/hlist.h>
#include <time/time.h>
#include <time/timer_types.h>
#include <types/types.h>

struct irq_context;
void timer_add(struct timer *timer);
void timer_add_on(struct timer *timer, cpu_id_t cpu);

/* Wrappers around `add()` with flag setting */
void timer_add_local(struct timer *timer);
void timer_add_global(struct timer *timer);

bool timer_modify(struct timer *timer, time_us_t new);
bool timer_modify_pending(struct timer *timer, time_us_t new);

/* Only modify if `new` is nearer than `expiration` */
bool timer_modify_reduce(struct timer *timer, time_us_t new);

/* `_sync()` functions wait for the handler to finish,
 * and non-`_sync()` functions do not */
bool timer_delete(struct timer *timer);
bool timer_delete_sync(struct timer *timer);
bool timer_shutdown(struct timer *timer);
bool timer_shutdown_sync(struct timer *timer);

enum irq_result timer_isr(struct irq_context *ctx, void *c);
void timers_init_bsp(void);
void timers_init_ap(cpu_id_t cpu);

static inline void timer_bucket_set(struct timer *timer, uint32_t bucket) {
    timer->flags =
        (timer->flags & ~(TIMER_FLAG_BUCKET_MASK << TIMER_FLAG_BUCKET_SHIFT)) |
        TIMER_FLAG_BUCKET(bucket);
}

static inline uint32_t timer_bucket_get(struct timer *timer) {
    return (timer->flags >> TIMER_FLAG_BUCKET_SHIFT) & TIMER_FLAG_BUCKET_MASK;
}

static inline void timer_cpu_set(struct timer *timer, cpu_id_t cpu) {
    timer->flags = (timer->flags & ~TIMER_FLAG_CPU_MASK) | TIMER_FLAG_CPU(cpu);
}

static inline cpu_id_t timer_cpu_get(struct timer *timer) {
    return timer->flags & TIMER_FLAG_CPU_MASK;
}

static inline void timer_init(struct timer *timer,
                              void (*func)(struct timer *)) {
    timer->hlist_node.next = NULL;
    timer->hlist_node.pprev = NULL;
    timer->expiration_us = 0;
    timer->func = func;
    timer->flags = TIMER_FLAG_NONE;
}

static inline time_us_t timer_delta_us(time_us_t delta_us) {
    return time_get_us() + delta_us;
}
