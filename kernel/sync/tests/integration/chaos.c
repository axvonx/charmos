#include "sync/tests/test_internal.h"

struct chaos_state {
    struct thread *t;
    atomic_bool alive;
};

#define CHAOS_THREADS_MAX 64
static size_t chaos_threads = 12;

#if 0
#define CHAOS_LOG(fmt, ...)                                                    \
    test_info("[chaos %lu] " fmt, time_get_ms(), ##__VA_ARGS__)
#else
#define CHAOS_LOG(fmt, ...) ((void) 0)
#endif

static size_t chaos_iters_count = 300;
static struct chaos_state states[CHAOS_THREADS_MAX];
static atomic_bool chaos_stop = false;
static atomic_bool starter_ok = false;
static _Atomic uint32_t sync_chaos_left = 0;

static struct mutex chaos_fuzz_mtx = MUTEX_INIT;
static struct rwlock chaos_fuzz_rw = RWLOCK_INIT(THREAD_PRIO_CLASS_TIMESHARE);
static struct spinlock chaos_fuzz_spin = SPINLOCK_INIT;
static struct qspinlock chaos_fuzz_qspin = QSPINLOCK_INIT;

static _Atomic uint64_t chaos_apc_lock_taken = 0;
static _Atomic uint64_t chaos_apc_lock_skips = 0;

static _Atomic uint64_t chaos_iters[CHAOS_THREADS_MAX];
static _Atomic uint64_t chaos_spammer_iters;
static _Atomic uint64_t chaos_waker_iters;

/* Contend on a global lock from inside the APC,
 * as the interleaving is what we fuzz.
 *
 * Blocking here means we spin irq disabled,
 * so we trylock and record skips */
static void chaos_apc_fn(void *arg) {
    unused(arg);
    CHAOS_LOG("apc executed on %p", thread_get_current());

    enum irql irql;
    if (!spin_trylock_irq_disable(&chaos_fuzz_spin, &irql)) {
        atomic_fetch_add_explicit(&chaos_apc_lock_skips, 1,
                                  memory_order_relaxed);
        return;
    }

    atomic_fetch_add_explicit(&chaos_apc_lock_taken, 1, memory_order_relaxed);
    for (volatile int j = 0; j < 10; j++)
        cpu_relax();
    spin_unlock(&chaos_fuzz_spin, irql);
}

static void chaos_apc_spammer(void *arg) {
    unused(arg);
    CHAOS_LOG("apc spammer start");

    while (!atomic_load(&chaos_stop)) {
        atomic_fetch_add_explicit(&chaos_spammer_iters, 1,
                                  memory_order_relaxed);
        int id = prng_next() % chaos_threads;

        if (!atomic_load(&states[id].alive)) {
            scheduler_yield();
            continue;
        }

        if (!thread_get(states[id].t)) {
            scheduler_yield();
            continue;
        }

        struct apc *a = kmalloc(sizeof(struct apc), ALLOC_FLAGS_ZERO);
        if (a) {
            apc_init(a, chaos_apc_fn, NULL, apc_destroy_free);
            CHAOS_LOG("queue apc to %p", states[id].t);
            apc_enqueue(states[id].t, a, APC_TYPE_KERNEL);
            apc_put(a);
        }
        thread_put(states[id].t);

        scheduler_yield();
    }
}

static void chaos_sleeper(void *arg) {
    size_t id = (size_t) arg;
    struct thread *t = thread_get_current();
    states[id].t = t;
    atomic_store(&states[id].alive, true);

    while (!atomic_load(&starter_ok))
        cpu_relax();

    for (size_t i = 0; i < chaos_iters_count; i++) {
        atomic_fetch_add_explicit(&chaos_iters[id], 1, memory_order_relaxed);

        /* Exercise mutex */
        mutex_lock(&chaos_fuzz_mtx);
        for (volatile int j = 0; j < (int) (prng_next() & 0xF); j++)
            cpu_relax();
        mutex_unlock(&chaos_fuzz_mtx);

        /* Exercise rwlock */
        if (prng_next() & 1) {
            rw_lock(&chaos_fuzz_rw, RWLOCK_ACQUIRE_READ);
            for (volatile int j = 0; j < (int) (prng_next() & 0xF); j++)
                cpu_relax();
            rw_unlock(&chaos_fuzz_rw);
        } else {
            rw_lock(&chaos_fuzz_rw, RWLOCK_ACQUIRE_WRITE);
            for (volatile int j = 0; j < (int) (prng_next() & 0xF); j++)
                cpu_relax();
            rw_unlock(&chaos_fuzz_rw);
        }

        /* Exercise qspinlock */
        enum irql irql = qspin_lock(&chaos_fuzz_qspin);
        for (volatile int j = 0; j < (int) (prng_next() & 0xF); j++)
            cpu_relax();
        qspin_unlock(&chaos_fuzz_qspin, irql);

        /* Sleep and wait for waker */
        thread_prepare_to_sleep(t, THREAD_SLEEP_REASON_MANUAL,
                                THREAD_WAIT_INTERRUPTIBLE, (void *) id);
        thread_yield_until_wake_match();
        CHAOS_LOG("sleeper %zu woke up, iter %zu", id, i);
    }

    atomic_store(&states[id].alive, false);
    atomic_fetch_sub(&sync_chaos_left, 1);
}

static void chaos_waker(void *arg) {
    unused(arg);
    CHAOS_LOG("waker start");

    while (!atomic_load(&chaos_stop)) {
        atomic_fetch_add_explicit(&chaos_waker_iters, 1, memory_order_relaxed);
        int id = prng_next() % chaos_threads;

        if (!atomic_load(&states[id].alive)) {
            scheduler_yield();
            continue;
        }

        if (!thread_get(states[id].t)) {
            scheduler_yield();
            continue;
        }

        CHAOS_LOG("wake %p", states[id].t);
        thread_wake(states[id].t, THREAD_WAKE_REASON_SLEEP_MANUAL,
                    THREAD_PRIO_CLASS_TIMESHARE, (void *) (uintptr_t) id);
        thread_put(states[id].t);

        scheduler_yield();
    }
}

/* Ring depth per thread in a stall report */
#define CHAOS_ARMS_FOCUS 16
#define CHAOS_ARMS_LIVE 4

#define CHAOS_JOIN_POLL_MS 5000
#define CHAOS_QUIET_POLLS 3
#define CHAOS_MAX_REPORTS 2

static bool chaos_progress_moved(_Atomic uint64_t *counter, uint64_t *last) {
    uint64_t now = atomic_load_explicit(counter, memory_order_relaxed);
    bool moved = now != *last;
    *last = now;
    return moved;
}

/* `max_arms` caps the wait-arm ring dump and 0 skips the trace */
static void chaos_dump_thread(const char *role, size_t idx, struct thread *t,
                              uint64_t max_arms) {
    if (!t) {
        log_msg(LOG_ERROR, "  %s[%zu]: <null>", role, idx);
        return;
    }

    log_msg(LOG_ERROR, "  %s[%zu] '%s': state=%d flags=0x%x wait_type=%d", role,
            idx, t->name, (int) thread_get_state(t),
            (unsigned) thread_get_flags(t), (int) thread_get_wait_type(t));

    log_msg(LOG_ERROR,
            "  %s[%zu]   wake_src=%p expected=%p mask=0x%x apcs_ran=%u "
            "ctxsw=%zu",
            role, idx, (void *) atomic_load(&t->wake_src), t->expected_wake_src,
            (unsigned) atomic_load(&t->apc_pending_mask), t->total_apcs_ran,
            t->context_switches);

    if (max_arms)
        thread_dump_wait_trace(t, role, idx, max_arms);
}

static void chaos_report_stall(const char *waiting_on, size_t waiting_idx,
                               struct thread **threads, struct thread *spammer,
                               struct thread *waker) {
    log_msg(LOG_ERROR,
            "chaos: no progress while joining %s[%zu] -- left=%u stop=%d "
            "threads=%zu apc_taken=%llu apc_skips=%llu",
            waiting_on, waiting_idx, atomic_load(&sync_chaos_left),
            (int) atomic_load(&chaos_stop), chaos_threads,
            (unsigned long long) atomic_load(&chaos_apc_lock_taken),
            (unsigned long long) atomic_load(&chaos_apc_lock_skips));

    log_msg(LOG_ERROR, "  spammer_iters=%llu waker_iters=%llu",
            (unsigned long long) atomic_load(&chaos_spammer_iters),
            (unsigned long long) atomic_load(&chaos_waker_iters));

    bool stuck_sleeper = strcmp(waiting_on, "sleeper") == 0;

    for (size_t i = 0; i < chaos_threads; i++) {
        log_msg(LOG_ERROR, "  sleeper[%zu] alive=%d iters=%llu", i,
                (int) atomic_load(&states[i].alive),
                (unsigned long long) atomic_load(&chaos_iters[i]));
        uint64_t arms = 0;
        if (stuck_sleeper && i == waiting_idx)
            arms = CHAOS_ARMS_FOCUS;
        else if (atomic_load(&states[i].alive))
            arms = CHAOS_ARMS_LIVE;

        chaos_dump_thread("sleeper", i, threads[i], arms);
    }

    uint64_t helper_arms = stuck_sleeper ? CHAOS_ARMS_LIVE : CHAOS_ARMS_FOCUS;
    chaos_dump_thread("spammer", 0, spammer, helper_arms);
    chaos_dump_thread("waker", 0, waker, helper_arms);
}

static void chaos_join_watched(struct thread *t, const char *role, size_t idx,
                               _Atomic uint64_t *counter,
                               struct thread **threads, struct thread *spammer,
                               struct thread *waker) {
    uint64_t last = atomic_load_explicit(counter, memory_order_relaxed);

    size_t quiet = 0;
    size_t reports = 0;
    int status;

    while (!thread_join_timeout(t, CHAOS_JOIN_POLL_MS, &status)) {
        if (chaos_progress_moved(counter, &last)) {
            quiet = 0;
            continue;
        }

        if (++quiet < CHAOS_QUIET_POLLS)
            continue;

        quiet = 0;
        if (reports++ < CHAOS_MAX_REPORTS)
            chaos_report_stall(role, idx, threads, spammer, waker);
    }
}

TEST_DECLARE_INTEGRATION(mutex, interruptible_apc_fuzz,
                         TEST_INTENSITY(4, 12, CHAOS_THREADS_MAX)) {
    if (global.core_count < 2) {
        return TEST_SKIP(TEST_SKIP_NONE);
    }

    chaos_threads = ctx->intensity_val ? ctx->intensity_val : 12;
    if (chaos_threads > CHAOS_THREADS_MAX)
        chaos_threads = CHAOS_THREADS_MAX;

    for (size_t i = 0; i < chaos_threads; i++) {
        states[i].t = NULL;
        atomic_store(&states[i].alive, false);
    }

    atomic_store(&sync_chaos_left, chaos_threads);
    atomic_store(&chaos_stop, false);
    atomic_store(&starter_ok, false);
    atomic_store(&chaos_apc_lock_taken, 0);
    atomic_store(&chaos_apc_lock_skips, 0);
    atomic_store(&chaos_spammer_iters, 0);
    atomic_store(&chaos_waker_iters, 0);
    for (size_t i = 0; i < CHAOS_THREADS_MAX; i++)
        atomic_store(&chaos_iters[i], 0);

    struct thread *threads[CHAOS_THREADS_MAX];
    for (size_t i = 0; i < chaos_threads; i++) {
        threads[i] = thread_create("cs", chaos_sleeper, (void *) i);
        TEST_ASSERT_NONNULL(threads[i]);
        thread_set_joinable(threads[i]);
        kassert(thread_get(threads[i]));
        thread_enqueue(threads[i]);
    }

    struct thread *spammer =
        thread_spawn_joinable("chaos_apc_spammer", chaos_apc_spammer, NULL);
    TEST_ASSERT_NONNULL(spammer);
    struct thread *waker =
        thread_spawn_joinable("chaos_waker", chaos_waker, NULL);
    TEST_ASSERT_NONNULL(waker);

    atomic_store(&starter_ok, true);

    for (size_t i = 0; i < chaos_threads; i++)
        chaos_join_watched(threads[i], "sleeper", i, &chaos_iters[i], threads,
                           spammer, waker);

    atomic_store(&chaos_stop, true);

    chaos_join_watched(spammer, "spammer", 0, &chaos_spammer_iters, threads,
                       spammer, waker);
    spammer = NULL; /* The join consumed its last caller-owned reference. */
    chaos_join_watched(waker, "waker", 0, &chaos_waker_iters, threads, spammer,
                       waker);

    for (size_t i = 0; i < chaos_threads; i++)
        thread_put(threads[i]);

    TEST_ASSERT_EQ(atomic_load(&sync_chaos_left), 0);

    test_info("apc lock: %llu taken, %llu skipped",
              (unsigned long long) atomic_load(&chaos_apc_lock_taken),
              (unsigned long long) atomic_load(&chaos_apc_lock_skips));

    return TEST_SUCCESS;
}
