/* @title: TLB */
#pragma once
#include <atomic.h>
#include <mem/page.h>
#include <stdint.h>
#include <structures/mpmc_queue.h>
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
 * The idea here is that we scope at the IPI cluster scope.
 */

/* A type describes what an actual operation is */
enum tlb_op_type {
    TLB_OP_PAGE,  /* Invalidate just this one page */
    TLB_OP_RANGE, /* Invalidate this range */
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
};

MPMC_QUEUE_DECLARE(tlb_queue, struct tlb_queue_entry);

struct tlb_node {
    struct topology_node *topo_node;
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
