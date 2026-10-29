#include <atomic.h>
#include <compiler/intrinsic.h>
#include <container_of.h>
#include <sch/irql.h>
#include <sch/sched.h>
#include <sch/wroost.h>
#include <string.h>
#include <structures/list.h>
#include <sync/spinlock.h>
#include <thread/apc.h>
#include <thread/thread.h>
#include <thread/thread_diag.h>
#include <thread/thread_types.h>
#include <thread/wait.h>
#include <time/time.h>
#include <time/timer.h>
#include <topo/core.h>
#include <topo/topology.h>

#include "sch/internal.h"

/* Lock ordering here:
 *
 * wait_lock -> one wait_header.lock at a time, runqueues -> thread
 *
 * APCs can run a little bit when we're going to complete a wait start,
 * and alerts only unlink interruptible registrations
 */
static inline uint64_t wait_epoch(struct thread *t) {
    return atomic_load_rlx(&t->wait_epoch);
}

static void unlink_blocks(struct thread *t, uint16_t got) {
    for (size_t i = 0; i < t->wait_block_count; i++) {
        struct thread_wait_block *b = &t->active_wait_blocks[i];

        if (b->state != THREAD_WAIT_BLOCK_ACTIVE)
            continue;

        enum irql irql = spin_lock_high(&b->header->lock);

        list_node_delete_init(&b->obj_node);
        b->state =
            i == got ? THREAD_WAIT_BLOCK_SATISFIED : THREAD_WAIT_BLOCK_INACTIVE;

        spin_unlock(&b->header->lock, irql);
    }
}

static inline enum thread_resume_reason manual_reason(struct thread *t) {
    return t->wait_state == THREAD_STATE_SLEEPING
               ? THREAD_WAKE_REASON_SLEEP_MANUAL
               : THREAD_WAKE_REASON_BLOCKING_MANUAL;
}

static void deliver_alert_locked(struct thread *t) {
    if (!t->alert_pending || !t->active_wait_blocks || t->wait_done ||
        atomic_load_rlx(&t->wait_type) != THREAD_WAIT_INTERRUPTIBLE)
        return;

    unlink_blocks(t, THREAD_WAIT_KEY_NONE);

    t->alert_pending = false;
    t->wait_done = true;
    t->wait_status = THREAD_WAIT_ALERTED;
    t->wait_key = THREAD_WAIT_KEY_NONE;
    t->wait_completion_reason = manual_reason(t);

    scheduler_complete_object_wait(t, t->wait_completion_reason);
}

/* If an object already got signaled at registration,
 * we'll need to shrink the count so unlink_blocks
 * doesn't read the rest, because only blocks [0, key)
 * would be linked right now */
static void satisfy_at_prepare(struct thread *t, uint16_t key) {
    SPINLOCK_ASSERT_HELD(&t->wait_lock);
    t->active_wait_blocks[key].state = THREAD_WAIT_BLOCK_INACTIVE;
    t->wait_block_count = key + 1;
    unlink_blocks(t, key);
    t->active_wait_blocks[key].state = THREAD_WAIT_BLOCK_SATISFIED;

    t->wait_key = key;
    t->wait_done = true;
    t->wait_status = THREAD_WAIT_SATISFIED;
    t->wait_completion_reason = manual_reason(t);

    scheduler_complete_object_wait(t, t->wait_completion_reason);
}

