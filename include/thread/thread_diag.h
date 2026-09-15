/* @title: Per-thread diagnostics */
#pragma once
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <thread/thread_types.h>
#include <time/time.h>

struct thread;

#define THREAD_WAIT_TRACE_DEPTH 16

struct thread_wait_arm {
    const char *site;     /* which arming path */
    void *ra;             /* caller of arm */
    void *object;         /* first registered object (NULL for park) */
    uint64_t first_epoch; /* wait epoch when this descriptor was installed */
    uint64_t last_epoch;  /* wait epoch of the most recent identical arm */
    uint64_t repeats;     /* consecutive arms carrying this descriptor */
    time_ms_t last_ms;
    uint8_t state;              /* enum thread_state armed into */
    uint8_t wait_type;          /* enum thread_wait_type armed with */
    thread_act_reason_t reason; /* block/sleep reason byte */
    uint16_t block_count;
};

struct thread_diag {
    struct thread_wait_arm wait_trace[THREAD_WAIT_TRACE_DEPTH];
    uint64_t wait_arm_count;
    uint64_t wait_arm_total;

    uint64_t apc_deliver_entries; /* calls to deliver_apc_type() */
    uint64_t apc_deliver_max;     /* most APCs drained by a single call */
    void *apc_last_deliver_ra;    /* who called apc_check_and_deliver() */
};

bool thread_diag_attach(struct thread *t, struct thread_diag *d);

void thread_diag_detach(struct thread *t);

struct thread_diag *thread_diag(struct thread *t);

void thread_diag_record_arm(struct thread *t, const char *site, void *ra,
                            void *object, enum thread_state state,
                            enum thread_wait_type type, uint8_t reason);

void thread_diag_apc_deliver_enter(struct thread *t, void *ra);
void thread_diag_apc_deliver_batch(struct thread *t, uint64_t drained);

void thread_dump_wait_trace(struct thread *t, const char *role, size_t idx,
                            uint64_t max_arms);
