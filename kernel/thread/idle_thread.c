#include <asm.h>
#include <enforce.h>
#include <sch/global.h>
#include <sch/sched.h>
#include <sync/rcu.h>
#include <thread/workqueue.h>
#include <topo/core.h>
#include <topo/smp.h>

void scheduler_idle_main(void *nop) {
    cc_ignore(nop);
    struct scheduler *sched = sch_global.schedulers[smp_id_raw()];

    bool woken = false;

    while (true) {
        irq_disable();

        smp_boot_fence_park();

        rcu_idle_exit(); /* For potential NMI events */

        if (scheduler_mark_self_needs_resched(false) ||
            sched->total_thread_count > 0 ||
            sched->completed_rbt.root != NULL) {
            irq_enable();
            scheduler_yield();
            continue;
        }

        /* Go yield to try to pick up some work */
        if (woken && atomic_load(&scheduler_data.total_threads) > 0) {
            woken = false;
            irq_enable();
            scheduler_yield();
            continue;
        }

        rcu_idle_enter();
        cpu_idle();
        woken = true;
    }
}
