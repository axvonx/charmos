#include <sch/irql.h>
#include <string.h>
#include <structures/list.h>
#include <sync/event.h>
#include <sync/spinlock.h>
#include <thread/thread_types.h>
#include <thread/wait.h>

#include "sync/tests/test_internal.h"

TEST_GROUP_DEFINE(event);

static size_t linked_waiters(struct event *e) {
    size_t n = 0;
    struct list_node *pos;

    enum irql irql = spin_lock_high(&e->header.lock);
    list_for_each(pos, &e->header.waiters)
        n++;
    spin_unlock(&e->header.lock, irql);

    return n;
}

static void wait_for_linked(struct event *e, size_t n) {
    while (linked_waiters(e) < n)
        scheduler_yield();
}

struct event_waiter_args {
    struct event *e;
    atomic_uint *done;
    struct thread_wait_result result;
};

static void event_waiter(void *arg) {
    struct event_waiter_args *a = arg;
    a->result = event_wait(a->e, THREAD_WAIT_UNINTERRUPTIBLE,
                           THREAD_BLOCK_REASON_MANUAL);
    atomic_inc(a->done);
}

TEST_DEFINE_UNIT(event, sync_latch_and_consume) {
    struct event e;
    event_init(&e, EVENT_SYNCHRONIZATION, false);

    TEST_ASSERT(!event_set(&e));
    TEST_ASSERT(event_set(&e)); /* Already latched */
    TEST_ASSERT(event_read_state(&e));

    struct thread_wait_result r =
        event_wait(&e, THREAD_WAIT_UNINTERRUPTIBLE, THREAD_BLOCK_REASON_MANUAL);
    TEST_ASSERT_EQ(r.status, THREAD_WAIT_SATISFIED);
    TEST_ASSERT(!event_read_state(&e));
    TEST_ASSERT(list_head_empty(&e.header.waiters));
    return TEST_SUCCESS;
}

TEST_DEFINE_UNIT(event, sync_hands_off_to_waiter) {
    struct event e;
    atomic_uint done = 0;
    event_init(&e, EVENT_SYNCHRONIZATION, false);

    struct event_waiter_args a = {.e = &e, .done = &done};
    struct thread *t =
        thread_spawn("event_waiter", event_waiter, .arg = &a, .joinable = true);
    TEST_ASSERT_NONNULL(t);

    wait_for_linked(&e, 1);
    TEST_ASSERT(!event_set(&e));
    thread_join(t);

    /* The signal went to the waiter */
    TEST_ASSERT(!event_read_state(&e));
    TEST_ASSERT_EQ(a.result.status, THREAD_WAIT_SATISFIED);
    TEST_ASSERT_EQ(atomic_load(&done), 1);
    return TEST_SUCCESS;
}

TEST_DEFINE_UNIT(event, notification_wakes_all_waiters) {
    enum { WAITERS = 4 };
    struct event e;
    atomic_uint done = 0;
    struct event_waiter_args args[WAITERS];
    struct thread *threads[WAITERS];
    event_init(&e, EVENT_NOTIFICATION, false);

    for (size_t i = 0; i < WAITERS; i++) {
        args[i] = (struct event_waiter_args){.e = &e, .done = &done};
        threads[i] = thread_spawn("event_waiter", event_waiter, .arg = &args[i],
                                  .joinable = true);
        TEST_ASSERT_NONNULL(threads[i]);
    }

    wait_for_linked(&e, WAITERS);
    TEST_ASSERT(!event_set(&e));

    for (size_t i = 0; i < WAITERS; i++) {
        thread_join(threads[i]);
        TEST_ASSERT_EQ(args[i].result.status, THREAD_WAIT_SATISFIED);
    }

    TEST_ASSERT_EQ(atomic_load(&done), WAITERS);
    TEST_ASSERT(event_read_state(&e));
    TEST_ASSERT(list_head_empty(&e.header.waiters));
    return TEST_SUCCESS;
}

