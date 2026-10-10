#include <atomic.h>
#include <irq/ipi.h>
#include <mem/alloc.h>
#include <mem/page.h>
#include <mem/tlb.h>
#include <sch/sched.h>
#include <smp/percpu.h>
#include <smp/perdomain.h>
#include <stdint.h>
#include <thread/dpc.h>
#include <time/tsc.h>

#include "tlb_internal.h"

static void tlb_cpu_init(struct tlb_cpu *cpu, cpu_id_t id);
static void tlb_domain_init(struct tlb_domain *cpu, domain_id_t id);

struct tlb_globals tlb_global = {0};
struct spinlock tlb_shootdown_lock = SPINLOCK_INIT;
PERDOMAIN_DEFINE(struct tlb_domain, tlb_domains, tlb_domain_init);
PERCPU_DEFINE(struct tlb_cpu, tlb_cpus, tlb_cpu_init);

static void tlb_cpu_init(struct tlb_cpu *cpu, cpu_id_t id) {
    tlb_queue_init(&cpu->eager_ring.queue, TLB_QUEUE_SIZE);
    tlb_queue_init(&cpu->lazy_ring.queue, TLB_QUEUE_SIZE);
    cpu->id = id;
}

static void tlb_domain_init(struct tlb_domain *d, domain_id_t id) {
    d->domain = global.domains[id];
    struct cpu_mask mask = CPU_MASK_INIT;
    domain_set_cpu_mask(&mask, d->domain);

    d->ipi_cost = ipi_send_cost(&mask);
}

static void payload_invalidate(struct tlb_payload *pl) {
    switch (pl->type) {
    case TLB_OP_FLUSH: tlb_flush(); break;
    case TLB_OP_PAGE: tlb_invlpg(pl->addr); break;
    case TLB_OP_RANGE:
        for (vaddr_t start = pl->range.low; start < pl->range.hi;
             start += pl->stride) {
            tlb_invlpg(start);
        }
    }
}

/* Checking once and then deciding is fine here. We only need
 * to see the snapshot of need_flush at the time of entry,
 * since that is what we are invalidating to respond to. */
static void ring_invalidate(struct tlb_ring *ring) {
    if (!atomic_xchg_acq_rel(&ring->need_flush, false)) {
        tlb_queue_drain(&ring->queue);
        tlb_flush();
    } else {
        struct tlb_queue_entry got;
        while (tlb_queue_dequeue(&ring->queue, &got))
            payload_invalidate(&got.payload);
    }
}

static void ring_enqueue(struct tlb_ring *ring, struct tlb_payload *pl) {
    struct tlb_queue_entry qent = {.payload = *pl};
    if (!tlb_queue_enqueue(&ring->queue, qent))
        atomic_store_release(&ring->need_flush, true);
}

static void tlb_shootdown_internal(void) {
    struct tlb_cpu *c = PERCPU_PTR(TOPC_IRQ, tlb_cpus);

    uint64_t done = atomic_load_relaxed(&c->done_gen);

    while (true) {
        uint64_t req = atomic_load_acq(&c->req_gen);

        if (done >= req)
            break;

        ring_invalidate(&c->eager_ring);

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
    cc_unused(ctx, irq, rsp);

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

        struct tlb_payload pl = {.addr = addr, .type = TLB_OP_PAGE};
        ring_enqueue(&PERCPU_PTR_FOR(tlb_cpus, i)->eager_ring, &pl);

        atomic_store_release(&PERCPU_PTR_FOR(tlb_cpus, i)->req_gen, gen);
        ipi_send(i, IRQ_TLB_SHOOTDOWN);
    }

    if (synchronous) {
        for_each_cpu_id(i) {
            if (i == this_cpu)
                continue;

            struct tlb_cpu *cpu = PERCPU_PTR_FOR(tlb_cpus, i);

            int spins = 0;

            while (atomic_load_acq(&cpu->done_gen) < gen) {
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

void tlb_init(void) {
    if (tsc_has_invariant()) {
        tlb_global.stamp_source = TLB_STAMP_TSC;
        tlb_global.stamp_guard =
            tsc_global.max_warp + tsc_global.min_rtt + TLB_STAMP_GUARD_SLACK;
    } else {
        tlb_global.stamp_source = TLB_STAMP_GEN;
        tlb_global.stamp_guard = 0;
    }
}
