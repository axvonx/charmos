#include <log.h>
#include <thread/thread.h>
#include <thread/thread_diag.h>

struct thread_diag *thread_diag(struct thread *t) {
    if (!t || !(thread_get_flags(t) & THREAD_FLAG_DIAG))
        return NULL;

    return t->private;
}

bool thread_diag_attach(struct thread *t, struct thread_diag *d) {
    if (!t || !d)
        return false;

    if (t->private)
        return false;

    t->private = d;
    thread_or_flags(t, THREAD_FLAG_DIAG);
    return true;
}

void thread_diag_detach(struct thread *t) {
    if (!t || !(thread_get_flags(t) & THREAD_FLAG_DIAG))
        return;

    thread_and_flags(t, ~THREAD_FLAG_DIAG);
    t->private = NULL;
}

void thread_diag_record_arm(struct thread *t, const char *site, void *ra,
                            void *object, enum thread_state state,
                            enum thread_wait_type type, uint8_t reason) {
    struct thread_diag *d = thread_diag(t);
    if (!d)
        return;

    d->wait_arm_total++;

    struct thread_wait_arm *newest =
        d->wait_arm_count
            ? &d->wait_trace[(d->wait_arm_count - 1) % THREAD_WAIT_TRACE_DEPTH]
            : NULL;

    if (newest && newest->object == object &&
        newest->block_count == t->wait_block_count &&
        newest->wait_type == (uint8_t) type &&
        newest->state == (uint8_t) state && newest->reason == reason) {
        newest->last_epoch = t->wait_epoch;
        newest->last_ms = time_get_ms();
        newest->repeats++;
        return;
    }

    struct thread_wait_arm *e =
        &d->wait_trace[d->wait_arm_count % THREAD_WAIT_TRACE_DEPTH];

    e->site = site;
    e->ra = ra;
    e->object = object;
    e->first_epoch = t->wait_epoch;
    e->last_epoch = t->wait_epoch;
    e->repeats = 1;
    e->last_ms = time_get_ms();
    e->state = (uint8_t) state;
    e->wait_type = (uint8_t) type;
    e->reason = reason;
    e->block_count = t->wait_block_count;

    d->wait_arm_count++;
}

void thread_diag_apc_deliver_enter(struct thread *t, void *ra) {
    struct thread_diag *d = thread_diag(t);
    if (!d)
        return;

    d->apc_deliver_entries++;
    d->apc_last_deliver_ra = ra;
}

void thread_diag_apc_deliver_batch(struct thread *t, uint64_t drained) {
    struct thread_diag *d = thread_diag(t);
    if (!d)
        return;

    if (drained > d->apc_deliver_max)
        d->apc_deliver_max = drained;
}

static const char *thread_wait_type_str(uint8_t type) {
    switch (type) {
    case THREAD_WAIT_NONE: return "NONE";
    case THREAD_WAIT_UNINTERRUPTIBLE: return "UNINT";
    case THREAD_WAIT_INTERRUPTIBLE: return "INTR";
    default: return "?";
    }
}

void thread_dump_wait_trace(struct thread *t, const char *role, size_t idx,
                            uint64_t max_arms) {
    struct thread_diag *d = thread_diag(t);
    if (!d)
        return;

    uint64_t count = d->wait_arm_count;

    log_msg(LOG_ERROR, "  %s[%zu]   arms=%llu (%llu distinct)", role, idx,
            d->wait_arm_total, count);

    log_msg(LOG_ERROR,
            "  %s[%zu]   apc: entries=%llu max_burst=%llu deliver_ra=%p", role,
            idx, d->apc_deliver_entries, d->apc_deliver_max,
            d->apc_last_deliver_ra);

    uint64_t depth =
        max_arms < THREAD_WAIT_TRACE_DEPTH ? max_arms : THREAD_WAIT_TRACE_DEPTH;
    uint64_t shown = count < depth ? count : depth;

    for (uint64_t i = 0; i < shown; i++) {
        uint64_t seq = count - 1 - i;
        struct thread_wait_arm *e =
            &d->wait_trace[seq % THREAD_WAIT_TRACE_DEPTH];

        log_msg(LOG_ERROR, "  %s[%zu]   arm#%llu %s ra=%p x%llu", role, idx,
                seq, e->site ? e->site : "?", e->ra, e->repeats);

        log_msg(LOG_ERROR,
                "  %s[%zu]     -> state=%d %s object=%p reason=%u "
                "epoch=%llu..%llu last=%llums blocks=%u",
                role, idx, (int) e->state, thread_wait_type_str(e->wait_type),
                e->object, (unsigned) e->reason, e->first_epoch, e->last_epoch,
                e->last_ms, e->block_count);
    }
}
