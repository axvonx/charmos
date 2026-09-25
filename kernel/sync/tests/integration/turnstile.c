#include "sync/tests/test_internal.h"

union collision_lock {
    struct mutex lock;
    uint8_t stride[TURNSTILE_HASH_SIZE * 8];
};

static union collision_lock collision_locks[2];

static_assert(sizeof(union collision_lock) == TURNSTILE_HASH_SIZE * 8,
              "collision fixture must preserve the hash stride");

static void collision_waiter(void *arg) {
    struct mutex *lock = arg;
    mutex_lock(lock);
    mutex_unlock(lock);
}

TEST_DECLARE_INTEGRATION(turnstile, lookup_collision_miss) {
    struct mutex *occupied = &collision_locks[0].lock;
    struct mutex *missing = &collision_locks[1].lock;
    TEST_ASSERT_EQ(TURNSTILE_OBJECT_HASH(occupied),
                   TURNSTILE_OBJECT_HASH(missing));
    mutex_init(occupied);
    mutex_init(missing);

    mutex_lock(occupied);
    struct thread *waiter = thread_spawn("ts_collision", collision_waiter,
                                         .arg = occupied, .joinable = true);
    if (!waiter) {
        mutex_unlock(occupied);
        return TEST_FAIL("cannot create collision waiter");
    }

    time_ms_t deadline = time_get_ms() + 1000;
    while (!atomic_load(&waiter->blocked_ts) && time_get_ms() < deadline)
        thread_sleep_for_ms(1);

    struct turnstile *found;
    enum irql irql = turnstile_lookup(occupied, &found);
    bool blocked = found && found->lock_obj == occupied && found->waiters == 1;
    size_t occupied_count = turnstile_get_waiter_count(occupied);
    size_t missing_count = turnstile_get_waiter_count(missing);
    turnstile_unlock(occupied, irql);

    struct turnstile *miss;
    irql = turnstile_lookup(missing, &miss);
    bool missed = miss == NULL;
    turnstile_unlock(missing, irql);

    mutex_unlock(occupied);
    thread_join(waiter);

    TEST_ASSERT(blocked);
    TEST_ASSERT_EQ(occupied_count, 1);
    TEST_ASSERT_MSG(missed, "lookup returned another lock's turnstile");
    TEST_ASSERT_EQ(missing_count, 0);
    return TEST_SUCCESS;
}

TEST_DECLARE_INTEGRATION(turnstile, lookup_collision_chain) {
    struct thread *waiters[2];
    bool blocked[2] = {false, false};
    bool found[2] = {false, false};
    bool joined[2];

    TEST_ASSERT_EQ(TURNSTILE_OBJECT_HASH(&collision_locks[0].lock),
                   TURNSTILE_OBJECT_HASH(&collision_locks[1].lock));

    struct mutex *lock0 = &collision_locks[0].lock;
    struct mutex *lock1 = &collision_locks[1].lock;
    mutex_init(lock0);
    mutex_init(lock1);

    mutex_lock_subclass(lock0, 0);
    waiters[0] = thread_spawn("ts_chain", collision_waiter, .arg = lock0,
                              .joinable = true);

    time_ms_t deadline = time_get_ms() + 1000;
    while (!atomic_load(&waiters[0]->blocked_ts) && time_get_ms() < deadline)
        thread_sleep_for_ms(1);
    blocked[0] = atomic_load(&waiters[0]->blocked_ts) != NULL;

    mutex_lock_subclass(lock1, 1);
    waiters[1] = thread_spawn("ts_chain", collision_waiter, .arg = lock1,
                              .joinable = true);

    deadline = time_get_ms() + 1000;
    while (!atomic_load(&waiters[1]->blocked_ts) && time_get_ms() < deadline)
        thread_sleep_for_ms(1);
    blocked[1] = atomic_load(&waiters[1]->blocked_ts) != NULL;

    for (size_t i = 0; i < 2; i++) {
        struct mutex *lock = &collision_locks[i].lock;
        struct turnstile *ts;
        enum irql irql = turnstile_lookup(lock, &ts);
        found[i] = ts && ts->lock_obj == lock && ts->waiters == 1;
        turnstile_unlock(lock, irql);
    }

    mutex_unlock(lock1);
    mutex_unlock(lock0);

    TEST_ASSERT_NONNULL(waiters[0]);
    TEST_ASSERT_NONNULL(waiters[1]);

    for (size_t i = 0; i < 2; i++) {
        joined[i] = thread_join_timeout(waiters[i], 1000, NULL);
        if (!joined[i])
            thread_detach(waiters[i]);
    }

    TEST_ASSERT(blocked[0] && blocked[1]);
    TEST_ASSERT_MSG(found[0] && found[1],
                    "inserting a colliding lock lost an existing turnstile");
    TEST_ASSERT(joined[0] && joined[1]);
    return TEST_SUCCESS;
}
