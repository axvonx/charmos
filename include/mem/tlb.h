/* @title: TLB */
#include <atomic.h>
#include <mem/page.h>
#include <stdint.h>
#include <types/types.h>

/* per-cpu */
#define TLB_QUEUE_SIZE 64

/*
 * The way our TLB shootdowns work is that we have a two tiered queue:
 *
 * (1) Eager
 * (2) Lazy
 *
 * We build TLB queues that correspond to the processor struct domains,
 * which allow us to shard the shootdowns by having a domain
 * shootdown queue and a per-CPU one.
 *
 * Because TLB invalidations
 * are idempotent, it is fine to execute more, but executing
 * less leads to an invalid view of the page tables.
 *
 * However, there is also a consideration with domain-level
 * shootdowns: if domain 1 has cpu_mask 0000 0000 1111 1111,
 * and there is a shootdown with a cpu_mask of 0000 0000 1110 1111,
 * there is a case to be made that bothering CPU 11 with an IPI (or not,
 * in the case of a lazy shootdown), is fine.
 *
 * Of course, there is also the counter-case of how issuing many
 * shootdowns to a CPU that doesn't need it (e.g. CPU 11 in this example)
 * could cause more contention and latency.
 *
 * To solve this, we simply perform a check: if a given CPU mask
 * has
 */

/* A type describes what an actual operation is */
enum tlb_op_type {
    TLB_OP_PAGE,  /* Invalidate just this one page */
    TLB_OP_RANGE, /* Invalidate this range */
    TLB_OP_BATCH, /* Invalidate this scattered list */
    TLB_OP_FLUSH  /* Completely flush everything */
};

/* Modes specify how to deliver. SYNC and ASYNC are eager */
enum tlb_request_mode {
    TLB_REQUEST_SYNC,  /* Send the IPI out, wait for responses */
    TLB_REQUEST_ASYNC, /* Fire and forget */
    TLB_REQUEST_LAZY   /* Tell the other CPUs about it, but no enqueues */
};

struct tlb_payload {
    enum tlb_op_type type;
    union {
        vaddr_t vaddr;
        struct {
            vaddr_t hi, lo;
        };
    };
};

struct tlb_request {
    enum tlb_request_mode mode;
    struct cpu_mask targets;
    struct tlb_payload payload;
};

struct tlb_queue_entry {
    struct tlb_payload payload;
    atomic_uint64_t seq;
};

struct tlb_queue {};

struct tlb_node {
    struct domain *domain;
    struct tlb_queue eager_queue;
    struct tlb_queue lazy_queue;
};

struct tlb_shootdown_cpu {
    atomic_uintptr_t queue[TLB_QUEUE_SIZE];
    atomic_uint32_t head;
    atomic_uint32_t tail;
    atomic_bool in_tlb_shootdown;
    atomic_uint8_t flush_all;
    atomic_uint64_t req_gen;  /* last requested generation */
    atomic_uint64_t done_gen; /* last completed generation */
};

void tlb_init(void);
enum irq_result tlb_shootdown_isr(void *ctx, irq_t irq,
                                  struct irq_context *rsp);
void tlb_shootdown(uintptr_t addr, bool synchronous);
