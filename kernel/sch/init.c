#include <atomic.h>
#include <console/panic.h>
#include <irq/irq.h>
#include <mem/alloc.h>
#include <mem/nofail.h>
#include <sch/climb.h>
#include <sch/global.h>
#include <sch/sched.h>
#include <sch/wroost.h>
#include <structures/cpu_mask.h>
#include <structures/list.h>
#include <structures/rbt.h>
#include <sync/lock_chk.h>
#include <sync/spinlock.h>
#include <thread/thread.h>
#include <thread/thread_types.h>
#include <time/time.h>
#include <time/timer.h>
#include <topo/core.h>
#include <topo/global.h>
#include <types/types.h>

#include "internal.h"

struct sch_globals sch_global = {0};

enum irq_result scheduler_reschedule_isr(struct irq_context *rsp, void *ctx) {
    scheduler_mark_self_needs_resched(true);
    cc_ignore(ctx, rsp);
    return IRQ_HANDLED;
}

static size_t scheduler_thread_get_data(struct rbt_node *n) {
    return thread_from_rq_rbt_node(n)->virtual_runtime_left;
}

static int32_t scheduler_cmp_threads(const struct rbt_node *a,
                                     const struct rbt_node *b) {
    int64_t vrla = thread_from_rq_rbt_node(a)->virtual_runtime_left;
    int64_t vrlb = thread_from_rq_rbt_node(b)->virtual_runtime_left;
    return (vrla > vrlb) - (vrla < vrlb);
}

static void scheduler_tick(struct timer *t) {
    struct scheduler *self = smp_core_scheduler();
    scheduler_mark_self_needs_resched(true);

    if (scheduler_tick_enabled(self)) {
        timer_modify(t, timer_delta_us(MS_TO_US(self->tick_duration_ms)));
    }
}

void scheduler_init(void) {
    scheduler_data.max_concurrent_stealers = topo_global.cpu_count / 4;

    /* I mean, if we have one core and that core wants
     * to steal work from itself, go ahead? */
    if (scheduler_data.max_concurrent_stealers == 0)
        scheduler_data.max_concurrent_stealers = 1;

    sch_global.schedulers = vnews(sch_global.schedulers, topo_global.cpu_count);
    if (!sch_global.schedulers)
        panic(OUT_OF_MEMORY, "scheduler pointer array");

    wroost_init();
    size_t i;
    for_each_cpu_id(i) {
        struct scheduler *s = nofail_vnew(s, ALLOC_ZERO);

        spinlock_init(&s->lock, LOCK_UNCHKD);
        spinlock_init(&s->wroost_lock);
        rbt_init(&s->thread_rbt, scheduler_thread_get_data,
                 scheduler_cmp_threads);
        rbt_init(&s->completed_rbt, scheduler_thread_get_data,
                 scheduler_cmp_threads);
        rbt_init(&s->climb_threads, climb_get_thread_data, climb_cmp_threads);
        atomic_store_rlx(&s->tick_enabled, false);
        s->current_period = 1; /* Start at period 1 to avoid
                                * starting at 0 because
                                * that would lead to threads
                                * being mistakenly identified
                                * as completed */

        cpu_mask_clear_all(&s->deferred_resched_mask);
        s->core_id = i;
        s->tick.func = scheduler_tick;
        s->tick.flags =
            TIMER_FLAG_IRQ | TIMER_FLAG_CPU(s->core_id) | TIMER_FLAG_PINNED;
        struct thread *idle_thread = thread_create(
            ("idle_thread_%zu", i), scheduler_idle_main, .pinned = true);
        atomic_store_rlx(&idle_thread->state, THREAD_STATE_IDLE_THREAD);
        s->idle_thread = idle_thread;

        wroost_scheduler_init(s);
        list_head_init(&s->rt_threads);
        list_head_init(&s->urgent_threads);
        list_head_init(&s->bg_threads);

        if (!i) {
            struct thread *t =
                thread_create("main_thread", k_sch_main, .pinned = true);
            scheduler_add_thread(s, t);
        }

        sch_global.schedulers[i] = s;
    }
}
