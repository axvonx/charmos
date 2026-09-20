#include "tests/test_internal.h"

#include <sch/irql.h>
#include <sync/once_token.h>
#include <test/sync.h>
#include <time/spin_sleep.h>

TEST_GROUP_DECLARE(test_sync);

struct latch_fix {
    struct test_latch latch;
    _Atomic bool worker_returned;
    _Atomic bool worker_saw_set;
};

TEST_DECLARE_UNIT(test_sync, latch_spin_at_raised_irql) {
    struct test_latch l;
    test_latch_init(&l);

    enum irql irql = irql_raise(IRQL_DISPATCH_LEVEL);
    bool timed_out = !test_latch_spin_timeout(&l, 50);
    test_latch_set(&l);
    bool saw_set = test_latch_spin_timeout(&l, 50);
    irql_lower(irql);

    TEST_ASSERT(timed_out);
    TEST_ASSERT(saw_set);
    return TEST_SUCCESS;
}

struct phase_fix {
    struct test_phase phase;
    _Atomic uint32_t reached;
    _Atomic uint32_t failed;
};

static void phase_climber(void *arg) {
    struct phase_fix *f = arg;

    if (!test_phase_wait(&f->phase, 1, 5000)) {
        atomic_fetch_add(&f->failed, 1);
        return;
    }
    atomic_fetch_add(&f->reached, 1);
}

static void phase_forever_waiter(void *arg) {
    struct phase_fix *f = arg;

    /* Phase 99 is never published */
    if (!test_phase_wait(&f->phase, 99, 5000))
        atomic_fetch_add(&f->failed, 1);
    else
        atomic_fetch_add(&f->reached, 1);
}

TEST_DECLARE_UNIT(test_sync, phase_advance_wakes_waiters) {
    struct phase_fix f = {0};
    test_phase_init(&f.phase);

    struct thread *t[4];
    for (size_t i = 0; i < TEST_ARRAY_LEN(t); i++) {
        t[i] = thread_spawn_joinable("phase_climber", phase_climber, &f);
        TEST_ASSERT_NONNULL(t[i]);
    }

    sleep_spin_ms(20);
    TEST_ASSERT_EQ(test_phase_get(&f.phase), 0);
    test_phase_advance(&f.phase);

    for (size_t i = 0; i < TEST_ARRAY_LEN(t); i++)
        thread_join(t[i]);

    TEST_ASSERT_EQ(atomic_load(&f.reached), TEST_ARRAY_LEN(t));
    TEST_ASSERT_EQ(atomic_load(&f.failed), 0);
    TEST_ASSERT_EQ(test_phase_get(&f.phase), 1);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(test_sync, phase_poison_unblocks_all) {
    struct phase_fix f = {0};
    test_phase_init(&f.phase);

    struct thread *t[4];
    for (size_t i = 0; i < TEST_ARRAY_LEN(t); i++) {
        t[i] = thread_spawn_joinable("phase_waiter", phase_forever_waiter, &f);
        TEST_ASSERT_NONNULL(t[i]);
    }

    sleep_spin_ms(20);
    TEST_ASSERT(!test_phase_is_poisoned(&f.phase));
    test_phase_poison(&f.phase);

    for (size_t i = 0; i < TEST_ARRAY_LEN(t); i++)
        thread_join(t[i]);

    /* Every waiter must fail out */
    TEST_ASSERT_EQ(atomic_load(&f.failed), TEST_ARRAY_LEN(t));
    TEST_ASSERT_EQ(atomic_load(&f.reached), 0);
    TEST_ASSERT(test_phase_is_poisoned(&f.phase));

    /* A wait entered after poisoning must fail */
    TEST_ASSERT(!test_phase_wait(&f.phase, 0xFFFF, 100));
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(test_sync, phase_timeout_expires) {
    struct test_phase p;
    test_phase_init(&p);

    time_ms_t start = time_get_ms();
    TEST_ASSERT(!test_phase_wait(&p, 7, 100));
    time_ms_t took = time_get_ms() - start;

    TEST_ASSERT_GE(took, 90);
    TEST_ASSERT_LT(took, 2000);
    return TEST_SUCCESS;
}

TEST_DECLARE_UNIT(test_sync, phase_already_at_target) {
    struct test_phase p;
    test_phase_init(&p);
    test_phase_set(&p, 5);

    time_ms_t start = time_get_ms();
    TEST_ASSERT(test_phase_wait(&p, 5, 5000));
    TEST_ASSERT_LT(time_get_ms() - start, 1000);
    TEST_ASSERT_EQ(test_phase_get(&p), 5);
    return TEST_SUCCESS;
}

enum test_sync_token_state {
    TOKEN_INIT = 0,
    TOKEN_WORKING = 1,
    TOKEN_DONE = 2,
};

TEST_DECLARE_UNIT(test_sync, once_token_typechecked_ops) {
    ONCE_TOKEN_DEFINE(bool, btok);
    once_token_typecheck_internal(&btok);
    once_token_typecheck_internal_bool(&btok);

    TEST_ASSERT(!once_token_claimed(&btok));
    TEST_ASSERT(once_token_claim(&btok));
    TEST_ASSERT(!once_token_claim(&btok));
    TEST_ASSERT(once_token_claimed(&btok));
    once_token_reset(&btok);
    TEST_ASSERT(!once_token_claimed(&btok));

    ONCE_TOKEN_DEFINE(enum test_sync_token_state, stok) = {0};
    once_token_typecheck_internal(&stok);
    once_token_typecheck_internal_type(&stok, enum test_sync_token_state);

    TEST_ASSERT(once_token_claimed(&stok, TOKEN_INIT));
    TEST_ASSERT(once_token_claim(&stok, TOKEN_INIT, TOKEN_WORKING));
    TEST_ASSERT(once_token_claimed(&stok, TOKEN_WORKING));
    TEST_ASSERT(!once_token_claim(&stok, TOKEN_INIT, TOKEN_DONE));
    once_token_reset(&stok, TOKEN_DONE);
    TEST_ASSERT_EQ(once_token_read(&stok), TOKEN_DONE);

    return TEST_SUCCESS;
}
