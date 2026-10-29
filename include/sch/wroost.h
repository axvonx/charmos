/* @title: Writer to Reader Boosting */
#pragma once
#include <atomic.h>
#include <math/bit.h>
#include <sch/wroost_types.h>
#include <structures/bitmap.h>
#include <structures/cpu_mask_types.h>
#include <structures/hlist.h>
#include <sync/rwlock_types.h>
#include <thread/dpc.h>
#include <thread/thread_types.h>

struct scheduler;

/*
 * wroost "writer reader boost" portmanteau
 *
 * Implementing writer to reader boosting for rwlocks is quite tricky,
 * especially for turnstile-backed ones where you don't get extra data
 * to store lists of threads.
 *
 * Lock side stuff:
 *
 * Thus, this is a sort of lazy priority ceiling. The idea is this:
 * we store a hash table where each bucket has no hlist, but rather,
 * keeps a set of 4 16-bit counters and a CPU mask.
 *
 * We hash every lock to store a small 'counting bloom filter'
 * within each thread. We also keep a max_boost which is the
 * maximum of all rwlock's maximum priorities to mitigate excess
 * boosting that could be pathological.
 *
 * Then, writers hash into the table and bump the counter that
 * corresponds to their perceived priority.
 *
 * Scheduler side stuff:
 *
 * Every CPU stores 256 x 16 bit counters that indicate the number of threads
 * with a given ID (coarse entries are fanned across their respective
 * sub-buckets) read-locked.
 *
 * When one of these counters passes the 0 <-> 1 transition boundary, it
 * sets/clears a CPU mask in the corresponding global hash table bucket.
 *
 * Migrations also alter the CPU masks if they cross a transition boundary,
 * and do so by first setting the new bit and then clearing the old one.
 *
 *
 *
 *
 *
 */

/* Thread data */
#define WROOST_BITS_PER_COUNTER 2
#define WROOST_THREAD_MAP_DEPTH 32
#define WROOST_COUNTER_MAX 0x3
#define WROOST_COUNTER_MASK 0x3

#define WROOST_MIN_BUCKET_COUNT 256U
#define WROOST_MAX_BUCKET_COUNT 32768U
#define WROOST_BUCKETS_PER_CPU_MULT 16U
#define WROOST_FINE_VALID_MASK ((1U << WROOST_FINE_ENTRY_COUNT) - 1U)

/* Percpu data */
#define WROOST_THREAD_MAP_OVERFLOW ((struct thread *) (UINTPTR_MAX))

static_assert(BITS_NEEDED(WROOST_THREAD_COUNTERS - 1) == 5);

static_assert(BIT_SIZEOF(wroost_id_t) >= BITS_NEEDED(WROOST_THREAD_COUNTERS),
              "wroost_id_t needs to be wider");

enum wroost_state {
    WROOST_DPC_IDLE = 0,
    WROOST_DPC_PENDING = 1,
    WROOST_DPC_ACTIVE = 1 << 1,
};

struct wroost_process_result {
    bool restart;

    /* seen_data marks the lock IDs that the code has already
     * processed, at the bitmap word granularity. This is so
     * that if the pending process needs to restart, it
     * remains aware of what had already been processed, so
     * it can merge it with potentially new bits
     * and re-check and boost (since restart happens
     * due to a writer boosting a reader on calling CPU) */
    bitmap_word_t *seen_data;
};

/* Protected by scheduler->lock */
struct wroost_percpu_data {
    atomic(enum wroost_state) state;
    cpu_id_t id;

    /* This keeps track of the number of READY threads on this CPU's
     * runqueues of every given lock ID. Notably, it does NOT
     * keep track of RUNNING threads OR threads that were going to block.
     *
     * Upon switch in, if a thread has any rwlocks held as a reader,
     * it "contributes", and when leaving, it "withdraws" its counts
     * added.
     *
     * However, the threads only contribute 1 per lock maximum,
     * even if its 2-bit counter is more than 1. The intent here is to
     * simply track "any thread", not "exact counter", so we only
     * need every thread to contribute 1 for the count if it has
     * that lock ID held as a reader.
     *
     * We need this because this is what allows us to make the
     * 0 <-> 1 switch for the bitmap per-lock-ID
     */
    uint8_t *rq_bucket_counts;

