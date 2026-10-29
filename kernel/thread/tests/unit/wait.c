
#include "thread/tests/test_internal.h"
#include <atomic.h>
#include <sch/irql.h>
#include <structures/list.h>
#include <structures/locked_list.h>
#include <sync/spinlock.h>
#include <thread/apc_types.h>
#include <thread/thread_types.h>
#include <thread/wait.h>

TEST_GROUP_DEFINE(wait_block);

TEST_DEFINE_UNIT(wait_block, caller_storage_wait_any) {
    struct thread_wait_header headers[6];
    struct thread_wait_object objects[6];
    struct thread_wait_block blocks[6];
    for (size_t i = 0; i < 6; i++) {
        thread_wait_header_init(&headers[i]);
        objects[i] = (struct thread_wait_object){.header = &headers[i],
                                                 .object = &headers[i]};
    }
    thread_wait_prepare(objects, 6, blocks, THREAD_WAIT_UNINTERRUPTIBLE,
                        THREAD_BLOCK_REASON_MANUAL);
    TEST_ASSERT_EQ(thread_get_current()->active_wait_blocks, &blocks[0]);
    TEST_ASSERT(thread_wait_satisfy(&blocks[4],
                                    THREAD_WAKE_REASON_BLOCKING_MANUAL, NULL));
    for (size_t i = 0; i < 6; i++) {
        TEST_ASSERT(list_head_empty(&headers[i].waiters));
        TEST_ASSERT_EQ(blocks[i].state, i == 4 ? THREAD_WAIT_BLOCK_SATISFIED
                                               : THREAD_WAIT_BLOCK_INACTIVE);
    }
    TEST_ASSERT_EQ(thread_wait_complete().key, 4);
    return TEST_SUCCESS;
}

struct wait_racer {
    struct thread_wait_header *header;
    atomic_bool *go;
    atomic_uint *wins;
};

static void race_satisfy(void *arg) {
    struct wait_racer *r = arg;
    while (!atomic_load(r->go))
        scheduler_yield();
    if (thread_wait_header_satisfy(r->header,
                                   THREAD_WAKE_REASON_BLOCKING_MANUAL, NULL))
        atomic_inc(r->wins);
}

TEST_DEFINE_UNIT(wait_block, mt_wait_any) {
    struct thread_wait_header headers[2];
    struct thread_wait_object objects[2];
    struct wait_racer racers[2];
    struct thread *wakers[2];
    atomic_bool go = false;
    atomic_uint wins = 0;
    for (size_t i = 0; i < 2; i++) {
        thread_wait_header_init(&headers[i]);
        objects[i] = (struct thread_wait_object){.header = &headers[i],
                                                 .object = &headers[i]};
        racers[i] = (struct wait_racer){&headers[i], &go, &wins};
        wakers[i] = thread_spawn("wait_racer", race_satisfy, .arg = &racers[i],
                                 .joinable = true);
        enforce(wakers[i]);
    }
    thread_wait_prepare(objects, 2, NULL, THREAD_WAIT_UNINTERRUPTIBLE,
                        THREAD_BLOCK_REASON_MANUAL);
    atomic_store(&go, true);
    uint16_t key = thread_wait_complete().key;
    for (size_t i = 0; i < 2; i++)
        thread_join(wakers[i]);
    TEST_ASSERT_LT(key, 2);
    TEST_ASSERT_EQ(atomic_load(&wins), 1);
    TEST_ASSERT(list_head_empty(&headers[0].waiters));
    TEST_ASSERT(list_head_empty(&headers[1].waiters));
    return TEST_SUCCESS;
}

struct apc_wait_case {
    struct thread_wait_header header;
    atomic_bool armed;
    atomic_bool lost_registration;
    atomic_uint delivered;
    uint64_t epoch;
};

