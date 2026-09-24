#include <asm.h>
#include <irq/idt.h>
#include <kassert.h>
#include <sch/sched.h>
#include <smp/core.h>
#include <sync/rcu.h>
#include <thread/dpc.h>
#include <thread/workqueue.h>

void scheduler_idle_main(void *nop) {
    cc_unused(nop);
    struct scheduler *sched = global.schedulers[smp_id_raw()];

    while (true) {
        irq_disable();
        if (scheduler_mark_self_needs_resched(false) ||
            sched->total_thread_count > 0 ||
            sched->completed_rbt.root != NULL) {
            irq_enable();
            scheduler_yield();
            continue;
        }

        cpu_idle();
    }
}
