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

TEST_GROUP_EXTERN(mutex);
TEST_GROUP_EXTERN(rcu);
TEST_GROUP_EXTERN(rwlock);
TEST_GROUP_EXTERN(qspinlock);
TEST_GROUP_EXTERN(turnstile);
TEST_GROUP_EXTERN(lock_chk);
TEST_GROUP_EXTERN(raw_spinlock);
TEST_GROUP_EXTERN(condvar);
TEST_GROUP_EXTERN(semaphore);
TEST_GROUP_EXTERN(completion);