static void satisfy_during_apc(struct apc_context *ac, void *arg) {
    cc_ignore(ac);
    struct apc_wait_case *c = arg;
    struct thread *t = thread_get_current();
    enum irql irql = spin_lock_high(&t->wait_lock);
    if (t->active_wait_blocks != t->wait_blocks ||
        atomic_load_rlx(&t->wait_epoch) != c->epoch ||
        t->wait_blocks[0].state != THREAD_WAIT_BLOCK_ACTIVE || t->alert_pending)
        atomic_store(&c->lost_registration, true);
    spin_unlock(&t->wait_lock, irql);
    /* Four APCs must return to the SAME registration, and fifth completes
     * it from inside delivery */
    if (atomic_fetch_add(&c->delivered, 1) == 4)
        thread_wait_header_satisfy(&c->header,
                                   THREAD_WAKE_REASON_BLOCKING_MANUAL, NULL);
}

static void apc_waiter(void *arg) {
    struct apc_wait_case *c = arg;
    thread_wait_prepare_one(&c->header, c, THREAD_WAIT_INTERRUPTIBLE,
                            THREAD_BLOCK_REASON_MANUAL);
    c->epoch = atomic_load_rlx(&thread_get_current()->wait_epoch);
    atomic_store(&c->armed, true);
    thread_wait_complete();
}

TEST_DEFINE_UNIT(wait_block, apc_preserves_registration) {
    struct apc_wait_case c = {0};
    thread_wait_header_init(&c.header);
    struct apc apc;
    apc_init(&apc, satisfy_during_apc, &c, NULL);
    struct thread *t =
        thread_spawn("apc_waiter", apc_waiter, .arg = &c, .joinable = true);
    TEST_ASSERT_NONNULL(t);
    while (!atomic_load(&c.armed))
        scheduler_yield();
    for (size_t i = 0; i < 5; i++) {
        enforce(apc_enqueue(t, &apc, APC_TYPE_KERNEL));
        while (atomic_load_acq(&apc.state) != APC_STATE_IDLE)
            scheduler_yield();
    }
    thread_join(t);
    apc_putref(&apc);
    TEST_ASSERT(!atomic_load(&c.lost_registration));
    TEST_ASSERT_EQ(atomic_load(&c.delivered), 5);
    TEST_ASSERT(list_head_empty(&c.header.waiters));
    return TEST_SUCCESS;
}

TEST_DEFINE_UNIT(wait_block, uninterruptible_latches_alert) {
    struct thread_wait_header header;
    struct thread *t = thread_get_current();
    thread_wait_header_init(&header);
    thread_wait_prepare_one(&header, &header, THREAD_WAIT_UNINTERRUPTIBLE,
                            THREAD_BLOCK_REASON_MANUAL);
    thread_alert(t);
    bool linked = !list_head_empty(&header.waiters);
    thread_wait_satisfy(&t->wait_blocks[0], THREAD_WAKE_REASON_SLEEP_MANUAL,
                        NULL);
    struct thread_wait_result result = thread_wait_complete();
    TEST_ASSERT_EQ(result.status, THREAD_WAIT_SATISFIED);
    TEST_ASSERT(linked);
    TEST_ASSERT(t->alert_pending);
    thread_park(); /* Consumes the latched permit without blocking */
    TEST_ASSERT(!t->alert_pending);
    return TEST_SUCCESS;
}

TEST_DEFINE_UNIT(wait_block, stale_timeout_cannot_satisfy_next_wait) {
    struct thread_wait_header header;
    struct thread *t = thread_get_current();
    thread_wait_header_init(&header);
    thread_wait_prepare_one(&header, &header, THREAD_WAIT_UNINTERRUPTIBLE,
                            THREAD_BLOCK_REASON_MANUAL);
    uint64_t old_epoch = atomic_load_rlx(&t->wait_epoch);
    thread_wait_cancel();
    thread_wait_prepare_one(&header, &header, THREAD_WAIT_UNINTERRUPTIBLE,
                            THREAD_BLOCK_REASON_MANUAL);
    bool stale_won = thread_wait_satisfy_epoch(
        &t->wait_blocks[0], old_epoch, THREAD_WAKE_REASON_SLEEP_TIMEOUT, NULL);
    thread_wait_satisfy(&t->wait_blocks[0], THREAD_WAKE_REASON_SLEEP_MANUAL,
                        NULL);
    struct thread_wait_result result = thread_wait_complete();
    TEST_ASSERT(!stale_won);
    TEST_ASSERT_EQ(result.reason, THREAD_WAKE_REASON_SLEEP_MANUAL);
    return TEST_SUCCESS;
}