static void prepare_wait(const struct thread_wait_object *objects, size_t count,
                         struct thread_wait_block *storage,
                         enum thread_wait_type type, thread_act_reason_t reason,
                         enum thread_state state) {
    enforce_precond(count <= UINT16_MAX, CAPACITY_EXCEEDED);
    enforce_precond(type != THREAD_WAIT_NONE, "wait type must be provided");
    enforce_precond(storage || count <= THREAD_NUM_WAIT_BLOCKS,
                    CAPACITY_EXCEEDED);

    struct thread *t = thread_get_current();
    if (!storage)
        storage = t->wait_blocks; /* TODO: establish identities for the 4
                                   * wait blocks, we'll take 0 for now */

    enum irql irql = spin_lock_high(&t->wait_lock);
    enforce(!t->active_wait_blocks);
    t->active_wait_blocks = storage;
    t->wait_block_count = count;
    t->wait_key = THREAD_WAIT_KEY_NONE;
    t->wait_done = false;
    t->wait_status = THREAD_WAIT_SATISFIED;
    atomic_inc_rlx(&t->wait_epoch);

    t->wait_state = state;

    enum irql tirql = spin_lock_high(&t->lock);

    enforce(!t->object_wait);
    t->object_wait = true;

    atomic_store_rlx(&t->wait_type, type);
    thread_clear_flag(t, THREAD_FLAG_YIELDED);

    thread_diag_record_arm(t, "wait_blocks", ci_return_address(0),
                           count ? objects[0].object : NULL, state, type,
                           reason);

    /* Registration visible now, but we need to stay runnable so callers
     * can do their work before completeing */
    if (state == THREAD_STATE_SLEEPING) {
        thread_add_sleep_reason(t, reason);
    } else {
        thread_add_block_reason(t, reason);
    }

    thread_update_runtime_buckets(t, time_get_ms());

    spin_unlock(&t->lock, tirql);

    for (size_t i = 0; i < count; i++) {
        struct thread_wait_block *b = &storage[i];
        const struct thread_wait_object *obj = &objects[i];

        enforce(obj->header);
        *b = (struct thread_wait_block){.thread = t,
                                        .object = obj->object,
                                        .key = i,
                                        .state = THREAD_WAIT_BLOCK_ACTIVE,
                                        .header = objects[i].header};

        /* We put the checking and linking into one section here so
         * signalers are only able to see us linked or we see their signal */
        enum irql hirql = spin_lock_high(&b->header->lock);

        bool acquired = obj->try_acquire && obj->try_acquire(obj->object);

        if (!acquired)
            list_head_push_back(&b->header->waiters, &b->obj_node);

        spin_unlock(&b->header->lock, hirql);

        if (acquired) {
            satisfy_at_prepare(t, i);
            break;
        }
    }

    deliver_alert_locked(t);
    spin_unlock(&t->wait_lock, irql);
}

void thread_wait_prepare(const struct thread_wait_object *objs, size_t count,
                         struct thread_wait_block *storage,
                         enum thread_wait_type type,
                         enum thread_block_reason reason) {
    prepare_wait(objs, count, storage, type, reason, THREAD_STATE_BLOCKED);
}

void thread_wait_prepare_to_sleep(struct thread_wait_header *hdr, void *object,
                                  enum thread_wait_type type) {
    const struct thread_wait_object obj = {.header = hdr, .object = object};
    prepare_wait(&obj, /* count = */ 1, NULL, type, THREAD_SLEEP_REASON_MANUAL,
                 THREAD_STATE_SLEEPING);
}

void thread_wait_prepare_one(struct thread_wait_header *hdr, void *object,
                             enum thread_wait_type type,
                             enum thread_block_reason reason) {
    const struct thread_wait_object obj = {.header = hdr, .object = object};
    thread_wait_prepare(&obj, /* count = */ 1, NULL, type, reason);
}

static bool satisfy(struct thread *t, uint64_t epoch, uint16_t key,
                    enum thread_resume_reason reason,
                    void (*callback)(struct thread *)) {
    if (!t->active_wait_blocks || wait_epoch(t) != epoch ||
        key >= t->wait_block_count ||
        t->active_wait_blocks[key].state != THREAD_WAIT_BLOCK_ACTIVE)
        return false;

    SPINLOCK_ASSERT_HELD(&t->wait_lock);
    unlink_blocks(t, key);
    t->wait_key = key;
    t->wait_done = true;
    t->wait_status = THREAD_WAIT_SATISFIED;
    t->wait_completion_reason = reason;

    if (callback)
        callback(t);

    scheduler_complete_object_wait(t, reason);
    return true;
}

bool thread_wait_satisfy(struct thread_wait_block *b,
                         enum thread_resume_reason reason,
                         void (*cb)(struct thread *)) {
    struct thread *t = b->thread;

    enum irql irql = spin_lock_high(&t->wait_lock);

    uint16_t key = b->key;

    /* Has to be a block in the thread's storage */
    enforce(t->active_wait_blocks);
    enforce(b->state != THREAD_WAIT_BLOCK_SATISFIED);
    enforce(b == &t->active_wait_blocks[key]);

    bool curr = key < t->wait_block_count && b == &t->active_wait_blocks[key];

    bool won = curr && satisfy(t, wait_epoch(t), key, reason, cb);

    spin_unlock(&t->wait_lock, irql);

    return won;
}