TEST_DEFINE_UNIT(event, wait_any_signaled_at_registration) {
    struct event idle, ready;
    event_init(&idle, EVENT_SYNCHRONIZATION, false);
    event_init(&ready, EVENT_SYNCHRONIZATION, true);

    const struct thread_wait_object objects[2] = {event_wait_object(&idle),
                                                  event_wait_object(&ready)};
    thread_wait_prepare(objects, 2, NULL, THREAD_WAIT_UNINTERRUPTIBLE,
                        THREAD_BLOCK_REASON_MANUAL);

    TEST_ASSERT(thread_get_current()->wait_done);
    struct thread_wait_result r = thread_wait_complete();

    TEST_ASSERT_EQ(r.status, THREAD_WAIT_SATISFIED);
    TEST_ASSERT_EQ(r.key, 1);
    TEST_ASSERT(!event_read_state(&ready));
    TEST_ASSERT(list_head_empty(&idle.header.waiters));
    TEST_ASSERT(list_head_empty(&ready.header.waiters));
    TEST_ASSERT_EQ(thread_get_state(thread_get_current()),
                   THREAD_STATE_RUNNING);

    TEST_ASSERT(!event_set(&idle));
    TEST_ASSERT(event_read_state(&idle));
    return TEST_SUCCESS;
}

TEST_DEFINE_UNIT(event, wait_times_out) {
    struct event e;
    event_init(&e, EVENT_NOTIFICATION, false);

    struct thread_wait_result r = event_wait_timeout(
        &e, THREAD_WAIT_UNINTERRUPTIBLE, THREAD_BLOCK_REASON_MANUAL, 5);
    TEST_ASSERT_EQ(r.status, THREAD_WAIT_TIMED_OUT);
    TEST_ASSERT_EQ(r.key, THREAD_WAIT_KEY_NONE);
    TEST_ASSERT_EQ(r.reason, THREAD_WAKE_REASON_SLEEP_TIMEOUT);
    TEST_ASSERT(list_head_empty(&e.header.waiters));

    event_set(&e);
    r = event_wait_timeout(&e, THREAD_WAIT_UNINTERRUPTIBLE,
                           THREAD_BLOCK_REASON_MANUAL, 5);
    TEST_ASSERT_EQ(r.status, THREAD_WAIT_SATISFIED);
    return TEST_SUCCESS;
}

/* Event lives in waiter's stack, TODO: turn into death test */
struct stack_event_case {
    atomic(struct event *) published;
    atomic_uint rounds;
    atomic_bool stop;
};

enum { STACK_EVENT_ROUNDS = 500 };

static void stack_event_setter(void *arg) {
    struct stack_event_case *c = arg;

    while (atomic_load(&c->rounds) < STACK_EVENT_ROUNDS) {
        struct event *e = atomic_xchg_acq(&c->published, NULL);
        if (e)
            event_set(e);
        else
            scheduler_yield();
    }
}

static void stack_event_round(struct stack_event_case *c,
                              enum event_type type) {
    struct event e;
    event_init(&e, type, false);
    atomic_store_release(&c->published, &e);

    event_wait(&e, THREAD_WAIT_UNINTERRUPTIBLE, THREAD_BLOCK_REASON_MANUAL);
    memset(&e, 0xAA, sizeof(e));
}

TEST_DEFINE_UNIT(event, stack_event_lifetime) {
    struct stack_event_case c = {0};
    struct thread *setter = thread_spawn("event_setter", stack_event_setter,
                                         .arg = &c, .joinable = true);
    TEST_ASSERT_NONNULL(setter);

    for (size_t i = 0; i < STACK_EVENT_ROUNDS; i++) {
        stack_event_round(&c,
                          i & 1 ? EVENT_NOTIFICATION : EVENT_SYNCHRONIZATION);
        atomic_inc(&c.rounds);
    }

    thread_join(setter);
    return TEST_SUCCESS;
}
