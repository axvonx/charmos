/* @title: DPCs */
#pragma once
#include <atomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct dpc;
typedef void (*dpc_func_t)(void *ctx);

struct dpc {
    dpc_func_t func;
    void *ctx;
    atomic(struct dpc *) next; /* for MPSC push */
    atomic_bool enqueued;      /* prevents double-enqueue */
};

struct dpc_queue {
    atomic(struct dpc *) head;
    atomic_size_t count;
};

/* Per-cpu DPC data */
struct dpc_cpu {
    atomic_bool ipi_queued;
    struct dpc_queue queue;
};

void dpc_drain_local(void);
void dpc_run_local(void);
void dpc_run_dpcs_from_irq(void);
struct dpc *dpc_create(dpc_func_t fn, void *ctx);
struct dpc *dpc_init(struct dpc *d, dpc_func_t fn, void *ctx);
void dpc_init_percpu(void);
bool dpc_enqueue_local(struct dpc *d);
bool dpc_enqueue_on_cpu(size_t cpu, struct dpc *d);