bool thread_wait_satisfy_epoch(struct thread_wait_block *b, uint64_t epoch,
                               enum thread_resume_reason reason,
                               void (*callback)(struct thread *)) {
    struct thread *t = b->thread;

    enum irql irql = spin_lock_high(&t->wait_lock);
    bool won = satisfy(t, epoch, b->key, reason, callback);
    spin_unlock(&t->wait_lock, irql);

    return won;
}

struct thread *thread_wait_header_satisfy_one(struct thread_wait_header *hdr,
                                              enum thread_resume_reason reason,
                                              void (*callback)(struct thread *),
                                              void (*empty_locked)(void *),
                                              void *arg) {
    while (true) {
        enum irql irql = spin_lock_high(&hdr->lock);

        if (list_head_empty(&hdr->waiters)) {
            if (empty_locked)
                empty_locked(arg);

            spin_unlock(&hdr->lock, irql);
            return NULL;
        }

        struct thread_wait_block *b =
            list_first_entry(&hdr->waiters, struct thread_wait_block, obj_node);

        /* b is linked, this is b's wait */
        struct thread *t = b->thread;
        uint64_t epoch = wait_epoch(t);
        uint16_t key = b->key;

        enforce(thread_getref(t), STALE_REFERENCE);
        spin_unlock(&hdr->lock, irql);

        irql = spin_lock_high(&t->wait_lock);
        bool won = satisfy(t, epoch, key, reason, callback);
        spin_unlock(&t->wait_lock, irql);

        thread_putref(t);
        if (won)
            return t;
    }
}

struct thread *thread_wait_header_satisfy(struct thread_wait_header *hdr,
                                          enum thread_resume_reason reason,
                                          void (*callback)(struct thread *)) {
    return thread_wait_header_satisfy_one(hdr, reason, callback, NULL, NULL);
}

size_t thread_wait_header_satisfy_all(struct thread_wait_header *hdr,
                                      enum thread_resume_reason reason,
                                      void (*cb)(struct thread *),
                                      void (*before_lock)(void *), void *arg) {
    struct list_head snapshot;
    list_head_init(&snapshot);

    enum irql irql = spin_lock_high(&hdr->lock);

    if (before_lock)
        before_lock(arg);

    list_head_splice_back_init(&snapshot, &hdr->waiters);

    /* We have to still keep hdr->lock held until unlock so
     * other unlinks are correct while snapshot is on our stack */
    size_t woken = 0;
    while (!list_head_empty(&snapshot)) {

        struct thread_wait_block *b =
            list_first_entry(&snapshot, struct thread_wait_block, obj_node);

        struct thread *t = b->thread;
        uint64_t epoch = wait_epoch(t);
        uint16_t key = b->key;

        list_node_delete_init(&b->obj_node);
        bool last = list_head_empty(&snapshot);

        enforce(thread_getref(t), STALE_REFERENCE);
        spin_unlock(&hdr->lock, irql);

        enum irql wirql = spin_lock_high(&t->wait_lock);

        if (satisfy(t, epoch, key, reason, cb))
            woken++;

        spin_unlock(&t->wait_lock, wirql);
        thread_putref(t);

        if (last)
            return woken;

        irql = spin_lock_high(&hdr->lock);
    }

    spin_unlock(&hdr->lock, irql);
    return woken;
}

static void finish_locked(struct thread *t) {
    t->active_wait_blocks = NULL;
    t->wait_block_count = 0;

    enum irql irql = spin_lock_high(&t->lock);

    t->object_wait = false;
    atomic_store_rlx(&t->wait_type, THREAD_WAIT_NONE);
    enum thread_state state = thread_get_state(t);

    if (state == THREAD_STATE_BLOCKED || state == THREAD_STATE_SLEEPING)
        thread_set_state(t, THREAD_STATE_RUNNING);

    spin_unlock(&t->lock, irql);
}

struct thread_wait_result thread_wait_complete(void) {
    struct thread *t = thread_get_current();

    /* Whether the previous loop ran any APCs. We need this because
     * a delivery pass would leave the thread RUNNING. The
     * pass after one that delivers has to go actually block. */
    bool delivered = false;

