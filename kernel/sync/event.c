#include <sync/event.h>
#include <thread/thread.h>

struct event_set_ctx {
    struct event *e;
    bool prev;
};

static bool event_try_acquire(void *object) {
    struct event *e = object;

    if (e->type == EVENT_NOTIFICATION)
        return atomic_load_rlx(&e->signaled);

    return atomic_xchg_rlx(&e->signaled, false);
}

static void event_latch_locked(void *arg) {
    struct event_set_ctx *c = arg;
    c->prev = atomic_xchg_release(&c->e->signaled, true);
}

bool event_set_full(struct event *e, enum thread_resume_reason reason,
                    void (*callback)(struct thread *)) {
    struct event_set_ctx c = {.e = e, .prev = false};

    if (e->type == EVENT_NOTIFICATION) {
        /* Everyone that is linked now was
         * snapshotted by whatever set signaled it */
        if (atomic_load_acq(&e->signaled))
            return true;

        thread_wait_header_satisfy_all(&e->header, reason, callback,
                                       event_latch_locked, &c);
        return c.prev;
    }

    thread_wait_header_satisfy_one(&e->header, reason, callback,
                                   event_latch_locked, &c);
    return c.prev;
}

bool event_set(struct event *e) {
    return event_set_full(e, THREAD_WAKE_REASON_BLOCKING_MANUAL, NULL);
}

bool event_reset(struct event *e) {
    return atomic_xchg_rlx(&e->signaled, false);
}

void event_clear(struct event *e) {
    atomic_store_rlx(&e->signaled, false);
}

bool event_read_state(struct event *e) {
    return atomic_load_acq(&e->signaled);
}

struct thread_wait_object event_wait_object(struct event *e) {
    return (struct thread_wait_object){
        .header = &e->header, .object = e, .try_acquire = event_try_acquire};
}

/* We take the lock here so that setters that have signaled us
 * are known to have released it, so we can't race an unlock */
bool event_try_wait(struct event *e) {
    enum irql irql = spin_lock_high(&e->header.lock);

    bool acquired = event_try_acquire(e);

    spin_unlock(&e->header.lock, irql);
    return acquired;
}

static struct thread_wait_result event_fast_result(void) {
    return (struct thread_wait_result){.status = THREAD_WAIT_SATISFIED,
                                       .key = 0,
                                       .reason =
                                           THREAD_WAKE_REASON_BLOCKING_MANUAL};
}

static void event_prepare(struct event *e, enum thread_wait_type type,
                          enum thread_block_reason reason) {
    const struct thread_wait_object obj = event_wait_object(e);
    thread_wait_prepare(&obj, 1, NULL, type, reason);
}

struct thread_wait_result event_wait(struct event *e,
                                     enum thread_wait_type type,
                                     enum thread_block_reason reason) {
    if (event_try_wait(e))
        return event_fast_result();

    event_prepare(e, type, reason);
    return thread_wait_complete();
}

struct thread_wait_result event_wait_timeout(struct event *e,
                                             enum thread_wait_type type,
                                             enum thread_block_reason reason,
                                             time_ms_t timeout_ms) {
    if (event_try_wait(e))
        return event_fast_result();

    event_prepare(e, type, reason);
    return thread_wait_complete_timeout(timeout_ms);
}

void event_init(struct event *e, enum event_type type, bool signaled) {
    thread_wait_header_init(&e->header);
    e->type = type;
    atomic_store_rlx(&e->signaled, signaled);
}