    /* Not actually a hashmap, it's just the number of counters,
     * where precise entries get their coarse entry counter */
    struct hlist_head thread_map[WROOST_THREAD_COUNTERS];

    /* Since thread_map is guaranteed to only have single lock readers */
    struct hlist_head multi_lock_readers;

    /* Readers that happen to be blocked on a turnstile go here. This is
     * because such readers need to propagate their priorities, which
     * cannot happen under sched->lock, but can under wroost_lock (and
     * that is the sole purpose that lock actually exists).
     *
     * Readers enter blocked_ts_readers on switch-out,
     * and exit it on switch-in, both of which cannot happen during
     * the DPC that iterates through this.
     *
     * Migrations are not a concern here because these threads do not
     * exist in the runqueues, and there is thus no way to have
     * obtained them for a migration. These threads must also
     * be woken on the CPU that they went to sleep on for this reason. */
    struct hlist_head blocked_ts_readers;

    /* Set on switch-out when blocked_ts_reader gets boosted */
    bool ts_requeue_pending;
    atomic(bitmap_word_t) * pending_bitmap;

    struct wroost_process_result process_result;

    /* These primarily serve the purpose of implementing multi-hop.
     *
     * In the DPC, some readers are also writers, and upon a priority change,
     * they will also need to go bother relevant readers. However, this all
     * happens deep down in the scheduler lock, so DPCs can't be
     * arbitrarily sent (deadlock). Thus, we merge all the CPU masks into
     * a single one, and set bits in in_dpc_deferred_ids. The idea
     * is that merging all of this together means that we may
     * unnecessarily IPI some CPUs, but we will never have a false negative,
     * and in the common case, this won't be that bad since only a lock
     * or two of this kind will be held */
    bitmap_word_t *in_dpc_deferred_ids; /* Bits represent lock IDs */
    struct cpu_mask in_dpc_deferred_cpus;

    struct dpc boost_dpc;
    struct cpu_mask dpc_enqueue_deferred;
    wroost_id_t writer_lock_id; /* So we know what to set for deferred DPCS */
};

ct_assert_struct_size_eq(wroost_thread_data, 40);

struct wroost_bucket {
    /* "tally", so as to not conflict with "counter":
     *
     * 16-bit 2 * prio_class offset counters of owner prios */
    atomic_uint64_t raw_tally;

    /* 2 bits per CPU for 2 bits per prio class */

    /* clang-format is very eager to format this as a pointer (??)
     * TODO: Maybe this is a bug upstream */

    /* clang-format off */
    BITMAP_DECLARE_ATOMIC(rq_min_prio, THREAD_PRIO_CLASS_BIT_COUNT * CPU_MASK_BITS);
    /* clang-format on */
};

struct wroost_hash_table {
    struct wroost_bucket *buckets cc_counted_by(bucket_count);
    size_t bucket_count;
};

enum thread_prio_class wroost_compute_ceil(struct wroost_thread_data *wtd,
                                           enum thread_prio_class min);
void wroost_pre_acquire(struct rwlock *lock, enum rwlock_acquire_type type);
void wroost_post_acquire(struct rwlock *lock, enum rwlock_acquire_type type);
void wroost_post_release(struct rwlock *lock, enum rwlock_acquire_type type);
/* Separate from switch_in. this runs deferred DPC
 * notifications/IPIs onto the other CPUs */
void wroost_scheduler_switch_in_hook(void);
void wroost_switch_in(struct thread *t);

void wroost_switch_out(struct thread *t);
void wroost_pre_wait(struct thread *curr);
void wroost_post_wait(struct thread *curr);
void wroost_post_migrate_hook(struct thread *t, cpu_id_t from, cpu_id_t to);

size_t wroost_kick(struct wroost_kicks *kicks);
bool wroost_merge_kicks(struct wroost_percpu_data *pcd,
                        struct wroost_kicks *kicks);

struct wroost_kicks cc_must_check
wroost_post_prio_change_locked(struct thread *t, enum thread_prio_class old,
                               enum thread_prio_class new);

void wroost_init(void);
void wroost_scheduler_init(struct scheduler *sched);