    while (true) {
        enum irql irql = spin_lock_high(&t->wait_lock);
        enforce(t->active_wait_blocks);

        if (t->wait_done) {
            struct thread_wait_result result = {.status = t->wait_status,
                                                .key = t->wait_key,
                                                .reason =
                                                    t->wait_completion_reason};
            finish_locked(t);

            /* This is where we tunnel out of the wait.
             *
             * TODO: I really need to consider refactoring these random odd
             * hooks into a set of generic functions that I can gradually expand */
            wroost_post_wait(t);
            spin_unlock(&t->wait_lock, irql);
            return result;
        }

        enum irql tirql = spin_lock_high(&t->lock);

        bool apc =
            atomic_load_rlx(&t->wait_type) == THREAD_WAIT_INTERRUPTIBLE &&
            !delivered && atomic_load(&t->apc_pending_mask) != 0;

        thread_clear_flag(t, THREAD_FLAG_YIELDED);

        thread_set_state(t, apc ? THREAD_STATE_RUNNING : t->wait_state);

        thread_update_runtime_buckets(t, time_get_ms());

        spin_unlock(&t->lock, tirql);
        spin_unlock(&t->wait_lock, irql);

        if (apc)
            apc_check_and_deliver(t);

        delivered = apc;

        scheduler_yield();
    }
}

static void wait_timeout_send(struct timer *timer) {
    struct thread *t = container_of(timer, struct thread, wait_timer);

    enum irql irql = spin_lock_high(&t->wait_lock);

    if (!t->wait_done && wait_epoch(t) == t->wait_timer_epoch) {
        unlink_blocks(t, THREAD_WAIT_KEY_NONE);
        t->wait_done = true;
        t->wait_status = THREAD_WAIT_TIMED_OUT;
        t->wait_key = THREAD_WAIT_KEY_NONE;
        t->wait_completion_reason = THREAD_WAKE_REASON_SLEEP_TIMEOUT;
        scheduler_complete_object_wait(t, t->wait_completion_reason);
    }

    spin_unlock(&t->wait_lock, irql);
}

struct thread_wait_result thread_wait_complete_timeout(time_ms_t timeout_ms) {
    struct thread *t = thread_get_current();

    enum irql irql = spin_lock_high(&t->wait_lock);
    enforce(t->active_wait_blocks);

    bool armed = !t->wait_done;
    t->wait_timer_epoch = atomic_load_rlx(&t->wait_epoch);

    spin_unlock(&t->wait_lock, irql);

    if (armed) {
        timer_init(&t->wait_timer, wait_timeout_send, NULL);

        /* TOPC_NONE because it is not imperative that we
         * have the timer run on this CPU */
        t->wait_timer.flags = TIMER_FLAG_IRQ | TIMER_FLAG_PINNED |
                              TIMER_FLAG_CPU(smp_id(TOPC_NONE));

        timer_modify(&t->wait_timer, timer_delta_us(MS_TO_US(timeout_ms)));
    }

    struct thread_wait_result result = thread_wait_complete();

    /* Gotta remove it since new waits will reuse this timer */
    if (armed)
        timer_delete_sync(&t->wait_timer);

    return result;
}

void thread_wait_cancel(void) {
    struct thread *t = thread_get_current();
    enum irql irql = spin_lock_high(&t->wait_lock);
    enforce(t->active_wait_blocks);
    unlink_blocks(t, THREAD_WAIT_KEY_NONE);
    finish_locked(t);
    spin_unlock(&t->wait_lock, irql);
}

void thread_alert(struct thread *cc_nonnull_ptr t) {
    enforce_precond(t);
    enum irql irql = spin_lock_high(&t->wait_lock);

    if (!thread_test_flag(t, THREAD_FLAG_DYING)) {
        t->alert_pending = true;
        deliver_alert_locked(t);
    }

    spin_unlock(&t->wait_lock, irql);
}

void thread_park(void) {
    prepare_wait(/* objects = */ NULL, /* count = */ 0, /* storage = */ NULL,
                 THREAD_WAIT_INTERRUPTIBLE, THREAD_SLEEP_REASON_MANUAL,
                 THREAD_STATE_SLEEPING);
    thread_wait_complete();
}

void thread_wait_header_init(struct thread_wait_header *header) {
    spinlock_init(&header->lock);
    list_head_init(&header->waiters);
}

void thread_wait_init(struct thread *t) {
    spinlock_init(&t->wait_lock);

    t->active_wait_blocks = NULL;
    t->wait_block_count = 0;
    atomic_store_rlx(&t->wait_epoch, 0);
    t->wait_key = THREAD_WAIT_KEY_NONE;
    t->object_wait = false;
    t->alert_pending = false;

    for (size_t i = 0; i < THREAD_NUM_WAIT_BLOCKS; i++) {
        memset(&t->wait_blocks[i], 0, sizeof(struct thread_wait_block));
        list_node_init(&t->wait_blocks[i].obj_node);
    }
}
