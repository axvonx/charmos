#include <acpi/lapic.h>
#include <atomic.h>
#include <mem/alloc.h>
#include <mem/page.h>
#include <mem/tlb.h>
#include <sch/sched.h>
#include <stdint.h>
#include <thread/dpc.h>

struct spinlock tlb_shootdown_lock = SPINLOCK_INIT;

static void tlb_shootdown_internal(void) {
    size_t cpu = smp_id(TOPC_IRQ);
    struct tlb_shootdown_cpu *c = &global.shootdown_data[cpu];

    uint64_t done = atomic_load_relaxed(&c->done_gen);

    while (true) {
        uint64_t req = atomic_load_acq(&c->req_gen);

        if (done >= req)
            break;

        for (;;) {
            uint32_t tail = atomic_load_relaxed(&c->tail);
            uint32_t head = atomic_load_acq(&c->head);

            if (tail == head)
                break;

            while (tail != head) {
                uintptr_t addr =
                    atomic_load_acq(&c->queue[tail & (TLB_QUEUE_SIZE - 1)]);

                if (addr)
                    tlb_invlpg(addr);

                tail++;
            }

            atomic_store_release(&c->tail, tail);
        }

        if (atomic_xchg_acq_rel(&c->flush_all, false)) {
            tlb_flush();
            uint32_t h = atomic_load_acq(&c->head);
            atomic_store_release(&c->tail, h);
        }

        /* A drain satisfies each gen up to the `req` we read before
         * draining. Stepping one at a time makes cost of ack ~ prop to
         * how many shootdowns the rest of the machine had done, which
         * causes some larger slowdowns */
        done = req;
        atomic_store_release(&c->done_gen, done);
    }
}

enum irq_result tlb_shootdown_isr(void *ctx, irq_t irq,
                                  struct irq_context *rsp) {
    cc_var_unused(ctx, irq, rsp);

    tlb_shootdown_internal();
    return IRQ_HANDLED;
}

void tlb_shootdown(uintptr_t addr, bool synchronous) {
    if (global.current_bootstage < BOOTSTAGE_MID_MP)
        return;

    /* TODO: scale up */
    enum irql lirql = spin_lock(&tlb_shootdown_lock);

    uint64_t gen = atomic_inc_return_relaxed(&global.next_tlb_gen);

    size_t this_cpu = smp_id(TOPC_IRQL);

    size_t i;
    for_each_cpu_id(i) {
        if (i == this_cpu) {
            tlb_invlpg(addr);
            continue;
        }

        struct tlb_shootdown_cpu *t = &global.shootdown_data[i];

        uint32_t head = atomic_load_relaxed(&t->head);
        uint32_t tail = atomic_load_acq(&t->tail);

        if ((head - tail) >= TLB_QUEUE_SIZE) {
            atomic_store_release(&t->flush_all, true);
        } else {
            atomic_store_release(&t->queue[head & (TLB_QUEUE_SIZE - 1)], addr);
            atomic_store_release(&t->head, head + 1);
        }

        atomic_store_release(&t->req_gen, gen);
        ipi_send(i, IRQ_TLB_SHOOTDOWN);
    }

    if (synchronous) {
        for_each_cpu_id(i) {
            if (i == this_cpu)
                continue;

            struct tlb_shootdown_cpu *o = &global.shootdown_data[i];

            int spins = 0;

            while (atomic_load_acq(&o->done_gen) < gen) {
                if (spins < 100) {
                    cpu_pause();
                    spins++;
                    continue;
                }

                spins = 0;
                ipi_send(i, IRQ_TLB_SHOOTDOWN);
            }
        }

        atomic_inc_release(&global.pt_epoch);
    }

    spin_unlock(&tlb_shootdown_lock, lirql);
}
