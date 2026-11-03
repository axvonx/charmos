/* @title: Work(ers|queues)? */
#pragma once
#include <atomic.h>
#include <console/crash.h>
#include <structures/list.h>
#include <sync/event.h>

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
 *
 * All workers will be marked joinable by default, so
 */

enum work_state { WORK_STATE_ };

/* worker_flags: 32 bit bitflags
 *
 * These flags are set once at worker creation, and never
 *
 *      ┌──────────────────────────────────────────────────────┐
 * Bits │ 31..28 27..24 23..20 19..16 15..12 11..8  7..4  3..0 │
 * Use  │  AAAA   AAAA   AAAA   AAAA   AAAA   AAAA  AAAA  AAAQ │
 *      └──────────────────────────────────────────────────────┘
 *
 * Q - Queue bound - Worker is attached to a workqueue.
 *
 *     Some special behavior occurs as a result of this. Notably, QUEUE_BOUND
 *     workers are able to change the work item they are currently running,
 *     whilst non-queue bound ones cannot.
 *
 *
 *
 * A - Unused (available)
 * * - Unused (unavailable)
 *
 */
enum worker_flags {
    WORKER_FLAG_QUEUE_BOUND = 1,
    WORKER_FLAG_EMERGENCY = 1 << 1,
    WORKER_FLAG_
};

enum worker_action {
    WORKER_ACTION_WAIT,
    WORKER_ACTION_EXIT,
    WORKER_ACTION_REPEAT,
};

/* NOTE: for now, worker priority is also fixed upon spawn.
 *
 * TODO: once I implement generic lock depth tracking and things, this should
 * be mutable, but until then, it'll be fixed from spawn to exit */
enum worker_priority {
    WORKER_PRIORITY_BG,
    WORKER_PRIORITY_TS_LOW,
    WORKER_PRIORITY_TS_MID,
    WORKER_PRIORITY_TS_HIGH,
    WORKER_PRIORITY_RT,
    WORKER_PRIORITY_URGENT,
};

struct worker {
    struct list_node queue_list_node;
    struct list_node worker_list_node;

    enum worker_flags flags;
    enum worker_priority prio;

    struct thread *thread;
    atomic(struct work *) work;

    struct workqueue *workqueue; /* NULL if not queue bound */
    struct event event;
};

struct workqueue {};
