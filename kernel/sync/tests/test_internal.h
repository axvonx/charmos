#pragma once
#include <test/test.h>

#include <atomic.h>
#include <crypto/prng.h>
#include <log.h>
#include <mem/alloc.h>
#include <sch/sched.h>
#include <smp/core.h>
#include <string.h>
#include <sync/mutex.h>
#include <sync/mutex_simple.h>
#include <sync/rcu.h>
#include <sync/rwlock.h>
#include <thread/apc.h>
#include <thread/thread.h>
#include <thread/workqueue.h>
#include <time/spin_sleep.h>

#include <sync/qspinlock.h>
#include <sync/turnstile.h>

TEST_GROUP_DECLARE(mutex);
TEST_GROUP_DECLARE(rcu);
TEST_GROUP_DECLARE(rwlock);
TEST_GROUP_DECLARE(qspinlock);
TEST_GROUP_DECLARE(turnstile);
TEST_GROUP_DECLARE(lock_chk);
TEST_GROUP_DECLARE(raw_spinlock);
TEST_GROUP_DECLARE(condvar);
TEST_GROUP_DECLARE(semaphore);
TEST_GROUP_DECLARE(completion);
