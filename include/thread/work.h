/* @title: Work(ers|queues)? */
#pragma once
#include <atomic.h>
#include <console/crash.h>
#include <structures/list.h>

/* Throughout much of the kernel, drivers want to defer work to threaded
 * contexts. However, we don't necessarily want to make one big overarching
 * "workqueue" API and try to tailor to every single possible consumer.
 *
 * We support fine-grained worker-scoped management. One major benefit
 * this brings is better determinism. Workqueues, in edge cases and tail
 * end latency scenarios can get their threads taken hostage by a blocking
 * work item.
 *
 * This would thus require either (1) a new thread to be spun up, which
 * in itself could introduce delays, or (2) an old worker to pick up
 * the work item and begin execution (after some time passes).
 *
 * This isn't great for latency, which is why we have a generic "worker" API
 * alongside workqueues, and workqueues become a consumer of workers.
 *
 * Drivers that require that their works don't remain waiting on hostage
 * threads can thus use a dedicated worker that has a single work item
 * that handles processing of all the various things the driver needs.
 *
 * Ideally, such workers should not scope-creep into workqueues
 * and be reserved for dedicated emergency things.
 */

/* worker_flags: 32 bit bitflags
 *
 *      ┌──────────────────────────────────────────────────────┐
 * Bits │ 31..28 27..24 23..20 19..16 15..12 11..8  7..4  3..0 │
 * Use  │  AAAA   AAAA   AAAA   AAAA   AAAA   AAAA  AAAA  AAAD │
 *      └──────────────────────────────────────────────────────┘
 *
 * D - Detached - Worker is not assigned to a queue
 *
 * A - Unused (available)
 * * - Unused (unavailable)
 *
 */
enum worker_flags {
    WORKER_FLAG_DETACHED = 1,

};

struct worker {
    struct list_node list_node;
    enum worker_flags flags;

    struct thread *thread;
    atomic(struct work *) work;

    struct workqueue *workqueue; /* NULL if detached */
};
